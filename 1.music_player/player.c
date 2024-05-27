#include "player.h"

int g_shmid = 0;          //共享内存句柄
int g_start_flag = 0;     //开始播放标志位
int g_suspend_flag = 0;   //暂停播放标志位

extern int g_mixerfd;
extern Node *head;

int init_shm()
{
	//创建共享内存
	g_shmid = shmget(SHMKEY, SHMSIZE, IPC_CREAT | IPC_EXCL);
	if (-1 == g_shmid)
	{
		perror("shmget");
		return -1;
	}

	//映射
	void *addr = shmat(g_shmid, NULL, 0);
	if ((void *)-1 == addr)
	{
		perror("shmat");
		return -1;
	}

	Shm s;
	memset(&s, 0, sizeof(s));
	s.parent_pid = getpid();
	s.mode = SEQUENCE;
	memcpy(addr, &s, sizeof(s));         //把数据写入共享内存

	shmdt(addr);                         //解除映射
}

void get_shm(Shm *s)
{
	void *addr = shmat(g_shmid, NULL, 0);
	if ((void *)-1 == addr)
	{
		perror("shmat");
		return;
	}

	memcpy(s, addr, sizeof(Shm));

	shmdt(addr);
}

void set_shm(Shm s)
{
	void *addr = shmat(g_shmid, NULL, 0);
	if ((void *)-1 == addr)
	{
		perror("shmat");
		return;
	}

	memcpy(addr, &s, sizeof(Shm));

	shmdt(addr);
}


void get_volume(int *v)
{
	//获取音量大小
	if (ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, v) == -1)
	{
		perror("ioctl");
		return;
	}

	*v /= 257;
}

void get_music(const char *singer)
{
	//发送请求
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("get_music_list"));
	json_object_object_add(obj, "singer", json_object_new_string(singer));

	socket_send_data(obj);

	char msg[1024] = {0};
	socket_recv_data(msg);

	//形成链表
	create_link(msg);

	//上传音乐数据
	upload_music_list();
}

int start_play()
{
	if (g_start_flag == 1)     //已经开始播放
		return -1;

	if (head->next == NULL)
		return -1;

	char name[32] = {0};
	strcpy(name, head->next->music_name);

	//初始化音量
	int volume = DFL_VOL;
	volume *= 257;
	ioctl(g_mixerfd, SOUND_MIXER_WRITE_VOLUME, &volume);

	g_start_flag = 1;

	play_music(name);

	return 0;
}

/*子进程收到SIGUSR2信号，触发该函数*/
void child_quit()
{
	g_start_flag = 0;
}

void play_music(char *n)
{
	pid_t child_pid = fork();
	if (-1 == child_pid)
	{
		perror("fork");
		return;
	}
	else if (0 == child_pid)         //子进程
	{
		close(0);                    //关闭标准输入
		signal(SIGUSR2, child_quit);
		child_process(n);
		exit(0);
	}
	else 
	{
		return;
	}
}

/*子进程：
1、创建孙进程，调用mplayer播放音乐；
2、等待孙进程结束
*/
void child_process(char *n)
{
	while (g_start_flag)
	{
		pid_t grand_pid = fork();
		if (-1 == grand_pid)
		{
			perror("fork");
			return;
		}
		else if (0 == grand_pid)     //孙进程
		{
			close(0);                //关闭标准输入

			Shm s;
			memset(&s, 0, sizeof(s));

			if (strlen(n) == 0)      //第二次进入循环（自动播放下一首）
			{
				grand_get_shm(&s);

				if (find_next_music(s.cur_music, s.mode, n) == -1)
				{
					//歌曲播放完了  通知父进程和子进程
					kill(s.parent_pid, SIGUSR1);
					kill(s.child_pid, SIGUSR2);
					usleep(100000);
					exit(0);
				}
			}

			char *arg[7] = {0};
			char music_path[128] = {0};

			strcpy(music_path, URL);
			strcat(music_path, n);

			arg[0] = "mplayer";
			arg[1] = music_path;
			arg[2] = "-slave";
			arg[3] = "-quiet";
			arg[4] = "-input";
			arg[5] = "file=./cmd_fifo";
			arg[6] = NULL;

			//修改共享内存
			grand_get_shm(&s);
			char *p = n;                    //去除歌手名字
			while (*p != '/')
				p++;
			strcpy(s.cur_music, p + 1);
			grand_set_shm(s);

#ifdef ARM
			execv("/bin/mplayer", arg);
#else
			execv("/usr/local/bin/mplayer", arg);
#endif

		}
		else                         //子进程 
		{
			memset(n, 0, sizeof(n));

			int status;
			wait(&status);

			usleep(100000);
		}
	}
}

void grand_set_shm(Shm s)
{
	//修改共享内存（子进程 孙进程 id   音乐名字）
	int shmid = shmget(SHMKEY, SHMSIZE, 0);
	if (-1 == shmid)
	{
		perror("grand shmget");
		return;
	}

	void *addr = shmat(shmid, NULL, 0);
	if ((void *)-1 == addr)
	{
		perror("grand shmat");
		return;
	}

	s.child_pid = getppid();
	s.grand_pid = getpid();

	memcpy(addr, &s, sizeof(s));
	
	shmdt(addr);
}

