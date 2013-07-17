/************************************************************************/
/*                                                                      */
/************************************************************************/
#include <assert.h>
#include <errno.h>
#include <port.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#define MAX_EVENT_BATCHSZ 512

static int evport_debug = 0;

struct SokcetCtrl::Impl{
	int     portfd;                             /* event port */
	int     npending;                           /* # of pending fds */
	int     pending_fds[MAX_EVENT_BATCHSZ];     /* pending fds */
	int     pending_masks[MAX_EVENT_BATCHSZ];   /* pending fds' masks */

	int lookupPending(int fd) {
		for (int i = 0; i < this->npending; i++) {
			if (this->pending_fds[i] == fd){
				return (i);
			}
		}
		return (-1);
	}

	/*
	* Helper function to invoke port_associate for the given fd and mask.
	*/
	static int associate(const char *where, int portfd, int fd, int mask) {
		int events = 0;
		int rv, err;

		if (mask & AE_READABLE)
			events |= POLLIN;
		if (mask & AE_WRITABLE)
			events |= POLLOUT;

		if (evport_debug)
			fprintf(stderr, "%s: port_associate(%d, 0x%x) = ", where, fd, events);

		rv = port_associate(portfd, PORT_SOURCE_FD, fd, events,
			(void *)(uintptr_t)mask);
		err = errno;

		if (evport_debug)
			fprintf(stderr, "%d (%s)\n", rv, rv == 0 ? "no error" : strerror(err));

		if (rv == -1) {
			fprintf(stderr, "%s: port_associate: %s\n", where, strerror(err));

			if (err == EAGAIN)
				fprintf(stderr, "aeApiAssociate: event port limit exceeded.");
		}

		return rv;
	}
};

SokcetCtrl::SokcetCtrl(){
	this->impl = new Impl();
}

SokcetCtrl::~SokcetCtrl(){
	for (size_t i = 0 ; i < this->events.size(); ++ i){
		delete this->events[i];
	}
	close(this->impl->portfd);
	delete this->impl;
}

int SokcetCtrl::init(int size){
	this->impl->portfd = port_create();
	if (this->impl->portfd == -1) {
		return -1;
	}

	this->impl->npending = 0;

	for (i = 0; i < MAX_EVENT_BATCHSZ; i++) {
		this->impl->pending_fds[i] = -1;
		this->impl->pending_masks[i] = AE_NONE;
	}

	return 0;
}

int SokcetCtrl::addEvent(int fd, int mask){
	if (evport_debug)
		fprintf(stderr, "aeApiAddEvent: fd %d mask 0x%x\n", fd, mask);

	/*
	* Since port_associate's "events" argument replaces any existing events, we
	* must be sure to include whatever events are already associated when
	* we call port_associate() again.
	*/
	int fullmask = mask | this->events[fd].mask;
	int pfd = this->impl->lookupPending(fd);
	if (pfd != -1) {
		/*
		* This fd was recently returned from aeApiPoll.  It should be safe to
		* assume that the consumer has processed that poll event, but we play
		* it safer by simply updating pending_mask.  The fd will be
		* re-associated as usual when aeApiPoll is called again.
		*/
		if (evport_debug)
			fprintf(stderr, "aeApiAddEvent: adding to pending fd %d\n", fd);
		this->impl->pending_masks[pfd] |= fullmask;
		return 0;
	}

	return (Impl::associate("addEvent", this->impl->portfd, fd, fullmask));
}

