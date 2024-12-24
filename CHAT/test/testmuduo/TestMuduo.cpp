#include<iostream>
#include<string>
#include<functional>
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*
muduo网络库给用户提供了两个主要的类
TcpServer:用于编写服务器程序的
TcpClient:用于编写客户端程序的

epoll+线程池
好处：能够把网络IO代码和业务代码区分开
						用户的连接和断开	用户的可读写事件
*/

/*
基于muduo网络库开发服务器程序
1、组合TcpServer对象
2、创建EventLoop事件循环对象的指针
3、明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4、在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5、设置合适的服务端线程数量，muduo会自己划分I/O线程和worker线程
*/
class ChatServer {
public:
	ChatServer(EventLoop* loop, //事件循环
		const InetAddress& listenAddr, //IP+Port
		const string& nameArg) //服务器的名字
		:m_server(loop, listenAddr, nameArg), m_loop(loop)
	{

		// 给服务器注册用户连接的创建和断开回调
		m_server.setConnectionCallback(bind(&ChatServer::onConnection,this,_1));
		// setConnection接受一个回调函数ConnectionCallback类型的参数；bind则返回一个回调函数
		// ConnectionCallback = void (const TcpConnectionPtr& ) = onConnection


		//给服务器注册用户读写事件回调
		m_server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

		//设定服务器端的线程数量
		m_server.setThreadNum(8); //1个IO线程，7个worker线程
	}

	//开启事件循环
	void start()
	{
		m_server.start();
	}

private:
	//专门处理用户的连接和断开  epoll  listenfd  accept 
	void onConnection(const TcpConnectionPtr& conn)    // 它的类型是ConnectionCallback = void (const TcpConnectionPtr&)
	{
		if(conn->connected()) //conn->connected()返回true，表示此次调用 是 连接成功
		{
			cout << conn->peerAddress().toIpPort() << " -> " <<
				conn->localAddress().toIpPort()<<" state:online" << endl;
		}
		else       // 否则表示 此次调用 是有客户端 断开连接
		{
			cout << conn->peerAddress().toIpPort() << " -> " <<
				conn->localAddress().toIpPort() << " state:offline" << endl;
			conn->shutdown(); //close(fd); 回收fd资源
			// m_loop->quit();
		}
	}

	//专门处理用户的读写事件
	void onMessage(const TcpConnectionPtr& conn, //连接
		Buffer* buffer,  //缓冲区
		Timestamp receiveTime)  //接受到数据的时间信息
	{
		string buf = buffer->retrieveAllAsString();//把所接收的数据全部放到字符串当中
		cout << " recv data:" << buf << " time:" << receiveTime.toString() << endl;
		conn->send(buf);
	}
	TcpServer m_server;// #1
	EventLoop* m_loop; // #2 epoll 
};



void print(const string& s1, const string& s2) {
	cout << s1 << " " << s2 << endl;
}

int add(int x, int y) { return x + y; }


//int main()
//{
//	//string str1 = "hello";
//	//string str2 = "world";
//
//	//function<void(const string&)> f = bind(print, placeholders::_1, str2);//placeholders::_1是一个占位符表示print的第一个参数保留，等待调用时传入
//	//function<void(const string&)> f1 = bind(print, "bcd", placeholders::_1);//放在第二个位置表示print的第二个参数保留，等待调用时传入
//	//f(str1);
//	//f1("def");
//	//return 0;
//
//
//	EventLoop loop; //epoll
//	InetAddress addr("192.168.17.131", 9889);
//	ChatServer server(&loop, addr, "ChatServer");
//
//	server.start();  // 启动服务：将listenfd 通过 epoll_ctl 添加到 epoll 上 
//	loop.loop();     // epoll_wait以阻塞的方式等待新用户连接/已连接用户的读写事件
//
//	return 0;
//}
