DEBUG=-DARM
#DEBUG=
Target=main
Object=main.o select.o device.o player.o link.o socket.o 

CC=arm-linux-gcc
CFLAGS=-L /home/json_arm_install/lib -I /home/json_arm_install/include $(DEBUG)

$(Target):$(Object)
	$(CC) $(Object) -o $(Target) -Wall -ljson $(DEBUG)  -static $(CFLAGS)

clean:
	rm *.o main


