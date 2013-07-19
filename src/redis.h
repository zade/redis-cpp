/************************************************************************/
/*                                                                      */
/************************************************************************/
#ifndef _REDIS_CPP_REDIS_H_
#define _REDIS_CPP_REDIS_H_

namespace redis{

	struct Server{
		int sentinel_mode;
		int daemonize;
	};

	extern Server server;
}

#endif // _REDIS_CPP_REDIS_H_
