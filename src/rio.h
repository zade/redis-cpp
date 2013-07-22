/************************************************************************/
/*                                                                      */
/************************************************************************/

#ifndef _REDIS_RIO_H_
#define _REDIS_RIO_H_

#include "structs.h"

#include <stdio.h>
#include <stdint.h>


namespace redis{
	
	class RIO{
	public:
		uint64_t cksum;
		
		RIO()
			:cksum(0)
		{}

		virtual ~RIO(){}

		virtual size_t read(void* buf, size_t size) = 0;
		virtual size_t write(const void* buf, size_t size) = 0;
		virtual off_t  tell() const = 0;
		virtual void setAutoSyncBytes(off_t bytes) = 0;
		virtual void updateChecksum(const void* buf, size_t size){}
	};

	RIO* rioInitWithBuffer(sds_t* buf);
	RIO* rioInitWithFile(FILE* fp);

	size_t rioWriteBulkCount(RIO *r, char prefix, int count);
	size_t rioWriteBulkString(RIO *r, const char *buf, size_t len);
	size_t rioWriteBulkLongLong(RIO *r, long l);
	size_t rioWriteBulkDouble(RIO *r, double d);

	void rioGenericUpdateChecksum(RIO *r, const void *buf, size_t len);
}

#endif //_REDIS_RIO_H_
