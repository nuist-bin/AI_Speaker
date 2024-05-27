#include "database.h"

DataBase::DataBase()
{

}

DataBase::~DataBase()
{

}

bool DataBase::database_connect()
{
	//初始化数据库句柄
	mysql = mysql_init(NULL);

	//向数据库发起连接
	mysql = mysql_real_connect(mysql, "localhost", "root", "root", 
			"musicplayer", 0, NULL, 0);
	if (mysql == NULL)
	{
		std::cout << "[DATABASE CONNECT FAILURE] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	/*if (mysql_query(mysql, "set names utf8;") != 0)
	{
		
	}*/

	return true;
}

void DataBase::database_disconnect()
{
	mysql_close(mysql);
}

bool DataBase::database_init_table()
{
	if (!this->database_connect())
		return false;

	const char *sql = "create table if not exists account(appid char(11),password varchar(16), deviceid varchar(8))charset utf8;";

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "[MYSQL QUERY ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	this->database_disconnect();

	return true;
}

bool DataBase::database_user_exist(std::string appid)
{
	char sql[256] = {0};
	sprintf(sql, "select * from account where appid = '%s';", appid.c_str());

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "[MYSQL QUERY ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return true;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res)
	{
		std::cout << "[MYSQL STORE RESULT ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return true;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (row == NULL)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void DataBase::database_add_user(std::string a, std::string p)
{
	char sql[128] = {0};
	sprintf(sql, "insert into account (appid, password) values ('%s', '%s');", a.c_str(), p.c_str());

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "[MYSQL QUERY ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
	}
}

bool DataBase::database_password_correct(std::string a, std::string p)
{
	char sql[256] = {0};
	sprintf(sql, "select password from account where appid = '%s';", a.c_str());

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "[MYSQL QUERY ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res)
	{
		std::cout << "[MYSQL STORE RESULT ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (NULL == row)
	{
		std::cout << "[MYSQL FETCH ROW ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	if (!strcmp(p.c_str(), row[0]))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DataBase::database_user_bind(std::string a, std::string &d)
{
	char sql[256] = {0};
	sprintf(sql, "select deviceid from account where appid = '%s';", a.c_str());

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "[MYSQL QUERY ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	MYSQL_RES *res = mysql_store_result(mysql);
	if (NULL == res)
	{
		std::cout << "[MYSQL STORE RESULT ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	if (NULL == row)
	{
		std::cout << "[MYSQL FETCH ROW ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
		return false;
	}

	if (NULL == row[0])
	{
		return false;
	}
	else
	{
		d = std::string(row[0]);
		return true;
	}
}

void DataBase::database_bind_user(std::string a, std::string d)
{
	char sql[256] = {0};
	sprintf(sql, "update account set deviceid = '%s' where appid = '%s';", d.c_str(), a.c_str());

	if (mysql_query(mysql, sql) != 0)
	{
		std::cout << "[MYSQL QUERY ERROR] ";
		std::cout << mysql_error(mysql) << std::endl;
	}
}
