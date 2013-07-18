/** \file ae.cpp
*
* \brief 
*
* TODO 
*
* \author zade(zhaohongchao@gmail.com)
*
* \date 2013Äê7ÔÂ17ÈÕ
*
* \version 1.0.0 
*
* \since 1.0.0
*/
#include "ae.h"
#include "config.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <errno.h>

namespace redis { namespace{
	void getTime(long *seconds, long *milliseconds)
	{
		struct timeval tv;

		gettimeofday(&tv, NULL);
		*seconds = tv.tv_sec;
		*milliseconds = tv.tv_usec/1000;
	}

	void addMillisecondsToNow(long milliseconds, long *sec, long *ms) {
		long cur_sec, cur_ms, when_sec, when_ms;
		getTime(&cur_sec, &cur_ms);
		when_sec = cur_sec + milliseconds/1000;
		when_ms = cur_ms + milliseconds%1000;
		if (when_ms >= 1000) {
			when_sec ++;
			when_ms -= 1000;
		}
		*sec = when_sec;
		*ms = when_ms;
	}
}

struct PollCtrlBase{
	FileEventVector_t events;
	FiredEventVector_t fired;
	
	~PollCtrlBase(){
		for (size_t i = 0 ; i < this->events.size(); ++ i){
			delete this->events[i];
		}
	}

	FiredEventVector_t& getFiredEvents(){
		return this->fired;
	}

	FileEventVector_t& getFileEvents(){
		return this->events;
	}
};
/* Include the best multiplexing layer supported by this system.
* The following should be ordered by performances, descending. */
#ifdef HAVE_EVPORT
#include "ae_evport.cpp"
#else
#ifdef HAVE_EPOLL
#include "ae_epoll.cpp"
#else
#ifdef HAVE_KQUEUE
#include "ae_kqueue.cpp"
#else
#include "ae_select.cpp"
#endif
#endif
#endif


EventLoop::EventLoop()
:maxfd(-1),
stop_flag(0),
timeEventNextId(0),
lastTime(time(NULL))
{
	this->poll = new PollCtrl();
}

EventLoop::~EventLoop(){
	for (size_t i = 0 ; i < this->timers.size(); ++ i){
		delete this->timers[i];
	}
	delete this->poll;
}

int EventLoop::init(int size){
	return this->poll->init(size);
}

const char* EventLoop::getName() const{
	return this->poll->getName();
}

void EventLoop::stop(){
	this->stop_flag = 1;
}

int EventLoop::createFileEvent(FileEvent* event,int fd){
	FileEventVector_t& events = this->poll->getFileEvents();
	if (fd >= static_cast<int>(events.size())) {
		errno = ERANGE;
		return AE_ERR;
	}
	//events[fd]==NULL is ok
	delete events[fd];
	events[fd] = event;

	if (this->poll->addEvent(fd, event->mask) == -1){
		return AE_ERR;
	}
	if (fd > this->maxfd){
		this->maxfd = fd;
	}
	return AE_OK;
}

void EventLoop::deleteFileEvent(int fd, int mask){
	FileEventVector_t& events = this->poll->getFileEvents();
	if (fd >= static_cast<int>(events.size())) {
		return ;
	}
	FileEvent *fe = events[fd];
	if (fe->mask != AE_NONE) {
		fe->mask = fe->mask & (~mask);
		if (fd == this->maxfd && fe->mask == AE_NONE) {
			/* Update the max fd */
			int j = this->maxfd - 1;
			for (; j >= 0; -- j){
				if (events[j]->mask != AE_NONE) {
					break;
				}
			}
			this->maxfd = j;
		}
		this->poll->delEvent(fd, mask);
	}

	if (fe->mask == AE_NONE){
		delete fe;
		events[fd] = NULL;
	}
}

int EventLoop::getFileEvents(int fd){
	FileEventVector_t& events = this->poll->getFileEvents();
	if (fd >= static_cast<int>(events.size())) {
		return 0;
	}
	FileEvent *fe = events[fd];
	return fe->mask;
}

long EventLoop::createTimeEvent(TimeEvent* te,long milliseconds){
	long id = this->timeEventNextId ++;
	te->id = id;
	addMillisecondsToNow(milliseconds,&te->when_sec,&te->when_ms);
	this->timers.push_back(te);
	return id;
}

int EventLoop::deleteTimeEvent(long id){
	for (TimeEventVector_t::iterator itr = this->timers.begin(), end = this->timers.end();
		itr != end; ++ itr)
	{
		TimeEvent* te = *itr;
		if (te->id == id){
			te->onFinalizer(this);
			delete te;
			this->timers.erase(itr);
			return AE_OK;
		}
	}
	return AE_ERR;
}

int EventLoop::processEvents(int flags){
	/* Nothing to do? return ASAP */
	if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) {
		return 0;
	}

