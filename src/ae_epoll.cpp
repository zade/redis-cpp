/************************************************************************/
/*                                                                      */
/************************************************************************/
#include "ae.h"
#include <sys/epoll.h>

struct SokcetCtrl::Impl{
	int epfd;
	std::vector<epoll_event> events;
};

SokcetCtrl::SokcetCtrl(){
	for (size_t i = 0 ; i < this->events.size(); ++ i){
		delete this->events[i];
	}
	this->impl = new Impl();
}

SokcetCtrl::~SokcetCtrl(){
	::close(this->impl->epfd);
	delete this->impl;
}

int SokcetCtrl::init(int size){
	this->events.resize(size);
	this->fired.resize(size);
	this->impl->events.resize(size);
	this->impl->epfd = epoll_create(1024);
	if (this->impl->epfd == -1){
		return -1;
	}
	return 0;
}

int SokcetCtrl::addEvent(int fd, int mask){
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = this->events[fd]->mask == AE_NONE ?  EPOLL_CTL_ADD : EPOLL_CTL_MOD;
	struct epoll_event ee;
    ee.events = 0;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    mask |= this->events[fd]->mask; /* Merge old events */
	if (mask & AE_READABLE) {
		ee.events |= EPOLLIN;
	}
	if (mask & AE_WRITABLE) {
		ee.events |= EPOLLOUT;
	}
	if (epoll_ctl(this->impl->epfd,op,fd,&ee) == -1){
		return -1;
	}
    return 0;
}

void SokcetCtrl::delEvent(int fd, int delmask){
    struct epoll_event ee;
    int mask = this->events[fd]->mask & (~delmask);

    ee.events = 0;
	if (mask & AE_READABLE) {
		ee.events |= EPOLLIN;
	}
	if (mask & AE_WRITABLE) {
		ee.events |= EPOLLOUT;
	}
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (mask != AE_NONE) {
        epoll_ctl(this->impl->epfd,EPOLL_CTL_MOD,fd,&ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(this->impl->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
}

int SokcetCtrl::poll(timeval* tvp,int ){
	int numevents = 0;
	
	int retval = epoll_wait(this->impl->epfd,&this->impl->events[0],
		static_cast<int>(this->events.size()),tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
	if (retval > 0) {
		numevents = retval;
		struct epoll_event* begin = &this->impl->events[0];
		for (int j = 0; j < numevents; j++) {
			int mask = 0;
			struct epoll_event* e = begin + j;

			if (e->events & EPOLLIN) mask |= AE_READABLE;
			if (e->events & EPOLLOUT) mask |= AE_WRITABLE;
			if (e->events & EPOLLERR) mask |= AE_WRITABLE;
			if (e->events & EPOLLHUP) mask |= AE_WRITABLE;

			this->fired[j].fd = e->data.fd;
			this->fired[j].mask = mask;
		}
	}
	return numevents;
}

const char* SokcetCtrl::getName() const{
	return "epoll";
}
