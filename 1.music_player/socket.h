#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <json/json.h>
#include "player.h"

#define PORT    8000
//#define IP      "127.0.0.1"
#define IP      "106.14.253.205"

void socket_send_data(struct json_object *j);
void socket_recv_data(char *msg);
int init_socket();
void upload_music_list();
void socket_start_play();
void socket_stop_play();
void socket_suspend_play();
void socket_continue_play();
void socket_voice_up();
void socket_voice_down();
void socket_circle_play();
void socket_sequence_play();

#endif
