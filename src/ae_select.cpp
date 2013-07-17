/************************************************************************/
/*                                                                      */
/************************************************************************/
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct SokcetCtrl::Impl{
	fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds;
};

SokcetCtrl::SokcetCtrl(){
	for (size_t i = 0 ; i < this->events.size(); ++ i){
		delete this->events[i];
	}
	this->impl = new Impl();
}

SokcetCtrl::~SokcetCtrl(){
	delete this->impl;
}

int SokcetCtrl::init(int size){
	this->events.resize(size);
	this->fired.resize(size);
	FD_ZERO(&this->impl->rfds);
	FD_ZERO(&this->impl->wfds);
	return 0; 
}

int SokcetCtrl::addEvent(int fd, int mask){
	if (mask & AE_READABLE) FD_SET(fd,&this->impl->rfds);
	if (mask & AE_WRITABLE) FD_SET(fd,&this->impl->wfds);
	return 0;
}

void SokcetCtrl::delEvent(int fd, int delmask){
	if (mask & AE_READABLE) FD_CLR(fd,&this->impl->rfds);
	if (mask & AE_WRITABLE) FD_CLR(fd,&this->impl->wfds);
}

int SokcetCtrl::poll(timeval* tvp,int maxfd){
	int numevents = 0;

	memcpy(&this->impl->_rfds,&this->impl->rfds,sizeof(fd_set));
	memcpy(&this->impl->_wfds,&this->impl->wfds,sizeof(fd_set));

	int retval = select(maxfd+1,&this->impl->_rfds,&this->impl->_wfds,NULL,tvp);
	if (retval > 0) {
		for (int j = 0; j <= maxfd; j++) {
			int mask = 0;
			aeFileEvent *fe = &this->events[j];

			if (fe->mask == AE_NONE) continue;
			if (fe->mask & AE_READABLE && FD_ISSET(j,&this->impl->_rfds))
				mask |= AE_READABLE;
			if (fe->mask & AE_WRITABLE && FD_ISSET(j,&this->impl->_wfds))
				mask |= AE_WRITABLE;
			this->fired[numevents].fd = j;
			this->fired[numevents].mask = mask;
			numevents++;
		}
	}
	return numevents;
}

const char* SokcetCtrl::getName() const{
	return "select";
}

