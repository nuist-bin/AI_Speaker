#ifndef PLAYER_H
#define PLAYER_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <linux/soundcard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "socket.h"
#include "link.h"

//定义播放模式
#define SEQUENCE       1             //顺序播放
#define CIRCLE         2             //单曲循环

#define SHMKEY      1234
#define SHMSIZE     4096

#define DFL_VOL     100

#define URL         "http://106.14.253.205/music/"

//共享内存存放的数据
typedef struct Shm
{
	pid_t parent_pid;       //父进程的PID
	pid_t child_pid;        //子进程的PID
	pid_t grand_pid;        //孙进程的PID
	char cur_music[128];    //当前播放的歌曲名字
	int mode;               //播放模式
}Shm;

int init_shm();
void get_music(const char *singer);
void get_shm(Shm *s);
void get_volume(int *v);
int start_play();
void play_music(char *n);
void child_process(char *n);
void grand_set_shm(Shm s);
void grand_get_shm(Shm *s);
void stop_play();
void suspend_play();
void continue_play();
void next_play();
void prior_play();
void voice_down();
void voice_up();
void sequence_play();
void circle_play();
void singer_play(const char *);

#endif
