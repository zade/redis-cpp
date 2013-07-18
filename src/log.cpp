/************************************************************************/
/*                                                                      */
/************************************************************************/
#include "log.h"
#include "consts.h"
#include "util.h"

#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

namespace redis{namespace{

}


Logging::Logging()
	:syslog_enabled(0),verbosity(0)
{}

Logging::Logging(const std::string& path,int syslog,int verbosity)
	:syslog_enabled(syslog),verbosity(verbosity),path(path)
{}

void Logging::set(const std::string& path,int sys_log,int verbosity){
	this->path = path;
	this->verbosity = verbosity;
	this->syslog_enabled = sys_log;
}

void Logging::log(int level, const char *fmt, ...){
	if (!((level&0xff) < this->verbosity)) {
		char msg[REDIS_MAX_LOGMSG_LEN];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(msg, sizeof(msg), fmt, ap);
		va_end(ap);
		this->logRaw(level,msg);
	}
}
void Logging::log(int level,const char *fmt, va_list al){
	if (!((level&0xff) < this->verbosity)) {
		char msg[REDIS_MAX_LOGMSG_LEN];
		vsnprintf(msg, sizeof(msg), fmt, al);
		this->logRaw(level,msg);
	}
}

void Logging::logRaw(int level, const char *msg){
	level &= 0xff; /* clear flags */
	if (level < this->verbosity) {
		return;
	}

	FILE* fp = this->path.empty() ? stdout : fopen(this->path.c_str(),"a");
	if (!fp) {
		return;
	}

	if (level & REDIS_LOG_RAW) {
		fprintf(fp,"%s",msg);
	} else {
		const char *c = ".-*#";
		char buf[64];
		int off = 0;
		struct timeval tv;
		gettimeofday(&tv,NULL);
		off = strftime(buf,sizeof(buf),"%d %b %H:%M:%S.",localtime(&tv.tv_sec));
		snprintf(buf+off,sizeof(buf)-off,"%03d",(int)tv.tv_usec/1000);
		fprintf(fp,"[%d] %s %c %s\n",(int)getpid(),buf,c[level],msg);
	}
	fflush(fp);

	if (!this->path.empty()) {
		fclose(fp);
	}

	if (this->syslog_enabled) {
		const int syslogLevelMap[] = { LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING };
		syslog(syslogLevelMap[level], "%s", msg);
	}
}

void Logging::logFromHandler(int level, int daemonize,const char *msg){
	
	if ((level&0xff) < this->verbosity ||
		(this->path.empty()&& daemonize)) 
	{
			return;
	}
	
	int fd = this->path.empty() ?
		open(this->path.c_str(), O_APPEND|O_CREAT|O_WRONLY, 0644) : STDOUT_FILENO;
	if (fd == -1) {
		return;
	}

	do {
		char buf[64];
		//in handler you can't call snprintf because it is not reentry function
		ll2string(buf,sizeof(buf),getpid());
		
		if (write(fd,"[",1) == -1) {
			break;
		}
		if (write(fd,buf,strlen(buf)) == -1) {
			break;
		}
		if (write(fd," | signal handler] (",20) == -1) {
			break;
		}
		ll2string(buf,sizeof(buf),time(NULL));
		if (write(fd,buf,strlen(buf)) == -1) {
			break;
		}
		if (write(fd,") ",2) == -1) {
			break;
		}
		if (write(fd,msg,strlen(msg)) == -1) {
			break;
		}
		if (write(fd,"\n",1) == -1) {
			break;
		}
	} while (0);
	
	if (!this->path.empty()) {
		close(fd);
	}
}

}

