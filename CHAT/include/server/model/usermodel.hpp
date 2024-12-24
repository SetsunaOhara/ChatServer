#ifndef USERMODE_H
#define USERMODE_H

#include"user.hpp"

//User表的数据操作类
//对数据库表的增删改查操作 抽象出一个类
class UserModel
{
public:
	// User表的增加方法
	bool insert(User& user);

	// 根据 用户id(主键) 查询用户信息，并返回一个User对象
	User query(int id);

	//更新用户的状态信息
	bool updateState(User user);

	//重置用户的状态信息
	void resetState();

};

#endif // !USERMODE_H
