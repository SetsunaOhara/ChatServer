#include"offlinemessagemodel.hpp"
#include <db/db.h>


//存储用户的离线消息
void offlineMsgModel::insert(int userId, string msg) {
	// 1、组装SQL语句
	char sql[1024] = { 0 };
	sprintf(sql, "insert into OfflineMessage(userid,message) values(%d,'%s')", userId, msg.c_str());

	MySQL mysql;
	if (mysql.connect())
	{
		mysql.update(sql);
	}
}


//删除用户的离线消息
void offlineMsgModel::remove(int userId) {
	// 1、组装SQL语句
	char sql[1024] = { 0 };
	sprintf(sql, "delete from OfflineMessage where userid=%d", userId);

	MySQL mysql;
	if (mysql.connect())
	{
		mysql.update(sql);
	}
}

//查询用户的离线消息
vector<string> offlineMsgModel::query(int userId) {
	// 1、组装SQL语句
	char sql[1024] = { 0 };
	sprintf(sql, "select message from OfflineMessage where userid=%d", userId);

	vector<string>vec;
	MySQL mysql;
	if (mysql.connect())
	{
		MYSQL_RES* res = mysql.query(sql);   //这里返回了指针，说明肯定开辟了内存，事后要手动释放内存
		if (res != nullptr)
		{
			MYSQL_ROW row;
			// 把多行数据 即userid用户的所有离线消息放入vec中返回
			while ((row = mysql_fetch_row(res)) != nullptr )
			{
				vec.push_back(row[0]);
			}

			mysql_free_result(res);
		}
	}
	return vec;
}