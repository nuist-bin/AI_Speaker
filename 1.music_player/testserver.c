#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

void reply_music(int fd)
{
		struct json_object *obj = (struct json_object *)json_object_new_object();
		json_object_object_add(obj, "cmd", json_object_new_string("reply_music"));
		struct json_object *arr = (struct json_object *)json_object_new_array();
		json_object_array_add(arr, json_object_new_string("其他/以后的以后.mp3"));
		json_object_array_add(arr, json_object_new_string("其他/平凡之路.mp3"));
		json_object_array_add(arr, json_object_new_string("其他/朋友.mp3"));
		json_object_array_add(arr, json_object_new_string("其他/那片海.mp3"));
		json_object_array_add(arr, json_object_new_string("其他/倾国倾城.mp3"));
		json_object_object_add(obj, "music", arr);

		char buf[1024] = {0};
		const char *s = (const char *)json_object_to_json_string(obj);
		int len = strlen(s);
		memcpy(buf, &len, 4);
		memcpy(buf + 4, s, len);

		send(fd, buf, len + 4, 0);
	
}

void *recv_client(void *arg)
{
	int fd = *(int *)arg;
	char buf[1024] = {0};

	while (1)
	{
		memset(buf, 0, 1024);
		int len;
		int ret = recv(fd, &len, 4, 0);
		if (-1 == ret)
		{
			perror("recv");
		}
		else if (0 == ret)
		{
			printf("异常退出\n");
			break;
		}

		ret = recv(fd, buf, len, 0);
		if (-1 == ret)
		{
			perror("recv");
		}
		else if (0 == ret)
		{
			printf("异常退出\n");
			break;
		}

		printf("%s\n", buf);

		struct json_object *json = (struct json_object *)json_tokener_parse(buf);
		struct json_object *val = (struct json_object *)json_object_object_get(json, "cmd");
		const char *s = (const char *)json_object_get_string(val);

		if (!strcmp(s, "get_music_list"))
		{
			reply_music(fd);
		}


	}
}

int main()
{
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	
	int opt = 1; 
	setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in server_info;
	memset(&server_info, 0, sizeof(server_info));
	server_info.sin_family = PF_INET;
	server_info.sin_port = htons(8000);
	server_info.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(sockfd, (struct sockaddr *)&server_info, sizeof(server_info)) == -1)
	{
		perror("bind");
		return -1;
	}

	if (listen(sockfd, 10) == -1)
	{
		perror("listen");
		return -1;
	}

	struct sockaddr_in client_info;
	int len = sizeof(client_info);

	printf("等待客户端的连接 ....\n");

	int fd = accept(sockfd, (struct sockaddr *)&client_info, &len);
	if (-1 == fd)
	{
		perror("accept");
		return -1;
	}

	printf("连接成功 %d\n", fd);
	
	pthread_t tid;
	pthread_create(&tid, NULL, recv_client, &fd);

	sleep(5);
	const char *s = "{'cmd':'app_start'}";
	char buf[1024] = {0};
	len = strlen(s);
	memcpy(buf, &len, 4);
	memcpy(buf + 4, s, len);
	send(fd, buf, len + 4, 0);

	sleep(5);
	s = "{'cmd':'app_circle'}";
	memset(buf, 0, 1024);
	len = strlen(s);
	memcpy(buf, &len, 4);
	memcpy(buf + 4, s, len);
	send(fd, buf, len + 4, 0);

	sleep(5);
	s = "{'cmd':'app_sequence'}";
	memset(buf, 0, 1024);
	len = strlen(s);
	memcpy(buf, &len, 4);
	memcpy(buf + 4, s, len);
	send(fd, buf, len + 4, 0);

	while (1);

	return 0;
}
