/** \file ae.h *
*
* \brief 
*
* TODO 
*
* \author zade(zhaohongchao@gmail.com)
*
* \date 2013Äê7ÔÂ17ÈÕ
*
* \version 1.0
*
* \since 1.0
*/
#ifndef _REDIS_REDISCPP_AE_H_
#define _REDIS_REDISCPP_AE_H_

// replace with c11 <functional>
#include <boost/function.hpp>
#include <vector>
#include <sys/time.h>

namespace redis{

	enum {
		AE_OK =  0,
		AE_ERR = -1,

		AE_NONE = 0,
		AE_READABLE = 1,
		AE_WRITABLE = 2,

		AE_FILE_EVENTS = 1,
		AE_TIME_EVENTS = 2,
		AE_ALL_EVENTS = (AE_FILE_EVENTS|AE_TIME_EVENTS),
		AE_DONT_WAIT = 4,

		AE_NOMORE = -1
	};

	struct FiredEvent{
		int fd;
		int mask;
		FiredEvent(int fd = 0,int mask = 0)
			:fd(fd),mask(mask)
		{}
	};

	typedef std::vector<FiredEvent> FiredEventVector_t;

	class IEvent{
	public:
		virtual ~IEvent(){}
	};

	class EventLoop;

	class FileEvent : public IEvent{
	public:
		int mask;
		FileEvent()
			:mask(AE_NONE)
		{}
		virtual ~FileEvent(){}
		virtual void onRead(EventLoop* eventLoop, int fd, int mask){}
		virtual void OnWrite(EventLoop* eventLoop, int fd, int mask){}
		virtual bool isSameForIO() const { return false;}
	};

	typedef std::vector<FileEvent*> FileEventVector_t;

	class SokcetCtrl{
		struct Impl;
		Impl* impl;
		FileEventVector_t events;
		FiredEventVector_t fired;
	public:
		SokcetCtrl();
		~SokcetCtrl();
		int		init(int size);
		int		addEvent(int fd, int mask);
		void	delEvent(int fd, int delmask);
		int		poll(timeval* tvp,int maxfd);
		const char* getName() const;

		FiredEventVector_t& getFiredEvents(){
			return this->fired;
		}

		FileEventVector_t& getFileEvents(){
			return this->events;
		}

		int getSetSize() const{
			return static_cast<int>(this->events.size());
		}
	};

	class TimeEvent : public IEvent{
	public:
		long id;
		long when_sec;
		long when_ms;
		TimeEvent()
			:id(0),when_sec(0),when_ms(0)
		{}
		virtual int onTimer(EventLoop* eventLoop, long id){
			return 0;
		}
		virtual void onFinalizer(EventLoop* eventLoop){}
	};

	typedef std::vector<TimeEvent*> TimeEventVector_t;

	typedef boost::function<void(EventLoop*)> BeforeSleepProc_t;

	class EventLoop{
		int maxfd;
		int stop_flag;
		long timeEventNextId;
		time_t lastTime;
		TimeEventVector_t timers;
		BeforeSleepProc_t beforeSleepProc;
		SokcetCtrl	sctrl;

		TimeEvent* searchNearestTimer();
		int processTimeEvents();
	public:
		EventLoop();
		~EventLoop();

		int init(int size);
		void stop();
		int createFileEvent(FileEvent* event,int fd);
		void deleteFileEvent(int fd, int mask);
		int getFileEvents(int fd);
		long createTimeEvent(TimeEvent* event,long milliseconds);
		int deleteTimeEvent(long id);
		int processEvents(int flags);
		void main();
		void setBeforeSleepProc(const BeforeSleepProc_t& sleepProc){
			this->beforeSleepProc = sleepProc;
		}
		const char* getName() const{
			return this->sctrl.getName();
		}
		//////////////////////////////////////////////////////////////////////////
		static int wait(int fd,int mask,long milliseconds);
	};

}
#endif // _REDIS_REDISCPP_AE_H_
