/** \file anet.h *
*
* \brief 
*
* TODO 
*
* \author zade(zhaohongchao@gmail.com)
*
* \date 2013Äê7ÔÂ18ÈÕ
*
* \version 1.0
*
* \since 1.0
*/
#ifndef _REDIS_REDISCPP_ANET_H_
#define _REDIS_REDISCPP_ANET_H_

#if defined(__sun)
#define AF_LOCAL AF_UNIX
#endif

namespace redis{
	enum {
		ANET_OK =  0,
		ANET_ERR = -1,
		ANET_ERR_LEN = 256
	};
	int anetTcpConnect(char *err, char *addr, int port);
	int anetTcpNonBlockConnect(char *err, char *addr, int port);
	int anetUnixConnect(char *err, char *path);
	int anetUnixNonBlockConnect(char *err, char *path);
	int anetRead(int fd, char *buf, int count);
	int anetResolve(char *err, char *host, char *ipbuf);
	int anetTcpServer(char *err, int port, char *bindaddr);
	int anetUnixServer(char *err, char *path, mode_t perm);
	int anetTcpAccept(char *err, int serversock, char *ip, int *port);
	int anetUnixAccept(char *err, int serversock);
	int anetWrite(int fd, char *buf, int count);
	int anetNonBlock(char *err, int fd);
	int anetEnableTcpNoDelay(char *err, int fd);
	int anetDisableTcpNoDelay(char *err, int fd);
	int anetTcpKeepAlive(char *err, int fd);
	int anetPeerToString(int fd, char *ip, int *port);
	int anetKeepAlive(char *err, int fd, int interval);
}
#endif // _REDIS_REDISCPP_ANET_H_

