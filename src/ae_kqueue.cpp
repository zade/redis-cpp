/************************************************************************/
/*                                                                      */
/************************************************************************/

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

struct PollCtrl : public PollCtrlBase{
	int kqfd;
	std::vector<kevent> events;

	PollCtrl()
		:kqfd(-1)
	{}

	~PollCtrl(){
		if (this->kqfd != -1){
			::close(this->kqfd);
		}
	}

	int init(int size){
		this->events.resize(size);
		this->fired.resize(size);
		this->events.resize(size);
		this->kqfd = kqueue();
		if (this->kqfd == -1) {
			return -1;
		}
		return 0;
	}

	int addEvent(int fd, int mask){
		struct kevent ke;

		if (mask & AE_READABLE) {
			EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
			if (kevent(this->kqfd, &ke, 1, NULL, 0, NULL) == -1) {
				return -1;
			}
		}
		if (mask & AE_WRITABLE) {
			EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
			if (kevent(this->kqfd, &ke, 1, NULL, 0, NULL) == -1) {
				return -1;
			}
		}
		return 0;
	}

	void delEvent(int fd, int delmask){
		struct kevent ke;

		if (mask & AE_READABLE) {
			EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
			kevent(this->kqfd, &ke, 1, NULL, 0, NULL);
		}
		if (mask & AE_WRITABLE) {
			EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
			kevent(this->kqfd, &ke, 1, NULL, 0, NULL);
		}
	}

	int poll(timeval* tvp,int maxfd){
		int retval = 0;
		int numevents = 0;

		int setsize = static_cast<int>(this->events.size());
		if (tvp != NULL) {
			struct timespec timeout;
			timeout.tv_sec = tvp->tv_sec;
			timeout.tv_nsec = tvp->tv_usec * 1000;
			retval = kevent(this->kqfd, NULL, 0, &this->evts[0], setsize,&timeout);
		} else {
			retval = kevent(this->kqfd, NULL, 0, &this->evts[0], setsize,NULL);
		}

		if (retval > 0) {
			numevents = retval;
			struct kevent* begin = &this->evts[0];
			for(int j = 0; j < numevents; j++) {
				int mask = 0;
				struct kevent *e = begin + j;
				if (e->filter == EVFILT_READ) mask |= AE_READABLE;
				if (e->filter == EVFILT_WRITE) mask |= AE_WRITABLE;
				this->fired[j].fd = e->ident; 
				this->fired[j].mask = mask;           
			}
		}
		return numevents;
	}

	const char* getName() const{
		return "kqueue";
	}
};


