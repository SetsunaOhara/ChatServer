# ChatServer
基于muduo网络库的集群聊天服务器
Microsoft Visual Studio 2023远程linux开发，cmake构建

可以工作在nginx tcp负载均衡服务器上的客户端、服务端源码

使用了redis作为消息队列


编译方式：

	cd CHAT/build

	rm -rf *

	cmake ..

	make
