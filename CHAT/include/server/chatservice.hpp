#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include<memory>
#include<mutex>
#include"json.hpp"
#include"usermodel.hpp"
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"
using json = nlohmann::json;
using namespace std;
using namespace muduo;
using namespace muduo::net;


//业务代码

// 表示处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr& conn,json& js,Timestamp receiveTime)>;

// 聊天服务器业务类  单例模式
class ChatService {
public:
	//获取单例对象的接口函数
	static ChatService* instance();

	//处理登录业务，对应的业务id为LOGIN_MSG
	void login(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	//处理注册业务，对应的业务id为REG_MSG
	void reg(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	//一对一聊天业务
	void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	//添加好友业务
	void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	//创建群组业务
	void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	//加入群组业务
	void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	//群组聊天业务
	void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	// 处理注销业务
	void loginout(const TcpConnectionPtr& conn, json& js, Timestamp receiveTime);

	//处理客户端异常退出：将用户的 id-连接 键值对 从m_userConnMap中删除；修改用户在数据库中的状态信息。
	void clientCloseException(const TcpConnectionPtr& conn);

	// 处理订阅的redis通道传来的信息
	void handleRedisSubscribeMessage(int userid, string msg);

	//服务器异常，业务重置方法
	void reset();

	//获取消息对应的处理器Handler
	MsgHandler getHandler(int msgid);


private:
	ChatService();

	// 存储消息id和其对应的业务处理方法
	unordered_map<int, MsgHandler> msgHanlderMap;

	// 存储在线用户的通信连接  用户id-连接
	unordered_map<int, TcpConnectionPtr> m_userConnMap;

	// 定义互斥锁，保证m_userConnMap的线程安全
	mutex m_connMutex;


	// model层主要是对设计数据库的操作进行了封装
	// 用户数据操作类对象
	UserModel m_userModel;


	// 离线数据表操作对象
	offlineMsgModel m_offlineMsgModel;


	// 好友数据表操作对象
	FriendModel m_friendModel;


	// 群组数据表操作对象
	GroupModel m_groupModel;

	// redis操作对象
	Redis m_redis;
};



#endif
