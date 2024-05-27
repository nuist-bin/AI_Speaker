#include "select.h"

extern fd_set READSET;
extern int g_maxfd;
extern int g_buttonfd;
extern int g_serialfd;
extern int g_sockfd;

void init_select()
{
	FD_ZERO(&READSET);       //初始化集合

	FD_SET(0, &READSET);     //把标准输入添加到集合中
}

void menu()
{
	printf("*********************************************\n");
	printf("       1.开始播放       2.结束播放\n");
	printf("       3.暂停播放       4.继续播放\n");
	printf("       5.下一首         6.上一首\n");
	printf("       7.增加音量       8.减小音量\n");
	printf("       9.单曲循环       a.顺序播放\n");
	printf("*********************************************\n");
}

/*读取键盘数据*/
void select_read_stdio()
{
	char ch;
	scanf("%c", &ch);

	switch (ch)
	{
		case '1':
			start_play();
			break;
		case '2':
			stop_play();
			break;
		case '3':
			suspend_play();
			break;
		case '4':
			continue_play();
			break;
		case '5':
			next_play();
			break;
		case '6':
			prior_play();
			break;
		case '7':
			voice_up();
			break;
		case '8':
			voice_down();
			break;
		case '9':
			circle_play();
			break;
		case 'a':
			sequence_play();
			break;
		case 'b':
			singer_play("其他");
			break;
	}
}

void select_read_button()
{
	int key = get_key_id();
	printf("读到按键 %d ...\n", key);

	switch(key)
	{
		case 1:
			start_play();
			break;
		case 2:
			stop_play();
			break;
		case 3:
			suspend_play();
			break;
		case 4:
			continue_play();
			break;
		case 5:
			next_play();
			break;
		case 6:
			prior_play();
			break;
	}
}

void select_read_serial()
{
	printf("-- 串口读到数据 --");

	char ch;
	if (read(g_serialfd, &ch, 1) == -1)
	{
		perror("read");
		return;
	}

	printf("%d\n", ch);

	switch(ch)
	{
		case 1:
			start_play();
			break;
		case 2:
			stop_play();
			break;
		case 3:
			suspend_play();
			break;
		case 4:
			continue_play();
			break;
		case 5:
			prior_play();
			break;
		case 6:
			next_play();
			break;
		case 7:
			voice_up();
			break;
		case 8:
			voice_down();
			break;
		case 9:
			circle_play();
			break;
		case 0x0a:
			sequence_play();
			break;
		case 0x0b:
			singer_play("周杰伦");
			break;
		case 0x0c:
			singer_play("许嵩");
			break;
		case 0x0d:
			singer_play("陈奕迅");
			break;
		case 0x0e:
			singer_play("五月天");
			break;
			
	}
}

int parse_message(char *msg, char *cmd)
{
	struct json_object *obj = (struct json_object *)json_tokener_parse(msg);
	if (NULL == obj)
	{
		printf("json_tokener_parse\n");
		return -1;
	}

	struct json_object *value;
	value = (struct json_object *)json_object_object_get(obj, "cmd");
	if (NULL == value)
	{
		printf("没有cmd字段\n");
		return -1;
	}

	strcpy(cmd, (const char *)json_object_get_string(value));
}

void select_read_socket()
{
	char buf[1024] = {0};
	char cmd[128] = {0};

	socket_recv_data(buf);         //   {"cmd":"start"}
	if (parse_message(buf, cmd) == -1)
	{
		printf("收到的不是json格式\n");
	}
	else
	{
		printf("cmd %s\n", cmd);
	}

	if (!strcmp(cmd, "app_start"))    //开始播放
	{
		socket_start_play();	
	}
	else if (!strcmp(cmd, "app_stop"))
	{
		socket_stop_play();
	}
	else if (!strcmp(cmd, "app_suspend"))
	{
		socket_suspend_play();
	}
	else if (!strcmp(cmd, "app_continue"))
	{
		socket_continue_play();
	}
	else if (!strcmp(cmd, "app_prior"))
	{
		socket_prior_play();
	}
	else if (!strcmp(cmd, "app_next"))
	{
		socket_next_play();
	}
	else if (!strcmp(cmd, "app_voice_up"))
	{
		socket_voice_up();
	}
	else if (!strcmp(cmd, "app_voice_down"))
	{
		socket_voice_down();
	}
	else if (!strcmp(cmd, "app_circle"))
	{
		socket_circle_play();
	}
	else if (!strcmp(cmd, "app_sequence"))
	{
		socket_sequence_play();
	}
	else if (!strcmp(cmd, "app_get_music"))
	{
		upload_music_list();
	}
}

void m_select()
{
	menu();
	fd_set tmpset;

	while (1)
	{
		tmpset = READSET;

		int ret = select(g_maxfd + 1, &tmpset, NULL, NULL, NULL);
		if (-1 == ret && errno == EINTR)
		{
			continue;
		}
		else if (-1 == ret && errno != EINTR)
		{
			perror("select");
			return;
		}

		if (FD_ISSET(0, &tmpset))                      //键盘
		{
			select_read_stdio();
		}
		else if (FD_ISSET(g_buttonfd, &tmpset))        //按键
		{
			select_read_button();	
		}
		else if (FD_ISSET(g_sockfd, &tmpset))          //socket
		{
			select_read_socket();
		}
		else if (FD_ISSET(g_serialfd, &tmpset))        //语音
		{
			select_read_serial();		
		}
	}
}
