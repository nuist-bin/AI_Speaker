#ifndef DEVICE_H
#define DEVICE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

int init_device();
void start_buzzer();
int get_key_id();

#endif
