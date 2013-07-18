/************************************************************************/
/*                                                                      */
/************************************************************************/
#include <sys/epoll.h>

struct PollCtrl : public PollCtrlBase{
	int epfd;
	std::vector<epoll_event> evts;
	
	PollCtrl()
		:epfd(-1)
	{}
	
	~PollCtrl(){
		if (this->epfd != -1){
			::close(this->epfd);
		}
	}
	
	int init(int size){
		this->events.resize(size);
		this->fired.resize(size);
		this->evts.resize(size);
		this->epfd = epoll_create(1024);
		if (this->epfd == -1){
			return -1;
		}
		return 0;
	}
	
	int addEvent(int fd, int mask){
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
		if (epoll_ctl(this->epfd,op,fd,&ee) == -1){
			return -1;
		}
		return 0;
	}
	
	void delEvent(int fd, int delmask){
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
			epoll_ctl(this->epfd,EPOLL_CTL_MOD,fd,&ee);
		} else {
			/* Note, Kernel < 2.6.9 requires a non null event pointer even for
			* EPOLL_CTL_DEL. */
			epoll_ctl(this->epfd,EPOLL_CTL_DEL,fd,&ee);
		}
	}
	
	int poll(timeval* tvp,int maxfd){
		int numevents = 0;

		int retval = epoll_wait(this->epfd,&this->evts[0],
			static_cast<int>(this->events.size()),tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
		if (retval > 0) {
			numevents = retval;
			struct epoll_event* begin = &this->evts[0];
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
	
	const char* getName() const{
		return "epoll";
	}
};
