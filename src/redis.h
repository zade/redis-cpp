/** \file redis.h *
*
* \brief 
*
* TODO �����������
*
* \author zade(zhaohongchao@gmail.com)
*
* \date 2013��7��16��
*
* \version 1.0
*
* \since 1.0
*
* &copy; ��Ȩ����, 2000-2011, �ٶȿƼ����޹�˾��������Ȩ��
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
