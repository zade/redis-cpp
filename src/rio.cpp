/************************************************************************/
/*                                                                      */
/************************************************************************/
#include "config.h"
#include "rio.h"
#include "util.h"
#include "debug.h"


namespace redis { namespace{

	struct BufferedRIO : public RIO {
		sds_t* buf;
		off_t  pos;

		BufferedRIO(sds_t* buf)
			:buf(buf),pos(0)
		{}

		virtual size_t read(void* buf, size_t size){
			if(this->buf->size() - this->pos < size){
				return 0;
			}
			memcpy(buf,&(*this->buf->begin()) + this->pos, size);
			this->pos += size;
			return 1;
		}
		virtual size_t write(const void* buf, size_t size){
			const char* begin = reinterpret_cast<const char*>(buf);
			this->buf->insert(this->buf->end(),begin,begin + size);
			this->pos += size;
			return 1;
		}
		virtual off_t  tell() const{
			return this->pos;
		}
		virtual void setAutoSyncBytes(off_t){
			redisAssert(0);
		}
	};

	struct FileRIO : public RIO {
		FILE* fp;
		off_t buffered; /* Bytes written since last fsync. */
		off_t autosync; /* fsync after 'autosync' bytes written. */

		FileRIO(FILE* fp)
			:fp(fp),buffered(0),autosync(0)
		{}

		virtual size_t read(void* buf, size_t len){
			return fread(buf,len,1,this->fp);
		}
		virtual size_t write(const void* buf, size_t len){
			size_t retval = retval = fwrite(buf,len,1,this->fp);
			this->buffered += len;

			if (this->autosync && this->buffered >= this->autosync){
				aof_fsync(fileno(this->fp));
				this->buffered = 0;
			}
			return retval;
		}
		virtual off_t  tell() const{
			return ftello(this->fp);
		}
		virtual void setAutoSyncBytes(off_t bytes){
			this->autosync = bytes;
		}
	};
}

RIO* rioInitWithBuffer(sds_t* buf){
	return new BufferedRIO(buf);
}

RIO* rioInitWithFile(FILE* fp){
	return new FileRIO(fp);
}


/* ------------------------------ Higher level interface ---------------------------
 * The following higher level functions use lower level RIO.c functions to help
 * generating the Redis protocol for the Append Only File. */

/* Write multi bulk count in the format: "*<count>\r\n". */
size_t rioWriteBulkCount(RIO *r, char prefix, int count) {
    char cbuf[128];
    int clen;

    cbuf[0] = prefix;
    clen = 1+ll2string(cbuf+1,sizeof(cbuf)-1,count);
    cbuf[clen++] = '\r';
    cbuf[clen++] = '\n';
	if (r->write(cbuf,clen) == 0) {
		return 0;
	}
    return clen;
}

/* Write binary-safe string in the format: "$<count>\r\n<payload>\r\n". */
size_t rioWriteBulkString(RIO *r, const char *buf, size_t len) {
    size_t nwritten;

	if ((nwritten = rioWriteBulkCount(r,'$',len)) == 0) {
		return 0;
	}
	if (len > 0 && r->write(buf,len) == 0) {
		return 0;
	}
	if (r->write("\r\n",2) == 0) {
		return 0;
	}
    return nwritten+len+2;
}

/* Write a long long value in format: "$<count>\r\n<payload>\r\n". */
size_t rioWriteBulkLongLong(RIO *r, long l) {
    char lbuf[32];
    unsigned int llen;

    llen = ll2string(lbuf,sizeof(lbuf),l);
    return rioWriteBulkString(r,lbuf,llen);
}

/* Write a double value in the format: "$<count>\r\n<payload>\r\n" */
size_t rioWriteBulkDouble(RIO *r, double d) {
    char dbuf[128];
    unsigned int dlen;

    dlen = snprintf(dbuf,sizeof(dbuf),"%.17g",d);
    return rioWriteBulkString(r,dbuf,dlen);
}

void rioGenericUpdateChecksum(RIO *r, const void *buf, size_t len) {
	r->cksum = crc64(r->cksum,reinterpret_cast<const unsigned char*>(buf),len);
}

}

