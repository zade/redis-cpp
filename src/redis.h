/** \file redis.h *
*
* \brief 
*
* TODO 这里给出描述
*
* \author zade(zhaohongchao@gmail.com)
*
* \date 2013年7月16日
*
* \version 1.0
*
* \since 1.0
*
* &copy; 版权所有, 2000-2011, 百度科技有限公司保留所有权利
*/
#ifndef _REDIS_CPP_REDIS_H_
#define _REDIS_CPP_REDIS_H_

namespace redis{

	struct redisServer{
		int sentinel_mode;
	};

	extern redisServer server;
}

#endif // _REDIS_CPP_REDIS_H_
