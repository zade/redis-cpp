/************************************************************************/
/*                                                                      */
/************************************************************************/
#ifndef _REDIS_LOG_H_
#define _REDIS_LOG_H_

#include <string>
#include <stdarg.h>

namespace redis{
	
	struct Logging {
		int syslog_enabled;
		int verbosity;
		std::string path;
	public :
		Logging();
		Logging(const std::string& path,int syslog,int verbosity);
		void set(const std::string& path,int syslog,int verbosity);
		void log(int level, const char *fmt, ...);
		void log(int level,const char *fmt, va_list al);
		void logRaw(int level, const char *msg);
		void logFromHandler(int level,int daemonize, const char *msg);
		int	 logFD();

		static Logging& instance();
	};

	void redisLog(int level, const char *fmt, ...);
	void redisLogRaw(int level, const char *msg);
	void redisLogFromHandler(int level,int daemonize, const char *msg);
}

#endif //_REDIS_LOG_H_
