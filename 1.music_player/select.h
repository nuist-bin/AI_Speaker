#ifndef SELECT_H
#define SELECT_H


#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "socket.h"

void init_select();
void select_read_stdio();
void m_select();

#endif
