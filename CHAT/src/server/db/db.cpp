#include"db/db.h"


// 初始化数据库连接
MySQL::MySQL()
{
	m_conn = mysql_init(nullptr);
}
// 释放数据库连接资源这里用UserModel示例，通过UserModel如何对业务层封装底层数据库的操作。代码示例如下：
MySQL::~MySQL()
{
	if (m_conn != nullptr)
		mysql_close(m_conn);
}
// 连接数据库
bool MySQL:: connect()
{
	MYSQL* p = mysql_real_connect(m_conn, server.c_str(), user.c_str(),
		password.c_str(), dbname.c_str(), 3306, nullptr, 0);
	if (p != nullptr)
	{
		//C/C++代码默认的编码字符是ASCII码，如果不设置，从MySQL上拉下来的中文显示乱码
		mysql_query(m_conn, "set names gbk");
		LOG_INFO << "connect mysql success!";
	}
	else
	{
		LOG_INFO << "connect mysql error!";

	}
	return p;
}
// 更新操作
bool MySQL::update(string sql)
{
	if (mysql_query(m_conn, sql.c_str()))
	{
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
			<< sql << "更新失败!";
		return false;
	}
	return true;
}
// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
	if (mysql_query(m_conn, sql.c_str()))
	{
		LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
			<< sql << "查询失败!";
		return nullptr;
	}
	return mysql_use_result(m_conn);
}


//获取连接
MYSQL* MySQL::getConnection()
{
	return m_conn;
}
