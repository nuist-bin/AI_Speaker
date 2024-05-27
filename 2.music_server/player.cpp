#include "player.h"
#include "server.h"

Player::Player()
{
	info = new std::list<PlayerInfo>;
}

Player::~Player()
{
	if (info)
		delete info;
}

void Player::player_update_list(struct bufferevent *bev, Json::Value &v, Server *ser)
{
	auto it = info->begin();

	//遍历链表，如果设备存在，更新链表并且转发给APP
	for (; it != info->end(); it++)
	{
		if (it->deviceid == v["deviceid"].asString()) //找到了对应的结点
		{
			debug("设备已经存在，更新链表信息");
			it->cur_music = v["cur_music"].asString();
			it->volume = v["volume"].asInt();
			it->mode = v["mode"].asInt();
			it->d_time = time(NULL);

			if (it->a_bev)
			{
				debug("APP在线，数据转发给APP");
				ser->server_send_data(it->a_bev, v);
			}

			return;
		}
	}

	//如果设备不存在，新建结点
	PlayerInfo p;
	p.deviceid = v["deviceid"].asString();
	p.cur_music = v["cur_music"].asString();
	p.volume = v["volume"].asInt();
	p.mode = v["mode"].asInt();
	p.d_time = time(NULL);
	p.d_bev = bev;
	p.a_bev = NULL;

	info->push_back(p);

	debug("第一次上报数据，结点已经建立");
}

void Player::player_app_update_list(struct bufferevent *bev, Json::Value &v)
{
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (it->deviceid == v["deviceid"].asString())
		{
			it->a_time = time(NULL);
			it->appid = v["appid"].asString();
			it->a_bev = bev;
		}
	}
}

void Player::player_traverse_list()
{
	debug("定时器事件：遍历链表");

	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (time(NULL) - it->d_time > 6)  //超过3次，音箱离线
		{
			info->erase(it);
		}

		if (it->a_bev)
		{
			if (time(NULL) - it->a_time > 6)
			{
				it->a_bev = NULL;
			}
		}
	}
}

void Player::debug(const char *s)
{
	time_t cur = time(NULL);
	struct tm *t = localtime(&cur);

	std::cout << "[ " << t->tm_hour << " : " << t->tm_min << " : ";
	std::cout << t->tm_sec << " ] " << s << std::endl;
}

void Player::player_upload_music(Server *ser, struct bufferevent *bev, Json::Value &v)
{
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (it->d_bev == bev)
		{
			if (it->a_bev != NULL)    //app在线
			{
				ser->server_send_data(it->a_bev, v);
				debug("APP在线 歌曲名字转发成功");
			}
			else
			{
				debug("APP不在线 歌曲名字不转发");
			}
			break;
		}
	}
}

void Player::player_option(Server *ser, struct bufferevent *bev, Json::Value &v)
{
	//判断音箱是否在线
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (it->a_bev == bev)
		{
			ser->server_send_data(it->d_bev, v);
			debug("音箱在线，指令转发成功");
			return;
		}
	}

	//音箱不在线
	Json::Value value;
	std::string cmd = v["cmd"].asString();
	cmd += "_reply";
	value["cmd"] = cmd;
	value["result"] = "offline";

	ser->server_send_data(bev, value);

	debug("音箱不在线，转发失败");
}

void Player::player_reply_option(Server *ser, struct bufferevent *bev, Json::Value &v)
{
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (it->d_bev == bev)
		{
			if (it->a_bev)
			{
				ser->server_send_data(it->a_bev, v);
				break;
			}
		}
	}
}

void Player::player_offline(struct bufferevent *bev)
{
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (it->d_bev == bev)
		{
			debug("音箱异常下线处理");
			info->erase(it);
			break;
		}

		if (it->a_bev == bev)
		{
			debug("APP异常下线处理");
			it->a_bev = NULL;
			break;
		}
	}
}

void Player::player_app_offline(struct bufferevent *bev)
{
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (it->a_bev == bev)
		{
			debug("APP正常下线处理");
			it->a_bev = NULL;
			break;
		}
	}
}

void Player::player_get_music(Server *ser, struct bufferevent *bev, Json::Value &v)
{
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if (it->a_bev == bev)
		{
			ser->server_send_data(it->d_bev, v);
		}
	}
}
