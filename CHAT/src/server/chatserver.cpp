#include"chatserver.hpp"
#include<functional>
#include<string>
#include"json.hpp"
#include"chatservice.hpp"
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

//网络模块代码

//初始化聊天服务器对象
ChatServer::ChatServer(EventLoop* loop,
	const InetAddress& listenAddr,
	const string& nameArg):m_server(loop,listenAddr,nameArg),m_loop(loop)
{
	// 注册连接回调
	m_server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));//把回调函数绑定到对象上，然后传给m_server

	//注册消息回调
	m_server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

	//设置线程数量
	m_server.setThreadNum(8);
}


//启动服务
void ChatServer::start()
{
	m_server.start();
}


//如何把网络模块发来的消息派发到业务模块，实现业务模块与网络模块完全解耦？
// 
// 上报连接建立和断开相关信息的回调函数
void  ChatServer::onConnection(const TcpConnectionPtr& conn) {
	// 客户端断开连接，释放socket资源
	if (!conn->connected())
	{
		ChatService::instance()->clientCloseException(conn);
		conn->shutdown();
	}
}

// 上报读写事件相关信息的回调函数
void  ChatServer::onMessage(const TcpConnectionPtr& conn, //连接
	Buffer* buffer,         //缓冲区
	Timestamp receiveTime)  //接受到数据的时间信息
{
	string buf = buffer->retrieveAllAsString();
	//数据的反序列化
	json js = json::parse(buf);//这的buf都有一个业务标识 msgid

	//通过js["msgid"] 获取 =》业务handler =》 可以传入  conn    js    time
	//达到的目的：完全解耦网络模块的代码和业务模块的代码


	auto chatMsgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());

	//回调消息绑定好的事件处理器，来执行相应 的 业务处理
	chatMsgHandler(conn, js, receiveTime);
}




