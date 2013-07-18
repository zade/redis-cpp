/** \file redis.h *
*
* \brief 
*
* \author zade(zhaohongchao@gmail.com)
*
* \date 2013��7��16��
*
* \version 1.0
*
* \since 1.0
*/
#ifndef _REDIS_CPP_REDIS_H_
#define _REDIS_CPP_REDIS_H_

#include "log.h"

namespace redis{

	struct Server{
		int sentinel_mode;
		int daemonize;

		Logging log;
	};

	extern Server server;
}

#endif // _REDIS_CPP_REDIS_H_
