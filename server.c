#include <error.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/timerfd.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include "server.h"
#include "cJSON.h"

int g_wifimain_fd = -1;
int g_timer_fd = -1;
client_info g_client_info[MAX_CONNECTION_NUMBER];

int wifimain_server_parsejson(char *msg, char *msgtype, char *msgmodule, char *msgresult, char *msgcontent)
{
	cJSON* obj;
	cJSON* item;
	
	obj = cJSON_Parse(msg);
	if (obj == NULL)
	{
		return -1;
	}
	
	if (obj->type != cJSON_Object)
	{
		cJSON_Delete(obj);
		return -1;
	}
	
	item = cJSON_GetObjectItem(obj, "type");
	if (item != NULL)
	{
		strncpy(msgtype, item->valuestring, 127);
	}
	item = cJSON_GetObjectItem(obj, "module");
	if (item != NULL)
	{
		strncpy(msgmodule, item->valuestring, 127);
	}
	item = cJSON_GetObjectItem(obj, "result");
	if (item != NULL)
	{
		strncpy(msgresult, item->valuestring, 31);
	}
	item = cJSON_GetObjectItem(obj, "content");
	if (item != NULL)
	{
		strncpy(msgcontent, item->valuestring, 31);
	}
	
	cJSON_Delete(obj);
	return 0;
}

int wifimain_server_msgsend(int fd, char *msg)
{
	int len;
	char buf[1024];
	char spec[6] = {0xef, 0xef, 0xef, 0xef, 0xef, 0xef};
	
	if (fd > 0)
	{
		memset(buf, 0, 1024);
		memcpy(buf, spec, 6);
		*(unsigned short *)(buf + 6) = strlen(msg);
		strcpy(buf + 8, msg);
		len = send(fd, buf, 8 + strlen(msg), 0);
		if (len != 8 + strlen(msg))
		{
			printf("Failed to send msg(%s)\n", msg);
			return -1;
		}
		
		return 0;
	}
	
	return -1;
}

int wifimain_server_msg_process(client_info *info, char *buf)
{
	char msgtype[128] = {0};
	char msgmodule[128] = {0};
	char msgresult[32] = {0};
	char msgcontent[1024] = {0};
	
	printf("Received msg(%s)\n", buf);
	wifimain_server_parsejson(buf, msgtype, msgmodule, msgresult, msgcontent);
	printf("msgtype(%s), msgmodule(%s), msgresult(%s), msgcontent(%s)\n", msgtype, msgmodule, msgresult, msgcontent);
	
	if (strcmp(msgtype, "init") == 0)
	{
		strncpy(info->name, msgmodule, 31);
	}
	else if (strcmp(msgtype, "result") == 0)
	{
		printf("Module(%s), result(%s), content(%s)\n", msgmodule, msgresult, msgcontent);
	}
	else if (strcmp(msgtype, "log") == 0)
	{
		printf("Module(%s), log(%s)\n", msgmodule, msgcontent);
	}
	wifimain_server_msgsend(info->fd, "Don't response!");
	return 0;
}

int wifimain_server_msg_recv(client_info *info)
{
	char buf[1024];
	char spec[6] = {0xef, 0xef, 0xef, 0xef, 0xef, 0xef};
	int len;
	int data_len;
	
	len = recv(info->fd, buf, 8, 0);
	if (len == 0)
	{
		printf("Close client(%s)\n", info->name);
		close(info->fd);
		info->fd = -1;
		memset(info->name, 0, 32);
	}
	else if (len == 8)
	{
		if (memcmp(spec, buf, 6) == 0)
		{
			data_len = *(unsigned short *)(buf + 6);
			if (data_len > 1024 - 1)
			{
				data_len = 1024 - 1;
			}
			
			len = recv(info->fd, buf, data_len, 0);
			if (len == data_len)
			{
				buf[data_len] = 0;
				wifimain_server_msg_process(info, buf);
			}
		}
	}
	
	return 0;
}

int wifimain_server_check_networkcard_ip(const char *file, const char *eth)
{
	FILE *fp;
	char buf[1024];
	char *str;
	
	fp = fopen(file, "r");
	if (fp == NULL)
	{
		return -1;
	}
	
	fread(buf, 1, 1024, fp);
	fclose(fp);
	
	if (strcmp(eth, "eth0") == 0)
	{
		str = strstr(buf, "inet 192.168.30.1");
		if (str != NULL)
		{
			return 0;
		}
		else
		{
			printf("Error, the ip of eth0 must be 192.168.30.1\n");
		}
	}
	else if (strcmp(eth, "wlan1") == 0)
	{
		str = strstr(buf, "inet 192.168.60.1");
		if (str != NULL)
		{
			return 0;
		}
		else
		{
			printf("Error, the ip of wlan1 must be 192.168.60.1\n");
		}
	}
	else if (strcmp(eth, "wlan4") == 0)
	{
		str = strstr(buf, "inet 192.168.10.1");
		if (str != NULL)
		{
			return 0;
		}
		else
		{
			printf("Error, the ip of wlan4 must be 192.168.10.1\n");
		}
	}
	
	return -1;
}