	int processed = 0;
	/* Note that we want call select() even if there are no
	* file events to process as long as we want to process time
	* events, in order to sleep until the next time event is ready
	* to fire. */
	if (this->maxfd != -1 ||
		((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT))) 
	{
		TimeEvent *shortest = NULL;
		struct timeval tv;
		struct timeval* tvp = NULL;

		if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT))
			shortest = this->searchNearestTimer();
		if (shortest) {
			long now_sec, now_ms;

			/* Calculate the time missing for the nearest
			* timer to fire. */
			getTime(&now_sec, &now_ms);
			tvp = &tv;
			tvp->tv_sec = shortest->when_sec - now_sec;
			if (shortest->when_ms < now_ms) {
				tvp->tv_usec = ((shortest->when_ms+1000) - now_ms)*1000;
				tvp->tv_sec --;
			} else {
				tvp->tv_usec = (shortest->when_ms - now_ms)*1000;
			}
			if (tvp->tv_sec < 0) tvp->tv_sec = 0;
			if (tvp->tv_usec < 0) tvp->tv_usec = 0;
		} else {
			/* If we have to check for events but need to return
			* ASAP because of AE_DONT_WAIT we need to set the timeout
			* to zero */
			if (flags & AE_DONT_WAIT) {
				tv.tv_sec = tv.tv_usec = 0;
				tvp = &tv;
			} else {
				/* Otherwise we can block */
				tvp = NULL; /* wait forever */
			}
		}

		FileEventVector_t& events = this->poll->getFileEvents();
		FiredEventVector_t& fired = this->poll->getFiredEvents();
		int numevents = this->poll->poll(tvp,this->maxfd);
		for (int j = 0; j < numevents; j ++) {
			FileEvent *fe = events[fired[j].fd];
			int mask = fired[j].mask;
			int fd = fired[j].fd;
			int rfired = 0;

			/* note the fe->mask & mask & ... code: maybe an already processed
			* event removed an element that fired and we still didn't
			* processed, so we check if the event is still valid. */
			if (fe->mask & mask & AE_READABLE) {
				rfired = 1;
				fe->onRead(this,fd,mask);
			}
			if (fe->mask & mask & AE_WRITABLE) {
				if (!rfired || !fe->isSameForIO()){
					fe->OnWrite(this,fd,mask);
				}
			}
			processed ++;
		}
	}
	/* Check time events */
	if (flags & AE_TIME_EVENTS){
		processed += this->processTimeEvents();
	}

	return processed; /* return the number of processed file/time events */
}

int EventLoop::wait(int fd,int mask,long milliseconds){
	struct pollfd pfd;
	int retmask = 0, retval;

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	if (mask & AE_READABLE) pfd.events |= POLLIN;
	if (mask & AE_WRITABLE) pfd.events |= POLLOUT;

	if ((retval = ::poll(&pfd, 1, milliseconds))== 1) {
		if (pfd.revents & POLLIN) retmask |= AE_READABLE;
		if (pfd.revents & POLLOUT) retmask |= AE_WRITABLE;
		if (pfd.revents & POLLERR) retmask |= AE_WRITABLE;
		if (pfd.revents & POLLHUP) retmask |= AE_WRITABLE;
		return retmask;
	} else {
		return retval;
	}
}

void EventLoop::main(){
	this->stop_flag = 0;
	while (!this->stop_flag) {
		if (this->beforeSleepProc){
			this->beforeSleepProc(this);
		}
		this->processEvents(AE_ALL_EVENTS);
	}
}
//////////////////////////////////////////////////////////////////////////
TimeEvent* EventLoop::searchNearestTimer(){
	TimeEvent* nearest = NULL;
	if (!this->timers.empty()){
		TimeEventVector_t::iterator itr = this->timers.begin();
		nearest = *itr;
		++ itr;
		for (TimeEventVector_t::iterator end = this->timers.end();itr != end; ++ itr){
			TimeEvent* te = *itr;
			if (te->when_sec < nearest->when_sec ||
				(te->when_sec == nearest->when_sec &&te->when_ms < nearest->when_ms))
			{
				nearest = te;
			}
		}
	}
	return nearest;
}

int EventLoop::processTimeEvents(){
	time_t now = time(NULL);
	/* If the system clock is moved to the future, and then set back to the
	* right value, time events may be delayed in a random way. Often this
	* means that scheduled operations will not be performed soon enough.
	*
	* Here we try to detect system clock skews, and force all the time
	* events to be processed ASAP when this happens: the idea is that
	* processing events earlier is less dangerous than delaying them
	* indefinitely, and practice suggests it is. */
	if (now < this->lastTime) {
		for (TimeEventVector_t::iterator itr = this->timers.begin(), end = this->timers.end();
			itr != end; ++ itr)
		{
			(*itr)->when_sec = 0;
		}
	}
	this->lastTime = now;

	int processed = 0;
	long maxId = this->timeEventNextId-1;
	for (TimeEventVector_t::iterator itr = this->timers.begin(), end = this->timers.end();
		itr != end; ++ itr)
	{
		TimeEvent* te = *itr;
		if (te->id > maxId) {
			continue;
		}

		long now_sec = 0;
		long now_ms = 0;
		getTime(&now_sec, &now_ms);
		if (now_sec > te->when_sec ||
			(now_sec == te->when_sec && now_ms >= te->when_ms))
		{
			long id = te->id;
			int retval = te->onTimer(this, id);
			processed++;
			/* After an event is processed our time event list may
			* no longer be the same, so we restart from head.
			* Still we make sure to don't process events registered
			* by event handlers itself in order to don't loop forever.
			* To do so we saved the max ID we want to handle.
			*
			* FUTURE OPTIMIZATIONS:
			* Note that this is NOT great algorithmically. Redis uses
			* a single time event so it's not a problem but the right
			* way to do this is to add the new elements on head, and
			* to flag deleted elements in a special way for later
			* deletion (putting references to the nodes to delete into
			* another linked list). */
			if (retval != AE_NOMORE) {
				addMillisecondsToNow(retval,&te->when_sec,&te->when_ms);
			} else {
				this->deleteTimeEvent(id);
				end = this->timers.end();
			}
			itr = this->timers.begin();
		}
	}
	return processed;
}

}
