#ifndef _SERVER_H_
#define _SERVER_H_

#define WIFIMAIN_SOCK "wifiMain.sock"
#define MAX_CONNECTION_NUMBER 10

#define PAGE_SIZE		4096
#define MMAP_SIZE		PAGE_SIZE
#define IPC_FILE_NAME	"/dev/shm/netipcfile"
#define	MAX_SHAREMEM_SIZE	(16 << 20)	// Max share memory size = 16MB
#define	SHAREMEM_MAGIC		(0x12345678)
#define FILE_FLAG		(O_RDWR | O_CREAT)
#define FILE_MODE		(S_IRUSR | S_IWUSR)
#define MMAP_PROT		(PROT_READ | PROT_WRITE)

#define IPCKEY		0x12				/** 进程通信的IPCKey **/
#define WORKPATH	"/work/updplat/bin/"
#define MAX_WPEXTWMODULE_THREADNUM 128

typedef struct s_client_info
{
	int fd;
	char name[32];
} client_info;

struct sharemem_handle
{
	int fd;
	void *mmap_base;
	size_t mmap_size;
	int mmap_flag;
};

struct proc_handle {
	pid_t nProcID;				/* 进程ID，对父子进程均有效 */
	int nProcIdx;				/* 进程索引号，对父子进程均有效 */

	int nProcessNum;			/* 系统中的进程数目，仅对父进程有效 */
	int nThreadsNum;			/* 进程中的线程数目，对父子进程均有效 */

	volatile uint64_t nCommand;		/* 进程控制命令，对父子进程均有效 */
	volatile uint64_t nStatus;		/* 进程控制状态，对父子进程均有效 */

	unsigned long nMinSysMemSize;		/* 系统最低物理内存保障，仅对父进程有效 */

	unsigned long nOsUsedMemSize;		/* 内核态角度中的已使用内存数(KB)，对父子进程均有效 */
	unsigned long nOsFreeMemSize;		/* 内核态角度中的未使用内存数(KB)，对父子进程均有效 */
	unsigned long nApUsedMemSize;		/* 用户态角度中的已使用内存数(KB)，对父子进程均有效 */
	unsigned long nApFreeMemSize;		/* 用户态角度中的未使用内存数(KB)，对父子进程均有效 */

	unsigned long nVMemSize;		/* 当前进程所使用的虚拟内存大小(MB) 对父子进程均有效 */
	unsigned long nPMemSize;		/* 当前进程所使用的物理内存大小(MB) 对父子进程均有效 */

	char pEth[32];				/* 当前进程对应的捕获网卡，仅对子进程有效 */
	uint64_t nOldRecvPkts;			/* 当前进程捕获到的数据包数目(上次)，仅对子进程有效 */
	uint64_t nOldRecvByte;			/* 当前进程捕获到的数据包字节(上次)，仅对子进程有效 */
	uint64_t nOldDropPkts;			/* 当前进程被丢弃的数据包数目(上次)，仅对子进程有效 */
	uint64_t nOldDropByte;			/* 当前进程被丢弃的数据包字节(上次)，仅对子进程有效 */

	uint64_t nCurRecvPkts;			/* 当前进程捕获到的数据包数目(本次)，仅对子进程有效 */
	uint64_t nCurRecvByte;			/* 当前进程捕获到的数据包字节(本次)，仅对子进程有效 */
	uint64_t nCurDropPkts;			/* 当前进程被丢弃的数据包数目(本次)，仅对子进程有效 */
	uint64_t nCurDropByte;			/* 当前进程被丢弃的数据包字节(本次)，仅对子进程有效 */

	double   nCurFluxBitsRate;		/* 数据采集模块的当前流量(Mbps)，仅对子进程有效 */
	double   nMaxFluxBitsRate;		/* 数据采集模块的流量峰值(Mbps)，仅对子进程有效 */
	double   nCurFluxPktsRate;		/* 数据采集模块的当前流量(pps)，仅对子进程有效 */
	double   nMaxFluxPktsRate;		/* 数据采集模块的流量峰值(pps)，仅对子进程有效 */

