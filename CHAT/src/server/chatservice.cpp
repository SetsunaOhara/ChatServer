#include"chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
#include<string>
#include<vector>
#include<map>
using namespace std;
using namespace muduo;


//业务模块代码
//using namespace muduo::net;
// 获取单例对象的接口函数
ChatService* ChatService::instance()
{
	static ChatService service;
	return &service;
}

// 初始化：向Handler表中插入事件id(键)和对应的回调操作(值)          注册消息id以及对应的回调操作
ChatService::ChatService()
{
	msgHanlderMap.insert({LOGIN_MSG, bind(&ChatService::login,this,_1,_2,_3)});

	msgHanlderMap.insert({ LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3) });

	msgHanlderMap.insert({ REG_MSG, bind(&ChatService::reg,this,_1,_2,_3) });
	//LOGIN_MSG是msgHandlerMap这个哈希表的键，其值为0，代表登录操作的业务id，对应的值是登录业务所要进行的操作
	//这些id 由public.hpp定义
	msgHanlderMap.insert({ ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3) });

	// Friend
	msgHanlderMap.insert({ ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3) });

	// Group
	msgHanlderMap.insert({ CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3) });

	msgHanlderMap.insert({ ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3) });

	msgHanlderMap.insert({ GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3) });

	// 连接redis服务器
	if (m_redis.connect())
	{
		LOG_INFO << "m_redis connect success!!!";
		//设置上报消息的回调
		m_redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
	}

}

//服务器异常，业务重置方法
void ChatService::reset()
{
	// 把online状态的用户，设置成offline
	m_userModel.resetState();
}

//获取消息对应的处理器Handler
MsgHandler ChatService::getHandler(int msgid)
{
	//记录错误日志，msgid没有对应的事件处理回调
	auto it = msgHanlderMap.find(msgid);
	if (it == msgHanlderMap.end())
	{
		//返回一个默认的处理器，空操作
		return [=](const TcpConnectionPtr& conn, json& js, Timestamp receiveTime) {
			LOG_ERROR << "msgid:" << msgid << " can not find hanlder!"; 
		};
	}
	else
	{
		return msgHanlderMap[msgid];
	}
}

//处理登录业务  id  pwd,   检查输入的密码是否正确pwd == password
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	LOG_INFO << "do login service!!!";
	int id = js["id"].get<int>();
	string pwd = js["password"];

	//使用数据库操作类 查询用户，并使用一个User类存储
	User user = m_userModel.query(id);  //查不到，则会返回一个User()，默认user.Id等于-1
	if (user.getId() == id && user.getPwd()==pwd)
	{
		if (user.getState() == "online")
		{
			// 登录失败：该用户已经登录，不允许重复登录
			json response;
			response["msgid"] = LOGIN_MSG_ACK;
			response["errno"] = 2;
			response["errmsg"] = "this account has been logined!";
			response["id"] = user.getId();
			conn->send(response.dump());
		}
		else
		{
			// 登录成功，记录用户连接信息。同时，多用户并发同时登陆的时候，这里涉及多线程同时向map插入，要考虑线程安全
			// 注： 数据库的插入操作线程安全 由MySQL保证。
			{
				lock_guard<mutex> lock(m_connMutex);
				m_userConnMap.insert({ id,conn });
			}

			// id用户登录成功后，向redis订阅channel(id)
			m_redis.subscribe(id);

			// 登录成功，更新用户状态信息 state offline -> online
			user.setState("online");
			m_userModel.updateState(user);
			json response;
			response["msgid"] = LOGIN_MSG_ACK;
			response["errno"] = 0;
			response["id"] = user.getId();
			response["name"] = user.getName();

			// 检查该用户是否有离线消息
			auto msgVec = m_offlineMsgModel.query(id);
			if (!msgVec.empty())
			{
				response["offlineMsg"] = msgVec;

				// 读取该用户的离线消息以后，把该用户的所有离线消息删除掉
				m_offlineMsgModel.remove(id);
			}

			// 查询该用户离线时的好友信息并返回
			vector<User> userVec = m_friendModel.query(id);
			if (!userVec.empty())
			{
				vector<string> userMsgVec;
				for (User& user : userVec)
				{
					json js;
					js["id"] = user.getId();
					js["name"] = user.getName();
					js["state"] = user.getState();
					userMsgVec.push_back(js.dump());
				}
				response["friends"] = userMsgVec;
			}

			// 登录成功时，查询该用户离线时的群聊信息并返回
			vector<Group> groupuserVec = m_groupModel.queryGroups(id); //查询他加了哪些群
			if (!groupuserVec.empty())
			{
				//group::[{groupid:[xxx,xxx,xxx,xxx]}]
				vector<string> groupV;
				for (Group& group : groupuserVec)
				{
					json grpjs;
					grpjs["id"] = group.getId();
					grpjs["groupname"] = group.getName();
					grpjs["groupdesc"] = group.getDesc();
					vector<string>userV;
					for (GroupUser& user : group.getUser())
					{
						json js;
						js["id"] = user.getId();
						js["name"] = user.getName();
						js["state"] = user.getState();
						js["role"] = user.getRole();
						userV.push_back(js.dump());
					}
					grpjs["users"] = userV;
					groupV.push_back(grpjs.dump());
				}
				response["groups"] = groupV;
			}

			conn->send(response.dump());
		}
	}
	else
	{

		// 登录失败：该用户不存在 或 密码错误
		json response;
		response["msgid"] = LOGIN_MSG_ACK;
		response["errno"] = 1;
		response["errmsg"] = "userid or password is invalid!";
		response["id"] = user.getId();
		conn->send(response.dump());
	}
}

