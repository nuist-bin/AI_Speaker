#include "socket.h"

int g_sockfd = 0;         //socket文件描述符

extern fd_set READSET;
extern int g_maxfd;
extern Node *head;
extern int g_mixerfd;
extern int g_start_flag;
extern int g_suspend_flag;

/*
函数描述：每隔5秒执行一次，向服务器上报数据（歌曲 音量 模式 设备号）
函数返回值：没有
函数参数：信号值
*/
void send_server(int sig)
{
	struct json_object *SendObj = json_object_new_object();
	json_object_object_add(SendObj, "cmd", json_object_new_string("info"));
	//获取当前播放的音乐
	Shm s;
	memset(&s, 0, sizeof(s));
	get_shm(&s);
	json_object_object_add(SendObj, "cur_music", json_object_new_string(s.cur_music));

	//播放模式
	json_object_object_add(SendObj, "mode", json_object_new_int(s.mode));

	//播放状态
	if (g_start_flag == 0)
	{
		json_object_object_add(SendObj, "status", json_object_new_string("stop"));
	}
	else if (g_start_flag == 1 && g_suspend_flag == 1)
	{
		json_object_object_add(SendObj, "status", json_object_new_string("suspend"));
	}
	else if (g_start_flag == 1 && g_suspend_flag == 0)
	{
		json_object_object_add(SendObj, "status", json_object_new_string("start"));
	}

	//获取当前系统的音量
	int volume;
	get_volume(&volume);
	json_object_object_add(SendObj, "volume", json_object_new_int(volume));

	json_object_object_add(SendObj, "deviceid", json_object_new_string("0001"));

	//发送到服务器
	socket_send_data(SendObj);

	json_object_put(SendObj);

	alarm(2);
}

/*
函数描述：向服务器发送json对象
发送格式：json对象的长度 ＋ json对象
函数参数：需要发送的对象
*/
void socket_send_data(struct json_object *j)
{
	char msg[1024] = {0};
	const char *s = json_object_to_json_string(j);
	int len = strlen(s);

	memcpy(msg, &len, sizeof(int));
	memcpy(msg + sizeof(int), s, len);

	if (send(g_sockfd, msg, len + 4, 0) == -1)
	{
		perror("send");
	}
}

void socket_recv_data(char *msg)
{
	//获取长度
	int len;
	ssize_t size = 0;
	char buf[1024] = {0};

	while (1)
	{
		size += recv(g_sockfd, buf + size, sizeof(int) - size, 0);
		if (size >= sizeof(int))
			break;
	}

	len = *(int *)buf;
	printf("-- LENGTH : %d ", len);
	size = 0;

	//获取数据
	while (1)
	{
		size += recv(g_sockfd, msg + size, len - size, 0);
		if (size >= len)
			break;
	}

	printf("MSG : %s\n", msg);
}

int init_socket()
{
	int count = 50;

	//创建socket
	g_sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == g_sockfd)
	{
		perror("socket");
		return -1;
	}

	struct sockaddr_in server_info;
	memset(&server_info, 0, sizeof(server_info));
	server_info.sin_family = PF_INET;
	server_info.sin_port = htons(PORT);
	server_info.sin_addr.s_addr = inet_addr(IP);

	int len = sizeof(server_info);

	//向服务器发起连接
	while (count--)
	{
		if (connect(g_sockfd, (struct sockaddr *)&server_info, len) == -1)
		{
			printf("CONNECT SERVER FAILURE .....\n");
			sleep(1);
			continue;
		}

#ifdef ARM
		//蜂鸣器0.5秒提示
		start_buzzer();
#endif

		//连接成功
		FD_SET(g_sockfd, &READSET);       //把网络加入集合
		g_maxfd = (g_maxfd < g_sockfd) ? g_sockfd : g_maxfd;

		//音箱2秒上传一次数据
		//alarm(2);                         //SIGALRM
		signal(SIGALRM, send_server);
		send_server(SIGALRM);

		break;
	}
}

/*每次更新完歌曲链表，都要通知APP*/
void upload_music_list()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("upload_music"));

	struct json_object *array = json_object_new_array();
	Node *p = head->next;
	while (p)
	{
		json_object_array_add(array, json_object_new_string(p->music_name));
		p = p->next;
	}

	json_object_object_add(obj, "music", array);

	socket_send_data(obj);

	json_object_put(obj);
	json_object_put(array);
}

void socket_start_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_start_reply"));

	if (start_play() == -1)
	{
		json_object_object_add(obj, "result", json_object_new_string("failure"));
	}
	else 
	{
		json_object_object_add(obj, "result", json_object_new_string("success"));
	}

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_stop_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_stop_reply"));

	stop_play();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_suspend_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_suspend_reply"));

	suspend_play();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_continue_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_continue_reply"));

	continue_play();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_prior_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_prior_reply"));

	prior_play();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	Shm s;
	get_shm(&s);
	json_object_object_add(obj, "music", json_object_new_string(s.cur_music));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_next_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_next_reply"));

	next_play();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	Shm s;
	get_shm(&s);
	json_object_object_add(obj, "music", json_object_new_string(s.cur_music));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_voice_up()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_voice_up_reply"));

	voice_up();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	int volume;
	if (ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, &volume) == -1)
	{
		perror("ioctl");
	}
	volume /= 257;
	json_object_object_add(obj, "voice", json_object_new_int(volume));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_voice_down()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_voice_down_reply"));

	voice_down();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	int volume;
	if (ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, &volume) == -1)
	{
		perror("ioctl");
	}
	volume /= 257;
	json_object_object_add(obj, "voice", json_object_new_int(volume));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_circle_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_circle_reply"));

	circle_play();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	socket_send_data(obj);

	json_object_put(obj);
}

void socket_sequence_play()
{
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("app_sequence_reply"));

	sequence_play();
	
	json_object_object_add(obj, "result", json_object_new_string("success"));

	socket_send_data(obj);

	json_object_put(obj);
}

