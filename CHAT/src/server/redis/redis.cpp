#include"redis.hpp"
#include<iostream>

//// 订阅通道
//void Redis::subscribe(int channel)
//{
//	// 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
//	if (REDIS_ERR == redisAppendCommand(this->_context, "SUBSCRIBE %d", channel))
//	{
//		LOG_ERROR << "subscribe [" << channel << "] error!";
//		return;
//	}
//	// redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
//	int done = 0;
//	while (!done)
//	{
//		if (REDIS_ERR == redisBufferWrite(this->_context, &done))
//		{
//			LOG_ERROR << "subscribe [" << channel << "] error!";
//			return;
//		}
//	}
//	LOG_INFO << "subscribe [" << channel << "] success!";
//}


Redis::Redis():m_publish_context(nullptr),m_subscribe_context(nullptr)
{
}


Redis::~Redis()
{
	if (m_publish_context != nullptr)
	{
		redisFree(m_publish_context);
	}
	if (m_subscribe_context != nullptr)
	{
		redisFree(m_subscribe_context);
	}
}


bool Redis::connect()
{
	// 先建立两个上下文

	// 负责publish发布消息的上下文连接
	m_publish_context = redisConnect("127.0.0.1", 6379);
	if (m_publish_context == nullptr || m_publish_context->err)
	{
		cerr << "connect redis failed" << endl; 
		return false;
	}


	// 负责subscribe订阅消息的上下文连接
	m_subscribe_context = redisConnect("127.0.0.1", 6379);
	if (m_subscribe_context == nullptr || m_subscribe_context->err)
	{
		cerr << "connect redis failed" << endl;
		return false;
	}

	// 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
	thread t([&] {
		observer_channel_message();
		});
	t.detach();

	cout << "connect redis-server success!" << endl;

	return true;
}


// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
	redisReply* reply = (redisReply*)redisCommand(m_publish_context, "PUBLISH %d %s", channel, message.c_str());
	if (reply == nullptr)
	{
		cerr << "publish command failed!" << endl;
		return false;
	}
	freeReplyObject(reply);
	return true;
}


//// 向redis指定的通道subscribe订阅消息
//bool Redis::subscribe(int channel)
//{
//	// SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅消息，不接收通道消息
//	// 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
//	// 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢相应资源
//
//	if (REDIS_ERR == redisAppendCommand(this->m_subscribe_context, "SUBSCRIBE %d", channel))
//	{
//		cerr << "subscribe redisAppendCommand command failed!" << endl;
//		return false;
//	}
//	// redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
//	int done = 0;
//	while (!done)
//	{
//		if (REDIS_ERR == redisBufferWrite(this->m_subscribe_context, &done))
//		{
//			cerr << "subscribe redisBufferWrite command failed!" << endl;
//			return false;
//		}
//	}
//	// redisGetReply
//
//	return true;
//}


//// 向redis指定的通道unsubscribe取消订阅消息
//bool Redis::unsubscribe(int channel)
//{
//	if (REDIS_ERR == redisAppendCommand(this->m_subscribe_context, "UNSUBSCRIBE %d", channel))
//	{
//		cerr << "unsubscribe redisAppendCommand command failed!" << endl;
//		return false;
//	}
//	// redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
//	int done = 0;
//	while (!done)
//	{
//		if (REDIS_ERR == redisBufferWrite(this->m_subscribe_context, &done))
//		{ 
//			cerr << "unsubscribe redisBufferWrite command failed!" << endl;
//			return false;
//		}
//	}
//	return true;
//}
// 



// 向Redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
	// redisCommand 会先把命令缓存到context中，然后调用RedisAppendCommand发送给redis
	// redis执行subscribe是阻塞，不会响应，不会给我们一个reply
	// redis 127.0.0.1:6379> SUBSCRIBE runoobChat
	if (REDIS_ERR == redisAppendCommand(m_subscribe_context, "SUBSCRIBE %d", channel))
	{
		cerr << "subscibe redisAppendCommand command failed" << endl;
		return false;
	}

	int done = 0;
	while (!done)
	{
		if (REDIS_ERR == redisBufferWrite(m_subscribe_context, &done))
		{
			cerr << "subscribe redisBufferWrite command failed" << endl;
			return false;
		}
	}

	//redisReply* reply = (redisReply*)redisCommand(m_subscribe_context, "SUBSCRIBE %d", channel);
	//if(reply == nullptr){
	//	cerr << "subscibe redisCommand command failed" << endl;
	//	return false;
	//}
	//freeReplyObject(reply);

	return true;
}


//取消订阅
bool Redis::unsubscribe(int channel)
{
	//redisCommand 会先把命令缓存到context中，然后调用RedisAppendCommand发送给redis
	//redis执行subscribe是阻塞，不会响应，不会给我们一个reply
	if (REDIS_ERR == redisAppendCommand(m_subscribe_context, "UNSUBSCRIBE %d", channel))
	{
		cerr << "unsubscibe redisAppendCommand command failed" << endl;
		return false;
	}

	int done = 0;
	while (!done)
	{
		if (REDIS_ERR == redisBufferWrite(m_subscribe_context, &done))
		{
			cerr << "unsubscribe redisBufferWrite command failed" << endl;
			return false;
		}
	}

	return true;
}



// 在独立线程中接收订阅通道中的消息 ---它在connect中创建的独立线程中调用
void Redis::observer_channel_message()
{
	redisReply* reply = nullptr;
	while (REDIS_OK == redisGetReply(this->m_subscribe_context, (void**)&reply))
	{
		// 订阅收到的消息是一个带三元素的数组 element[1]就是订阅的通道号，element[2]就是接收的消息
		if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
		{
			// 调用回调函数，给业务层上报通道上发生的消息 
			notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
		}
		freeReplyObject(reply);
	}
	cerr << ">>>>>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int, string)>fn)
{
	this->notify_message_handler = fn;
}