//处理注册业务  name   password
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	LOG_INFO << "do reg service!!!";
	string name = js["name"];
	string pwd = js["password"];

	// 创建一个数据表对象，用来存放 即将填入数据库的数据
	User user;
	user.setName(name);
	user.setPwd(pwd);

	// 使用User表的数据库操作类 UserMode  执行相应的数据库操作
	bool isOk = m_userModel.insert(user);
	if (isOk)
	{
		//注册成功
		json response;
		response["msgid"] = REG_MSG_ACK;
		response["errno"] = 0;
		response["id"] = user.getId();
		conn->send(response.dump());
	}
	else
	{
		//注册失败
		json response;
		response["msgid"] = REG_MSG_ACK;
		response["errno"] = 1;
		response["id"] = user.getId();
		conn->send(response.dump());
	}
}

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	int userid = js["id"].get<int>();
	{
		lock_guard<mutex> lock(m_connMutex);
		auto it = m_userConnMap.find(userid);
		if (it != m_userConnMap.end())
		{

			m_userConnMap.erase(it);
		}
	}

	// 用户注销，相当于就是下线，在redis中取消订阅通道
	m_redis.unsubscribe(userid);

	// 更新用户状态信息
	User m_user;
	m_user.setId(userid);
	m_user.setState("offline");
	m_userModel.updateState(m_user);
	//conn->send("");//这里并不会导致客户端接收到长度为0的字符串，而是send立即返回0，没有数据被发送到网络上，而recv并没有任何数据可以接收，继续阻塞
} 

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
	User user;
	{
		//防止有用户在退出 同时也有用户在登录，因此设计m_userConnMap的修改，都需要考虑线程安全
		lock_guard<mutex> lock(m_connMutex);
		for (auto it = m_userConnMap.begin(); it != m_userConnMap.end(); it++)
		{
			if (it->second == conn)
			{
				// 从map表删除用户信息
				user.setId(it->first);
				m_userConnMap.erase(it);
				break;
			}
		}
	}

	// 用户退出，相当于就是下线，在redis中取消订阅通道
	if(user.getId() != -1)
		m_redis.unsubscribe(user.getId());

	//更新用户的状态信息
	if(user.getId() != -1)
	{
		user.setState("offline");
		m_userModel.updateState(user);
	}
}

//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	int toId = js["toId"].get<int>();

	{
		lock_guard<mutex>lock(m_connMutex);
		auto it = m_userConnMap.find(toId);

		if (it != m_userConnMap.end())
		{
			//toId 在线，转发消息		服务器主动推送消息给toId用户
			it->second->send(js.dump());
			return;
		} // 有两种可能，一种是对方在其他主机上登录，一种是对方不在线
	}

	User user = m_userModel.query(toId);
	if (user.getId() == toId && user.getState() == "online")  //toId在线，但是不在当前主机上
	{
		m_redis.publish(toId, js.dump());
	}
	else
		//toId 不在线，存储离线消息
		m_offlineMsgModel.insert(toId, js.dump());
}

//添加好友业务 msgid   id   friendid
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	int userid = js["id"].get<int>();
	int friendid = js["friendid"].get<int>();

	// 存储好友信息
	m_friendModel.insert(userid, friendid);
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	int userid = js["id"].get<int>();
	string name = js["groupname"];
	string desc = js["groupdesc"];

	//存储新创建的群组信息
	Group group(-1, name, desc);  //还没有查询数据库，群id还不知道，随便填一个就行，groupmodel.createGroup不会使用这个字段，这个主键会在数据库中自动生成	if (m_groupModel.createGroup(group))
	if(m_groupModel.createGroup(group))
	{
		//存储群组创建人信息
		m_groupModel.addGroup(userid, group.getId(), "creator");
	}
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	int userid = js["id"].get<int>();
	int groupid = js["groupid"].get<int>();
	m_groupModel.addGroup(userid, groupid, "normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime)
{
	int userid = js["id"].get<int>();
	int groupid = js["groupid"].get<int>();
	vector<int> useridVec = m_groupModel.queryGroupsUsers(userid, groupid);

	lock_guard<mutex> lock(m_connMutex);
	for (int id : useridVec)
	{
		auto it = m_userConnMap.find(id);
		if (it != m_userConnMap.end())  // 找到了，说明用户当前在线，每个登录的用户，其conn都会insert进入connMap中
		{
			//转发群消息
			it->second->send(js.dump());
		}
		else
		{
			User user = m_userModel.query(id);
			if (user.getId() == id && user.getState() == "online") // 登录在其他主机
			{
				m_redis.publish(id, js.dump());
			}
			else
			{
				//存储离线群消息
				m_offlineMsgModel.insert(id, js.dump());
			}
		}
	}
}


void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{

	lock_guard<mutex> lock(m_connMutex);
	auto it = m_userConnMap.find(userid);

	LOG_INFO << "向其他服务器转发消息";

	if (it != m_userConnMap.end())
	{
		it->second->send(msg);
		return;
	}

	// 如果收到的订阅消息在主机上找不到对应的用户conn，可能用户在检查userid对应的User表项时，对方还在线，但等消息发过来的时候就下线了，这种情况应该存储到offlinemsg表中
	m_offlineMsgModel.insert(userid, msg);
}