int wifimain_server_read_networkcard_stream(const char *file, const char *eth)
{
	FILE *fp;
	char *line = NULL;
	size_t len;
	ssize_t read;
	char *ptmp;
	
	fp = fopen(file, "r");
	if (fp == NULL)
	{
		return -1;
	}
	
	printf("%s:\n", eth);
	while ((read = getline(&line, &len, fp)) != -1)
	{
		if ((ptmp = strstr(line, "RX packets")))
		{
			printf("%s", ptmp);
		}
		else if ((ptmp = strstr(line, "RX errors")))
		{
			printf("%s", ptmp);
		}
		else if ((ptmp = strstr(line, "TX packets")))
		{
			printf("%s", ptmp);
		}
		else if ((ptmp = strstr(line, "TX errors")))
		{
			printf("%s", ptmp);
		}
	}
	
	if (line)
	{
		free(line);
	}
	fclose(fp);
	
	return 0;
}

int wifimain_server_read_networkcard(char *file)
{
	FILE *fp;
	char buf[1024];
	
	fp = fopen(file, "r");
	if (fp == NULL)
	{
		return -1;
	}
	
	fread(buf, 1, 1024, fp);
	fclose(fp);
	if (strstr(buf, "lo") && strstr(buf, "eth0") && strstr(buf, "eth1")
		&& strstr(buf, "wlan1") && strstr(buf, "wlan3") && strstr(buf, "wlan4")
		&& strstr(buf, "wlan0mon") && strstr(buf, "wlan2mon"))
	{
		printf("The networkCard is normal\n");
		return 0;
	}
	
	printf("The networkCard is abnormal, networkcard:\n%s\n", buf);
	return -1;
}

static int inline GetIPCKey()
{
	int key = 0;

	key = ftok(WORKPATH, IPCKEY);

	return key;
}

void *sysSharememOpen(int key, const char *file_name, size_t size)
{
	struct sharemem_handle *shm_handle = NULL;

	if((size <= 0) || (size > MAX_SHAREMEM_SIZE)) 
	{
		return NULL;
	}

	shm_handle = (struct sharemem_handle *)malloc(sizeof(struct sharemem_handle));
	if(shm_handle == NULL) 
	{
		return NULL;
	}
	shm_handle->fd = -1;
	shm_handle->mmap_base = NULL;
	shm_handle->mmap_size = size;

	if(file_name != NULL) 
	{
		char shmfilename[256] = "";
		int buff = SHAREMEM_MAGIC;

		sprintf(shmfilename, "%s_%08x_shm", file_name, key);
		shm_handle->fd = open(shmfilename, FILE_FLAG, FILE_MODE);
		if(shm_handle->fd < 0) 
		{
			printf("Failed to open %s, error(%d, %s)", shmfilename, errno, strerror(errno));
			goto error_0;
		}

		/* just write something into this file, avoid "Bus error" */
		lseek(shm_handle->fd, size-sizeof(buff), SEEK_SET);
		write(shm_handle->fd, &buff, sizeof(buff));
		shm_handle->mmap_flag = MAP_SHARED;
	} else {
		shm_handle->fd = -1;
		shm_handle->mmap_flag = MAP_SHARED | MAP_ANONYMOUS;
	}

	shm_handle->mmap_base = mmap(NULL, shm_handle->mmap_size, MMAP_PROT, shm_handle->mmap_flag, shm_handle->fd, 0);
	if(shm_handle->mmap_base == MAP_FAILED) {
		goto error_1;
	}

	return shm_handle;

error_1:
	if(shm_handle->fd) {
	 	close(shm_handle->fd);
		shm_handle->fd = -1;
		shm_handle->mmap_base = NULL;
		shm_handle->mmap_size = 0;
	}

error_0:
	if(shm_handle) {
		free(shm_handle);
		shm_handle = NULL;
	}

	return NULL;
}

void sysSharememClose(void *handle)
{
	struct sharemem_handle *shm_handle = (struct sharemem_handle *)handle;

	if(shm_handle == NULL) 
	{
		return;
	}

	if(shm_handle->mmap_base) 
	{
		munmap(shm_handle->mmap_base, shm_handle->mmap_size);
	}

	if(shm_handle->fd) 
	{
		close(shm_handle->fd);
		shm_handle->fd = -1;
		shm_handle->mmap_base = NULL;
		shm_handle->mmap_size = 0;
	}

	if(shm_handle) 
	{
		free(shm_handle);
		shm_handle = NULL;
	}
}