void grand_get_shm(Shm *s)
{
	int shmid = shmget(SHMKEY, SHMSIZE, 0);
	if (-1 == shmid)
	{
		perror("grand shmget");
		return;
	}

	void *addr = shmat(shmid, NULL, 0);
	if ((void *)-1 == addr)
	{
		perror("grand shmat");
		return;
	}
	
	memcpy(s, addr, sizeof(Shm));

	shmdt(addr);
}

void write_fifo(const char *cmd)
{
	int fd = open("cmd_fifo", O_WRONLY);
	if (-1 == fd)
	{
		perror("fifo open");
		return;
	}

	if (write(fd, cmd, strlen(cmd)) == -1)
	{
		perror("fifo write");
	}

	close(fd);
}

void stop_play()
{
	if (g_start_flag == 0)
		return;
		
	//通知子进程
	Shm s;
	get_shm(&s);
	kill(s.child_pid, SIGUSR2);

	//结束mplayer进程
	//write_fifo("stop\n");
	write_fifo("quit\n");

	//回收子进程
	int status;
	waitpid(s.child_pid, &status, 0);
	printf("回收完成\n");

	g_start_flag = 0;
}

void suspend_play()
{
	if (g_start_flag == 0 || g_suspend_flag == 1)
		return;

	printf("-- 暂停播放 --\n");

	write_fifo("pause\n");

	g_suspend_flag = 1;
}

void continue_play()
{
	if (g_start_flag == 0 || g_suspend_flag == 0)
		return;

	printf("-- 继续播放 --\n");

	write_fifo("pause\n");

	g_suspend_flag = 0;
}

void next_play()
{
	if (g_start_flag == 0)
		return;

	Shm s;
	get_shm(&s);
	char music[128] = {0};
	if (find_next_music(s.cur_music, SEQUENCE, music) == -1)
	{
		//链表里面的歌曲播放完了
		stop_play();

		char singer[128] = {0};
		get_singer(singer);

		clear_link();
		get_music(singer);
		sleep(1);

		start_play();

		if (g_suspend_flag == 1)
			g_suspend_flag = 0;

		return;
	}

	char path[256] = {0};
	strcat(path, URL);
	strcat(path, music);

	char cmd[256] = {0};
	sprintf(cmd, "loadfile %s\n", path);

	write_fifo(cmd);

	//更新共享内存
	int i;
	for (i = 0; i < sizeof(music); i++)
	{
		if (music[i] == '/')
			break;
	}
	strcpy(s.cur_music, music + i + 1);
	set_shm(s);

	if (g_suspend_flag == 1)
		g_suspend_flag = 0;
}

void prior_play()
{
	if (g_start_flag == 0)
		return;

	Shm s;
	get_shm(&s);
	char music[128] = {0};
	find_prior_music(s.cur_music, music);

	char path[256] = {0};
	strcat(path, URL);
	strcat(path, music);

	char cmd[256] = {0};
	sprintf(cmd, "loadfile %s\n", path);

	write_fifo(cmd);

	//更新共享内存
	int i;
	for (i = 0; i < sizeof(music); i++)
	{
		if (music[i] == '/')
			break;
	}
	strcpy(s.cur_music, music + i + 1);
	set_shm(s);

	if (g_suspend_flag == 1)
		g_suspend_flag = 0;
}

void voice_up()
{
	int volume;
	//获取音量大小
	if (ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, &volume) == -1)
	{
		perror("ioctl");
		return;
	}

	volume /= 257;

	if (volume <= 95)
	{
		volume += 5;
	}
	else if (volume > 95 && volume < 100)
	{
		volume = 100;
	}
	else if (volume == 100)
	{
		printf("音量已经最大 ...\n");
		return;
	}

	volume *= 257;

	if (ioctl(g_mixerfd, SOUND_MIXER_WRITE_VOLUME, &volume) == -1)
	{
		perror("ioctl");
		return;
	}

	printf("-- 增加音量 --\n");
}

void voice_down()
{
	int volume;
	//获取音量大小
	if (ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, &volume) == -1)
	{
		perror("ioctl");
		return;
	}

	volume /= 257;

	if (volume >= 5)
	{
		volume -= 5;
	}
	else if (volume > 0 && volume < 5)
	{
		volume = 0;
	}
	else if (volume == 0)
	{
		printf("音量已经最小 ...\n");
		return;
	}

	volume *= 257;

	if (ioctl(g_mixerfd, SOUND_MIXER_WRITE_VOLUME, &volume) == -1)
	{
		perror("ioctl");
		return;
	}

	printf("-- 减小音量 --\n");
}

void circle_play()
{	
	Shm s;
	get_shm(&s);
	s.mode = CIRCLE;
	set_shm(s);
	printf("-- 单曲循环 --\n");
}

void sequence_play()
{	
	Shm s;
	get_shm(&s);
	s.mode = SEQUENCE;
	set_shm(s);
	printf("-- 顺序播放 --\n");
}

void singer_play(const char *singer)
{
	printf("停止\n");
	//停止播放
	stop_play();

	printf("清空\n");
	//清空链表
	clear_link();

	printf("获取\n");
	//获取歌曲
	get_music(singer);

	printf("开始\n");
	//开始播放
	start_play();
}