void SokcetCtrl::delEvent(int fd, int delmask){
	if (evport_debug)
		fprintf(stderr, "del fd %d mask 0x%x\n", fd, mask);

	int pfd = this->impl->lookupPending(fd);

	if (pfd != -1) {
		if (evport_debug)
			fprintf(stderr, "deleting event from pending fd %d\n", fd);

		/*
		* This fd was just returned from aeApiPoll, so it's not currently
		* associated with the port.  All we need to do is update
		* pending_mask appropriately.
		*/
		this->impl->pending_masks[pfd] &= ~mask;

		if (this->impl->pending_masks[pfd] == AE_NONE)
			this->impl->pending_fds[pfd] = -1;

		return;
	}

	/*
	* The fd is currently associated with the port.  Like with the add case
	* above, we must look at the full mask for the file descriptor before
	* updating that association.  We don't have a good way of knowing what the
	* events are without looking into the eventLoop state directly.  We rely on
	* the fact that our caller has already updated the mask in the eventLoop.
	*/

	int fullmask = this->events[fd].mask;
	if (fullmask == AE_NONE) {
		/*
		* We're removing *all* events, so use port_dissociate to remove the
		* association completely.  Failure here indicates a bug.
		*/
		if (evport_debug)
			fprintf(stderr, "delEvent: port_dissociate(%d)\n", fd);

		if (port_dissociate(this->impl->portfd, PORT_SOURCE_FD, fd) != 0) {
			perror("delEvent: port_dissociate");
			abort(); /* will not return */
		}
	} else if (Impl::associate("delEvent", this->impl->portfd, fd,fullmask) != 0) {
		/*
		* ENOMEM is a potentially transient condition, but the kernel won't
		* generally return it unless things are really bad.  EAGAIN indicates
		* we've reached an resource limit, for which it doesn't make sense to
		* retry (counter-intuitively).  All other errors indicate a bug.  In any
		* of these cases, the best we can do is to abort.
		*/
		abort(); /* will not return */
	}
}

int SokcetCtrl::poll(timeval* tvp,int){
	port_event_t event[MAX_EVENT_BATCHSZ];
	/*
	* If we've returned fd events before, we must re-associate them with the
	* port now, before calling port_get().  See the block comment at the top of
	* this file for an explanation of why.
	*/
	for (int i = 0; i < this->impl->npending; i++) {
		if (this->impl->pending_fds[i] == -1)
			/* This fd has since been deleted. */
			continue;

		if (Impl::associate("poll", this->impl->portfd,
			this->impl->pending_fds[i], this->impl->pending_masks[i]) != 0) {
				/* See aeApiDelEvent for why this case is fatal. */
				abort();
		}

		this->impl->pending_masks[i] = AE_NONE;
		this->impl->pending_fds[i] = -1;
	}

	this->impl->npending = 0;

	timespec timeout;
	timespec *tsp = NULL;
	if (tvp != NULL) {
		timeout.tv_sec = tvp->tv_sec;
		timeout.tv_nsec = tvp->tv_usec * 1000;
		tsp = &timeout;
	} else {
		tsp = NULL;
	}

	/*
	* port_getn can return with errno == ETIME having returned some events (!).
	* So if we get ETIME, we check nevents, too.
	*/
	int nevents = 1;
	if (port_getn(this->impl->portfd, event, MAX_EVENT_BATCHSZ, &nevents,
		tsp) == -1 && (errno != ETIME || nevents == 0)) 
	{
		if (errno == ETIME || errno == EINTR)
			return 0;

		/* Any other error indicates a bug. */
		perror("poll: port_get");
		abort();
	}

	state->npending = nevents;

	for (int i = 0; i < nevents; i++) {
		int mask = 0;
		if (event[i].portev_events & POLLIN)
			mask |= AE_READABLE;
		if (event[i].portev_events & POLLOUT)
			mask |= AE_WRITABLE;

		this->fired[i].fd = event[i].portev_object;
		this->fired[i].mask = mask;

		if (evport_debug)
			fprintf(stderr, "aeApiPoll: fd %d mask 0x%x\n",
			(int)event[i].portev_object, mask);

		this->impl->pending_fds[i] = event[i].portev_object;
		this->impl->pending_masks[i] = (uintptr_t)event[i].portev_user;
	}

	return nevents;
}

const char* SokcetCtrl::getName() const{
	return "evport";
}
