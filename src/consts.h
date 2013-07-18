/************************************************************************/
/*                                                                      */
/************************************************************************/
#ifndef _REDIS_CONSTS_H
#define _REDIS_CONSTS_H

namespace redis{
	enum{
		/* Log levels */
		REDIS_DEBUG = 0,
		REDIS_VERBOSE = 1,
		REDIS_NOTICE = 2,
		REDIS_WARNING = 3,
		REDIS_LOG_RAW = (1<<10), /* Modifier to log without timestamp */
		REDIS_MAX_LOGMSG_LEN = 1024
	};
}

#endif //_REDIS_CONSTS_H
