/************************************************************************/
/*                                                                      */
/************************************************************************/
#ifndef _REDIS_STRUCTS_H_
#define _REDIS_STRUCTS_H_

#include "zmalloc.h"
#include <vector>

namespace redis{
	
	typedef std::vector<char,ZAllocator<char> > sds_t;


}


#endif //_REDIS_STRUCTS_H_
