#!/bin/sh

id=`ipcs -m | grep 4d2 | awk {'print $2'}`

if [ ! -z $id ]; then
	ipcrm -m $id
fi

file="cmd_fifo"

rm -rf $file
mkfifo $file
	