int sysSharememRead(void *handle, int pindex, int offset, char *buff, size_t size)
{
	struct sharemem_handle *shm_handle = (struct sharemem_handle *)handle;
	char *base = NULL, *ptr = NULL;

	if((shm_handle == NULL) || (buff == NULL) || (size > PAGE_SIZE))
		return -1;

	if(shm_handle->mmap_base == NULL) 
	{
		return -1;
	}

	base = (char *)shm_handle->mmap_base + pindex * PAGE_SIZE;
	ptr  = base + offset;
	if((ptr + size) > (base + PAGE_SIZE) || (ptr < base))
		return -1;

	memcpy(buff, ptr, size);

	return size;
}

int sysInfoPrint(struct proc_handle *tmpProcHandle)
{
	printf("\n");
	printf("Ap Used Mem(%lu KB)\n", tmpProcHandle->nApUsedMemSize);
	printf("Ap Free Mem(%lu KB)\n", tmpProcHandle->nApFreeMemSize);
	
	printf("Vir Mem(%lu MB)\n", tmpProcHandle->nVMemSize);
	printf("Phy Mem(%lu MB)\n", tmpProcHandle->nPMemSize);
	
	printf("Decode Total Pkts(%lu)\n", tmpProcHandle->nDecodeTtlPkts);
	printf("Decode Total Byte(%lu)\n", tmpProcHandle->nDecodeTtlByte);
	printf("Decode Tcp Pkts(%lu)\n", tmpProcHandle->nDecodeTCPPkts);
	printf("Decode Tcp Byte(%lu)\n", tmpProcHandle->nDecodeTCPByte);
	printf("Decode Udp Pkts(%lu)\n", tmpProcHandle->nDecodeUDPPkts);
	printf("Decode Udp Byte(%lu)\n", tmpProcHandle->nDecodeUDPByte);
	printf("Decode Other Pkts(%lu)\n", tmpProcHandle->nDecodeOthPkts);
	printf("Decode Other Byte(%lu)\n", tmpProcHandle->nDecodeOthByte);
	
	printf("Output Bcp Item(%lu)\n", tmpProcHandle->nOutPutBcpItem);
	printf("Output Bcp Size(%lu)\n", tmpProcHandle->nOutPutBcpSize);
	printf("Output Zip Item(%lu)\n", tmpProcHandle->nOutPutZipItem);
	printf("Output Zip Size(%lu)\n", tmpProcHandle->nOutPutZipSize);
	printf("\n");
	
	return 0;
}

int wifimain_server_check_updplat_status()
{
	struct sharemem_handle *pShmHandle = NULL;
	struct proc_handle tmpProcHandle;
	size_t size = sizeof(struct proc_handle);
	int rc;
	int key;
	
	/** 获取IPC进程通信的键值(nIPCKey) **/
	key = GetIPCKey();
	pShmHandle = (struct sharemem_handle *)sysSharememOpen(key, IPC_FILE_NAME, MMAP_SIZE);
	if (pShmHandle == NULL)
	{
		printf("Failed to open sharemem, error(%d, %s)\n", errno, strerror(errno));
		return -1;
	}
	
	memset(&tmpProcHandle, 0, sizeof(tmpProcHandle));
	rc = sysSharememRead((void *)pShmHandle, 0, 0, (char *)&tmpProcHandle, size);
	if (rc < 0)
	{
		printf("Failed to read sharemem, error(%d, %s)\n", errno, strerror(errno));
	}
	
	sysInfoPrint(&tmpProcHandle);
	sysSharememClose(pShmHandle);
	
	return 0;
}

int wifimain_server_timer_process()
{
	char buf[128];
	
	read(g_timer_fd, buf, 128);
	
	/* check networkCard */
	system("ip addr | grep mtu | awk \'{print $2}\' > networkCard.tmp");
	wifimain_server_read_networkcard("networkCard.tmp");
	
	/* check access networkCard eth0 */
	system("ifconfig eth0 > eth0.tmp");
	wifimain_server_check_networkcard_ip("eth0.tmp", "eth0");
	
	/* check access networkCard eth1 */
	system("ifconfig eth1 > eth1.tmp");
	wifimain_server_read_networkcard_stream("eth1.tmp", "eth1");
	
	/* check access networkCard wlan1 */
	system("ifconfig wlan1 > wlan1.tmp");
	wifimain_server_check_networkcard_ip("wlan1.tmp", "wlan1");
	
	/* check access networkCard wlan3 */
	system("ifconfig wlan3 > wlan3.tmp");
	wifimain_server_read_networkcard_stream("wlan3.tmp", "wlan3");
	
	/* check access networkCard wlan4 */
	system("ifconfig wlan4 > wlan4.tmp");
	wifimain_server_check_networkcard_ip("wlan4.tmp", "wlan4");
	
	/* check updplat status */
	wifimain_server_check_updplat_status();
	
	return 0;
}

