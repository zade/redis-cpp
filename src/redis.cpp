/** \file redis.cpp
*
* \brief 
*
* TODO 
*
* \author zade(zhaohongchao@gmail.com)
*
* \date 2013Äê7ÔÂ17ÈÕ
*
* \version 1.0.0 
*
* \since 1.0.0
*/

#include "redis.h"
#include "util.h"

#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>

namespace redis{ namespace{

	/* Returns 1 if there is --sentinel among the arguments or if
	* argv[0] is exactly "redis-sentinel". */
	int checkForSentinelMode(int argc, char **argv) {
		if (strstr(argv[0],"redis-sentinel") != NULL) {
			return 1;
		}
		for (int j = 1; j < argc; j++){
			if (!strcmp(argv[j],"--sentinel")) {
				return 1;
			}
		}
		return 0;
	}

	void initServerConfig(){

	}
}
/*============================ Utility functions ============================ */

/* Low level logging. To use only for very big messages, otherwise
 * redisLog() is to prefer. */
void redisLogRaw(int level, const char *msg) {
    const int syslogLevelMap[] = { LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING };
    const char *c = ".-*#";
    FILE *fp;
    char buf[64];
    int rawmode = (level & REDIS_LOG_RAW);

    level &= 0xff; /* clear flags */
    if (level < server.verbosity) return;

    fp = (server.logfile == NULL) ? stdout : fopen(server.logfile,"a");
    if (!fp) return;

    if (rawmode) {
        fprintf(fp,"%s",msg);
    } else {
        int off;
        struct timeval tv;

        gettimeofday(&tv,NULL);
        off = strftime(buf,sizeof(buf),"%d %b %H:%M:%S.",localtime(&tv.tv_sec));
        snprintf(buf+off,sizeof(buf)-off,"%03d",(int)tv.tv_usec/1000);
        fprintf(fp,"[%d] %s %c %s\n",(int)getpid(),buf,c[level],msg);
    }
    fflush(fp);

    if (server.logfile) fclose(fp);

    if (server.syslog_enabled) syslog(syslogLevelMap[level], "%s", msg);
}

/* Like redisLogRaw() but with printf-alike support. This is the function that
 * is used across the code. The raw version is only used in order to dump
 * the INFO output on crash. */
void redisLog(int level, const char *fmt, ...) {
    va_list ap;
    char msg[REDIS_MAX_LOGMSG_LEN];

    if ((level&0xff) < server.verbosity) return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    redisLogRaw(level,msg);
}

/* Log a fixed message without printf-alike capabilities, in a way that is
 * safe to call from a signal handler.
 *
 * We actually use this only for signals that are not fatal from the point
 * of view of Redis. Signals that are going to kill the server anyway and
 * where we need printf-alike features are served by redisLog(). */
void redisLogFromHandler(int level, const char *msg) {
    int fd;
    char buf[64];

    if ((level&0xff) < server.verbosity ||
        (server.logfile == NULL && server.daemonize)) return;
    fd = server.logfile ?
        open(server.logfile, O_APPEND|O_CREAT|O_WRONLY, 0644) :
        STDOUT_FILENO;
    if (fd == -1) return;
    ll2string(buf,sizeof(buf),getpid());
    if (write(fd,"[",1) == -1) goto err;
    if (write(fd,buf,strlen(buf)) == -1) goto err;
    if (write(fd," | signal handler] (",20) == -1) goto err;
    ll2string(buf,sizeof(buf),time(NULL));
    if (write(fd,buf,strlen(buf)) == -1) goto err;
    if (write(fd,") ",2) == -1) goto err;
    if (write(fd,msg,strlen(msg)) == -1) goto err;
    if (write(fd,"\n",1) == -1) goto err;
err:
    if (server.logfile) close(fd);
}


// golbal server object
redisServer server;

int Main(int argc, char **argv){
	server.sentinel_mode = checkForSentinelMode(argc,argv);
	return 0;
}

}


int main(int argc, char **argv){
	return redis::Main(argc,argv);
}
