#ifndef _CLIENT_H_
#define _CLIENT_H_

#define WIFIMAIN_SOCK "wifiMain.sock"

extern int wifimain_client_init(char *name);
extern int wifimain_client_msgsend(char *msg);

typedef int (*t_wifimain_client_msgrecv)(char *msg);

extern t_wifimain_client_msgrecv wifimain_client_msgrecv_handle;

#endif /* _CLIENT_H_ */