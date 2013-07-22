/************************************************************************/
/*                                                                      */
/************************************************************************/
#ifndef _REDIS_DEBUG_H_
#define _REDIS_DEBUG_H_

#include <signal.h>

namespace redis{
	//void _redisAssertWithInfo(redisClient *c, robj *o, char *estr, char *file, int line);
	void _redisAssert(const char *estr, const char *file, int line);
	void _redisPanic(const char *msg, const char *file, int line);
	void bugReportStart(void);
	//void redisLogObjectDebugInfo(robj *o);
	//void sigsegvHandler(int sig, siginfo_t *info, void *secret);
	//sds genRedisInfoString(char *section);
	void enableWatchdog(int period);
	void disableWatchdog(void);
	void watchdogScheduleSignal(int period);
	void redisLogHexDump(int level, const char *descr, const void *value, size_t len);
}

#define redisDebug(fmt, ...) \
	printf("DEBUG %s:%d > " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)

#define redisDebugMark() \
	printf("-- MARK %s:%d --\n", __FILE__, __LINE__)

#define redisAssert(_e) ((_e)?(void)0 : (_redisAssert(#_e,__FILE__,__LINE__),_exit(1)))

#endif //_REDIS_DEBUG_H_
