#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stddef.h>
#include <pthread.h>
#include "client.h"

int g_wifimain_clt_fd = -1;
t_wifimain_client_msgrecv wifimain_client_msgrecv_handle = NULL;
char g_module_name[33];

int client_msg_process(char *buf)
{
	printf("Receive server msg(%s)\n", buf);
	return 0;
}

int msg_recv()
{
	char buf[4096];
	char spec[6] = {0xef, 0xef, 0xef, 0xef, 0xef, 0xef};
	int len;
	int data_len;
	
	len = recv(g_wifimain_clt_fd, buf, 8, 0);
	if (len == 0)
	{
		close(g_wifimain_clt_fd);
		g_wifimain_clt_fd = -1;
	}
	else if (len == 8)
	{
		if (memcmp(spec, buf, 6) == 0)
		{
			data_len = *(unsigned short *)(buf + 6);
			if (data_len > 4096 - 1)
			{
				data_len = 4096 - 1;
			}
			
			len = recv(g_wifimain_clt_fd, buf, data_len, 0);
			if (len == data_len && wifimain_client_msgrecv_handle != NULL)
			{
				buf[data_len] = 0;
				wifimain_client_msgrecv_handle(buf);
			}
		}
	}
	
	return 0;
}

int wifimain_client_conn()
{
	int len;
	int rc;
	struct sockaddr_un un;
	char buf[1024];
	
	g_wifimain_clt_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (g_wifimain_clt_fd < 0)
	{
		printf("Failed to create socket, error(%d, %s)\n", errno, strerror(errno));
		return -1;
	}
	
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, WIFIMAIN_SOCK);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(WIFIMAIN_SOCK);
	
	while (1)
	{
		rc = connect(g_wifimain_clt_fd, (struct sockaddr *)&un, len);
		if (rc < 0)
		{
			sleep(1);
			continue;
		}
		
		break;
	}
	
	snprintf(buf, 1024, "{\"type\":\"init\", \"module\":\"%s\"}", g_module_name);
	wifimain_client_msgsend(buf);
	
	return 0;
}

int wifimain_client_init(char *name)
{
	fd_set fdset;
	struct timeval tv;
	int rc;
	
	strncpy(g_module_name, name, 32);
	
	while (1)
	{
		if (g_wifimain_clt_fd == -1)
		{
			wifimain_client_conn();
		}
		
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&fdset);
		FD_SET(g_wifimain_clt_fd, &fdset);
		
		rc = select(g_wifimain_clt_fd + 1, &fdset, NULL, NULL, &tv);
		if (rc <= 0)
		{
			continue;
		}
		
		if (FD_ISSET(g_wifimain_clt_fd, &fdset))
		{
			
			msg_recv();
		}
	}
	
	return 0;
}

void *socket_process(void *arg)
{
	wifimain_client_msgrecv_handle = client_msg_process;
	wifimain_client_init("test");
	
	return NULL;
}

int wifimain_client_msgsend(char *msg)
{
	int len;
	char buf[1024];
	char spec[6] = {0xef, 0xef, 0xef, 0xef, 0xef, 0xef};
	
	if (g_wifimain_clt_fd > 0)
	{
		memset(buf, 0, 1024);
		memcpy(buf, spec, 6);
		*(unsigned short *)(buf + 6) = strlen(msg);
		strcpy(buf + 8, msg);
		len = send(g_wifimain_clt_fd, buf, 8 + strlen(msg), 0);
		if (len != 8 + strlen(msg))
		{
			printf("Failed to send msg(%s)\n", msg);
			return -1;
		}
		
		return 0;
	}
	
	return -1;
}

int main(int argc, char *argv[])
{
	pthread_t tid;
	int rc;
	
	rc = pthread_create(&tid, NULL, socket_process, NULL);
	if (rc < 0)
	{
		printf("Failed to create thread, error(%d, %s)\n", errno, strerror(errno));
		return -1;
	}
	
	while (1)
	{
		wifimain_client_msgsend("{\"type\":\"init\", \"module\":\"test\", \"content\":\"hello\"}");
		sleep(10);
	}
	
	printf("Client exit\n");
	
	return 0;
}