int wifimain_server_accept()
{
	int fd;
	struct sockaddr_in clt;
	socklen_t clt_len = sizeof(clt);
	int i;

	fd = accept(g_wifimain_fd, (struct sockaddr *)&clt, &clt_len);
	if (fd < 0)
	{
		return -1;
	}
	
	for (i = 0; i < MAX_CONNECTION_NUMBER; i++)
	{
		if (g_client_info[i].fd == -1)
		{
			g_client_info[i].fd = fd;
			break;
		}
	}
	
	return 0;
}

void wifimain_server_socket_process()
{
	fd_set fdset;
	struct timeval tv;
	int max_fd;
	int rc;
	int i;
	
	while (1)
	{
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&fdset);
		
		max_fd = g_wifimain_fd;
		FD_SET(g_wifimain_fd, &fdset);
		
		if (g_timer_fd > 0)
		{
			max_fd = (max_fd > g_timer_fd) ? max_fd : g_timer_fd;
			FD_SET(g_timer_fd, &fdset);
		}
		
		for (i = 0; i < MAX_CONNECTION_NUMBER; i++)
		{
			if (g_client_info[i].fd != -1)
			{
				FD_SET(g_client_info[i].fd, &fdset);
				max_fd = (max_fd > g_client_info[i].fd) ? max_fd : g_client_info[i].fd;
			}
		}
		
		rc = select(max_fd + 1, &fdset, NULL, NULL, &tv);
		if (rc <= 0)
		{
			continue;
		}
		
		if (FD_ISSET(g_wifimain_fd, &fdset))
		{
			wifimain_server_accept();
		}
		
		if (FD_ISSET(g_timer_fd, &fdset))
		{
			wifimain_server_timer_process();
		}
		
		for (i = 0; i < MAX_CONNECTION_NUMBER; i++)
		{
			if (g_client_info[i].fd == -1)
			{
				continue;
			}
			if (FD_ISSET(g_client_info[i].fd, &fdset))
			{
				wifimain_server_msg_recv(&g_client_info[i]);
			}
		}
	}
}

int wifimain_server_timer_init()
{
	int rc;
	struct itimerspec it;
	
	g_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (g_timer_fd < 0)
	{
		printf("Failed to create timer, error(%d, %s)\n", errno, strerror(errno));
		return -1;
	}
	
	it.it_value.tv_sec = 1;
	it.it_value.tv_nsec = 0;
	it.it_interval.tv_sec = 10;
	it.it_interval.tv_nsec = 0;
	rc = timerfd_settime(g_timer_fd, 0, &it, NULL);
	if (rc < 0)
	{
		printf("Failed to set timer, error(%d, %s)\n", errno, strerror(errno));
		close(g_timer_fd);
		g_timer_fd = -1;
		return -1;
	}
	
	return 0;
}

int wifimain_server_init()
{
	int i;
	
	for (i = 0; i < MAX_CONNECTION_NUMBER; i++)
	{
		g_client_info[i].fd = -1;
		memset(g_client_info[i].name, 0, 32);
	}
	
	return 0;
}

int wifimain_server_reboot_app(char *appname)
{
	char buf[1024];
	
	snprintf(buf, 1024, "killall -9 %s", appname);
	if (strcmp(appname, "updplat") == 0)
	{
		system("/work/updplat/bin/updplat&");
	}
	
	return 0;
}

int wifimain_server_parse_opt(int argc, char **argv)
{
	int ch;
	
	while ((ch = getopt(argc, argv, "r:h")) != -1)
	{
		switch (ch)
		{
			case 'r':
				wifimain_server_reboot_app(optarg);
				exit(0);
			case 'h':
				printf("....hello....\n");
				exit(0);
			default:
				break;
		}
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	struct sockaddr_un un;
	int rc;
	int len;
	
	wifimain_server_parse_opt(argc, argv);
	/* data init */
	wifimain_server_init();
	/* timer init */
	wifimain_server_timer_init();
	
	/* server fd init */
	g_wifimain_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (g_wifimain_fd < 0)
	{
		printf("Failed to create socket, error(%d, %s)\n", errno, strerror(errno));
		return -1;
	}
	
	unlink(WIFIMAIN_SOCK);
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, WIFIMAIN_SOCK);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(WIFIMAIN_SOCK);
	
	rc = bind(g_wifimain_fd, (struct sockaddr *)&un, len);
	if (rc < 0)
	{
		printf("Failed to bind socket, error(%d, %s)\n", errno, strerror(errno));
		return -1;
	}
	
	rc = listen(g_wifimain_fd, MAX_CONNECTION_NUMBER);
	if (rc < 0)
	{
		printf("Failed to listen socket, error(%d, %s)\n", errno, strerror(errno));
		return -1;
	}
	
	wifimain_server_socket_process();
	
	return 0;
}