	uint64_t nDecodeTtlPkts;		/* 协议解码模块的已解码总数据包数目，仅对子进程有效 */
	uint64_t nDecodeTtlByte;		/* 协议解码模块的已解码总数据包字节，仅对子进程有效 */
	uint64_t nDecodeTCPPkts;		/* 协议解码模块的已解码TCP数据包数目，仅对子进程有效 */
	uint64_t nDecodeTCPByte;		/* 协议解码模块的已解码TCP数据包字节，仅对子进程有效 */
	uint64_t nDecodeUDPPkts;		/* 协议解码模块的已解码UDP数据包数目，仅对子进程有效 */
	uint64_t nDecodeUDPByte;		/* 协议解码模块的已解码UDP数据包字节，仅对子进程有效 */
	uint64_t nDecodeOthPkts;		/* 协议解码模块的非TCP/UDP数据包数目，仅对子进程有效 */
	uint64_t nDecodeOthByte;		/* 协议解码模块的非TCP/UDP数据包字节，仅对子进程有效 */

	uint64_t nCurTCPLink;			/* 连接管理模块中TCP的当前连接数，仅对子进程有效 */
	uint64_t nMaxTCPLink;			/* 连接管理模块中TCP的最大连接数，仅对子进程有效 */
	uint64_t nCurUDPLink;			/* 连接管理模块中UDP的当前连接数，仅对子进程有效 */
	uint64_t nMaxUDPLink;			/* 连接管理模块中UDP的最大连接数，仅对子进程有效 */
	uint64_t nCurTtlLink;			/* 连接管理模块中当前总的连接数，仅对子进程有效 */

	uint64_t nOutPutBcpItem;		/* 数据输出模块中当前输出BCP条目，仅对子进程有效 */
	uint64_t nOutPutBcpSize;		/* 数据输出模块中当前输出BCP大小，仅对子进程有效 */
	uint64_t nOutPutZipItem;		/* 数据输出模块中当前输出Zip条目，仅对子进程有效 */
	uint64_t nOutPutZipSize;		/* 数据输出模块中当前输出Zip大小，仅对子进程有效 */
	uint64_t nOutPutVoipItem;		/* 数据输出模块中当前输出Voip的Cap条目，仅对子进程有效 */
	uint64_t nOutPutVoipSize;		/* 数据输出模块中当前输出Voip的Cap大小，仅对子进程有效 */
	uint64_t nOutPutQQItem;			/* 数据输出模块中当前输出QQ的Cap条目，仅对子进程有效 */
	uint64_t nOutPutQQSize;			/* 数据输出模块中当前输出QQ的Cap大小，仅对子进程有效 */
	uint64_t nOutPutSpecialItem;		/* 数据输出模块中当前输出Special的Cap条目，仅对子进程有效 */
	uint64_t nOutPutSpecialSize;		/* 数据输出模块中当前输出Special的Cap大小，仅对子进程有效 */
	

	uint64_t nOutPutNode;			/* 数据输出模块中当前输出节点数目，仅对子进程有效 */
	uint64_t nOutPutSize;			/* 数据输出模块中当前输出数据大小，仅对子进程有效 */

	uint64_t nHeartBeat;			/* 当前进程的心跳，仅对子进程有效 */
	uint64_t nOldTimeStamp;			/* 当前进程的时戳(上次)，仅对子进程有效 */
	uint64_t nCurTimeStamp;			/* 当前进程的时戳(本次)，仅对子进程有效 */

	uint64_t nEspThreadNum;			/* 二级解码线程个数，仅对子进程有效 */
	uint64_t EspThreadDatanum[MAX_WPEXTWMODULE_THREADNUM];		/* 二级解码线程节点数，仅对子进程有效 */

	int nIPCKey;				/* 进程IPC通信键值，对父子进程均有效 */
	int nLockFileDesc;			/* 进程互斥锁文件描述符，对父子进程均有效 */
    unsigned int nGtpcRcvPk;          /*收到的广播包*/
    unsigned int nGtpcFound;          /*匹配个数*/
    unsigned int nGtpcFind;           /*查找次数*/
	struct sharemem_handle *pShmHandle;	/* 当前进程的共享内存句柄，对父子进程均有效 */
	void *pMsqHandle;	/* 当前进程的消息队列句柄，对父子进程均有效 */
};

#endif /* _SERVER_H_ */