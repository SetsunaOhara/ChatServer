#ifndef REDIS_H
#define REDIS_H


#include"hiredis/hiredis.h"
#include<string>
#include<thread>
#include<functional>
using namespace std;

class Redis
{
public:
	Redis();
	~Redis();

	// 连接redis服务器
	bool connect();

	// 向redis指定的通道channel发布消息
	bool publish(int channel, string message);

	// 向redis指定的通道subscribe订阅消息
	bool subscribe(int channel);

	// 向redis指定的通道unsubscribe订阅消息
	bool unsubscribe(int channel);

	// 向独立线程中接收订阅通道中的消息
	void observer_channel_message();

	// 初始化向业务层上报通道消息的回调对象
	void init_notify_handler(function<void(int, string)>fn);

private:
	// hiredis同步上下文对象，负责publish消息
	redisContext* m_publish_context;

	// hiredis同步上下文对象，负责subscrib消息
	redisContext* m_subscribe_context;

	//一个上下文(redisContext)相当于一个redis-cli，在linux中，我们可以知道，一个redis-cli中一旦执行了subscribe就会阻塞，因此无法再执行publish和其他任何操作，想要一边订阅一边发布消息，需要开两个上下文

	// 回调操作，收到订阅的消息，给service层上报
	function<void(int, string)> notify_message_handler;
};
#endif // !REDIS_H