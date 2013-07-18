/************************************************************************/
/*                                                                      */
/************************************************************************/
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct PollCtrl : public PollCtrlBase{
	fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds;

	PollCtrl()
	{}

	~PollCtrl(){
	}

	int init(int size){
		this->events.resize(size);
		this->fired.resize(size);
		FD_ZERO(&this->rfds);
		FD_ZERO(&this->wfds);
		return 0; 
	}

	int addEvent(int fd, int mask){
		if (mask & AE_READABLE) FD_SET(fd,&this->rfds);
		if (mask & AE_WRITABLE) FD_SET(fd,&this->wfds);
		return 0;
	}

	void delEvent(int fd, int mask){
		if (mask & AE_READABLE) FD_CLR(fd,&this->rfds);
		if (mask & AE_WRITABLE) FD_CLR(fd,&this->wfds);
	}

	int poll(timeval* tvp,int maxfd){
		int numevents = 0;

		memcpy(&this->_rfds,&this->rfds,sizeof(fd_set));
		memcpy(&this->_wfds,&this->wfds,sizeof(fd_set));

		int retval = select(maxfd+1,&this->_rfds,&this->_wfds,NULL,tvp);
		if (retval > 0) {
			for (int j = 0; j <= maxfd; j++) {
				int mask = 0;
				FileEvent *fe = &this->events[j];

				if (fe->mask == AE_NONE) continue;
				if (fe->mask & AE_READABLE && FD_ISSET(j,&this->_rfds))
					mask |= AE_READABLE;
				if (fe->mask & AE_WRITABLE && FD_ISSET(j,&this->_wfds))
					mask |= AE_WRITABLE;
				this->fired[numevents].fd = j;
				this->fired[numevents].mask = mask;
				numevents++;
			}
		}
		return numevents;
	}

	const char* getName() const{
		return "select";
	}
};

