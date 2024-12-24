#include<iostream>
#include"json.hpp"
#include<vector>
#include<map>
#include<string>
using json = nlohmann::json;
using namespace std;

//json序列化示例1
void func1()
{
	json js;
	js["msg_type"] = 2;
	js["from"] = "zhangsan";
	js["to"] = "li si";
	js["msg"] = "hello, what are you doing now";
	cout << js << endl;//底层使用了链式哈希表，是无序的容器。所以打印出来也是无序的
	string sendBuf = js.dump();
	cout << sendBuf.c_str() << endl;

}

void func2()
{
	json js;
	//添加数组
	js["id"] = { 1,2,3,4,5 };
	//添加key-value
	js["name"] = "zhang san";
	//添加对象
	js["msg"]["zhangsan"] = "hello world";
	js["msg"]["liu shuo"] = "hello china";
	//下面这一句等效上面两句
	js["msg"] = { {"zhang san","hello world"},{"liu shuo","hello china"} };

	cout << js << endl;
	cout << js.dump().c_str() << endl;
}

void func3()
{
	json js;

	//直接序列化一个vector容器
	vector<int>vec;
	vec.push_back(1);
	vec.push_back(2);
	vec.push_back(5);
	js["list"] = vec;

	//直接序列化一个map容器
	map<int, string>m;
	m.insert({ 1,"黄山" });
	m.insert({ 2,"华山" });
	m.insert({ 3,"泰山" });
	js["path"] = m;
	cout << js << endl;
	cout << js.dump().c_str() << endl;
}


//js数据的反序列化
void func4()
{
	//假设jsonstr是一个从网络接收到的字符串
	string recvBuf = "{\"list\":[1,2,5],\"path\":[[1,\"黄山\"],[2,\"华山\"],[3,\"泰山\"]]}";
	//反序列化
	json js2 = json::parse(recvBuf);

	//读取反序列化的结果
	vector<int>vec = js2["list"];
	for (auto num : vec)
	{
		cout << num<<" ";
	}
	cout << endl;

	map<int, string>mp = js2["path"];
	for (auto k : mp)
	{
		cout << k.first << " " << k.second << endl;
	}
}

//int main()
//{
//	func1();
//	func2();
//	func3();
//	func4();
//	return 0;
//}