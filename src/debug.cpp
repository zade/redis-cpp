/************************************************************************/
/*                                                                      */
/************************************************************************/
#include "debug.h"
#include "log.h"
#include "consts.h"

#include <sys/time.h>

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#include <ucontext.h>
#include <fcntl.h>
#endif /* HAVE_BACKTRACE */

namespace redis{ namespace{
	int bug_report_start = 0 ;
}

void bugReportStart(void) {
	if (bug_report_start == 0) {
		redisLog(REDIS_WARNING,
			"\n\n=== REDIS BUG REPORT START: Cut & paste starting from here ===");
		bug_report_start = 1;
	}
}

void _redisAssert(const char *estr, const char *file, int line) {
	bugReportStart();
	redisLog(REDIS_WARNING,"=== ASSERTION FAILED ===");
	redisLog(REDIS_WARNING,"==> %s:%d '%s' is not true",file,line,estr);
#ifdef HAVE_BACKTRACE
	//server.assert_failed = estr;
	//server.assert_file = file;
	//server.assert_line = line;
	redisLog(REDIS_WARNING,"(forcing SIGSEGV to print the bug report.)");
#endif
	*((char*)-1) = 'x';
}

void _redisPanic(char *msg, char *file, int line) {
	bugReportStart();
	redisLog(REDIS_WARNING,"------------------------------------------------");
	redisLog(REDIS_WARNING,"!!! Software Failure. Press left mouse button to continue");
	redisLog(REDIS_WARNING,"Guru Meditation: %s #%s:%d",msg,file,line);
#ifdef HAVE_BACKTRACE
	redisLog(REDIS_WARNING,"(forcing SIGSEGV in order to print the stack trace)");
#endif
	redisLog(REDIS_WARNING,"------------------------------------------------");
	*((char*)-1) = 'x';
}

void redisLogHexDump(int level, const char *descr, const void *value, size_t len) {
	char buf[65];
	const unsigned char *v = reinterpret_cast<const unsigned char *>(value);
	const char charset[] = "0123456789abcdef";

	redisLog(level,"%s (hexdump):", descr);
	char* b = buf;
	while(len) {
		b[0] = charset[(*v)>>4];
		b[1] = charset[(*v)&0xf];
		b[2] = '\0';
		b += 2;
		len--;
		v++;
		if (b-buf == 64 || len == 0) {
			redisLogRaw(level|REDIS_LOG_RAW,buf);
			b = buf;
		}
	}
	redisLogRaw(level|REDIS_LOG_RAW,"\n");
}

void watchdogScheduleSignal(int period) {
	struct itimerval it;

	/* Will stop the timer if period is 0. */
	it.it_value.tv_sec = period/1000;
	it.it_value.tv_usec = (period%1000)*1000;
	/* Don't automatically restart. */
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);
}

#ifdef HAVE_BACKTRACE
static void *getMcontextEip(ucontext_t *uc) {
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
    /* OSX < 10.6 */
    #if defined(__x86_64__)
    return (void*) uc->uc_mcontext->__ss.__rip;
    #elif defined(__i386__)
    return (void*) uc->uc_mcontext->__ss.__eip;
    #else
    return (void*) uc->uc_mcontext->__ss.__srr0;
    #endif
#elif defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
    /* OSX >= 10.6 */
    #if defined(_STRUCT_X86_THREAD_STATE64) && !defined(__i386__)
    return (void*) uc->uc_mcontext->__ss.__rip;
    #else
    return (void*) uc->uc_mcontext->__ss.__eip;
    #endif
#elif defined(__linux__)
    /* Linux */
    #if defined(__i386__)
    return (void*) uc->uc_mcontext.gregs[14]; /* Linux 32 */
    #elif defined(__X86_64__) || defined(__x86_64__)
    return (void*) uc->uc_mcontext.gregs[16]; /* Linux 64 */
    #elif defined(__ia64__) /* Linux IA64 */
    return (void*) uc->uc_mcontext.sc_ip;
    #endif
#else
    return NULL;
#endif
}

void logStackContent(void **sp) {
    for (int i = 15; i >= 0; i--) {
        if (sizeof(long) == 4)
			redisLog(REDIS_WARNING, "(%08lx) -> %08lx", sp+i, sp[i]);
        else
            redisLog(REDIS_WARNING, "(%016lx) -> %016lx", sp+i, sp[i]);
    }
}

void logRegisters(ucontext_t *uc) {
    redisLog(REDIS_WARNING, "--- REGISTERS");

/* OSX */
#if defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
  /* OSX AMD64 */
    #if defined(_STRUCT_X86_THREAD_STATE64) && !defined(__i386__)
    redisLog(REDIS_WARNING,
    "\n"
    "RAX:%016lx RBX:%016lx\nRCX:%016lx RDX:%016lx\n"
    "RDI:%016lx RSI:%016lx\nRBP:%016lx RSP:%016lx\n"
    "R8 :%016lx R9 :%016lx\nR10:%016lx R11:%016lx\n"
    "R12:%016lx R13:%016lx\nR14:%016lx R15:%016lx\n"
    "RIP:%016lx EFL:%016lx\nCS :%016lx FS:%016lx  GS:%016lx",
        uc->uc_mcontext->__ss.__rax,
        uc->uc_mcontext->__ss.__rbx,
        uc->uc_mcontext->__ss.__rcx,
        uc->uc_mcontext->__ss.__rdx,
        uc->uc_mcontext->__ss.__rdi,
        uc->uc_mcontext->__ss.__rsi,
        uc->uc_mcontext->__ss.__rbp,
        uc->uc_mcontext->__ss.__rsp,
        uc->uc_mcontext->__ss.__r8,
        uc->uc_mcontext->__ss.__r9,
        uc->uc_mcontext->__ss.__r10,
        uc->uc_mcontext->__ss.__r11,
        uc->uc_mcontext->__ss.__r12,
        uc->uc_mcontext->__ss.__r13,
        uc->uc_mcontext->__ss.__r14,
        uc->uc_mcontext->__ss.__r15,
        uc->uc_mcontext->__ss.__rip,
        uc->uc_mcontext->__ss.__rflags,
        uc->uc_mcontext->__ss.__cs,
        uc->uc_mcontext->__ss.__fs,
        uc->uc_mcontext->__ss.__gs
    );
    logStackContent((void**)uc->uc_mcontext->__ss.__rsp);
    #else
    /* OSX x86 */
    redisLog(REDIS_WARNING,
    "\n"
    "EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n"
    "EDI:%08lx ESI:%08lx EBP:%08lx ESP:%08lx\n"
    "SS:%08lx  EFL:%08lx EIP:%08lx CS :%08lx\n"
    "DS:%08lx  ES:%08lx  FS :%08lx GS :%08lx",
        uc->uc_mcontext->__ss.__eax,
        uc->uc_mcontext->__ss.__ebx,
        uc->uc_mcontext->__ss.__ecx,
        uc->uc_mcontext->__ss.__edx,
        uc->uc_mcontext->__ss.__edi,
        uc->uc_mcontext->__ss.__esi,
        uc->uc_mcontext->__ss.__ebp,
        uc->uc_mcontext->__ss.__esp,
        uc->uc_mcontext->__ss.__ss,
        uc->uc_mcontext->__ss.__eflags,
        uc->uc_mcontext->__ss.__eip,
        uc->uc_mcontext->__ss.__cs,
        uc->uc_mcontext->__ss.__ds,
        uc->uc_mcontext->__ss.__es,
        uc->uc_mcontext->__ss.__fs,
        uc->uc_mcontext->__ss.__gs
    );
    logStackContent((void**)uc->uc_mcontext->__ss.__esp);
    #endif
/* Linux */
#elif defined(__linux__)
    /* Linux x86 */
    #if defined(__i386__)
    redisLog(REDIS_WARNING,
    "\n"
    "EAX:%08lx EBX:%08lx ECX:%08lx EDX:%08lx\n"
    "EDI:%08lx ESI:%08lx EBP:%08lx ESP:%08lx\n"
    "SS :%08lx EFL:%08lx EIP:%08lx CS:%08lx\n"
    "DS :%08lx ES :%08lx FS :%08lx GS:%08lx",
        uc->uc_mcontext.gregs[11],
        uc->uc_mcontext.gregs[8],
        uc->uc_mcontext.gregs[10],
        uc->uc_mcontext.gregs[9],
        uc->uc_mcontext.gregs[4],
        uc->uc_mcontext.gregs[5],
        uc->uc_mcontext.gregs[6],
        uc->uc_mcontext.gregs[7],
        uc->uc_mcontext.gregs[18],
        uc->uc_mcontext.gregs[17],
        uc->uc_mcontext.gregs[14],
        uc->uc_mcontext.gregs[15],
        uc->uc_mcontext.gregs[3],
        uc->uc_mcontext.gregs[2],
        uc->uc_mcontext.gregs[1],
        uc->uc_mcontext.gregs[0]
    );
    logStackContent((void**)uc->uc_mcontext.gregs[7]);
    #elif defined(__X86_64__) || defined(__x86_64__)
    /* Linux AMD64 */
    redisLog(REDIS_WARNING,
    "\n"
    "RAX:%016lx RBX:%016lx\nRCX:%016lx RDX:%016lx\n"
    "RDI:%016lx RSI:%016lx\nRBP:%016lx RSP:%016lx\n"
    "R8 :%016lx R9 :%016lx\nR10:%016lx R11:%016lx\n"
    "R12:%016lx R13:%016lx\nR14:%016lx R15:%016lx\n"
    "RIP:%016lx EFL:%016lx\nCSGSFS:%016lx",
        uc->uc_mcontext.gregs[13],
        uc->uc_mcontext.gregs[11],
        uc->uc_mcontext.gregs[14],
        uc->uc_mcontext.gregs[12],
        uc->uc_mcontext.gregs[8],
        uc->uc_mcontext.gregs[9],
        uc->uc_mcontext.gregs[10],
        uc->uc_mcontext.gregs[15],
        uc->uc_mcontext.gregs[0],
        uc->uc_mcontext.gregs[1],
        uc->uc_mcontext.gregs[2],
        uc->uc_mcontext.gregs[3],
        uc->uc_mcontext.gregs[4],
        uc->uc_mcontext.gregs[5],
        uc->uc_mcontext.gregs[6],
        uc->uc_mcontext.gregs[7],
        uc->uc_mcontext.gregs[16],
        uc->uc_mcontext.gregs[17],
        uc->uc_mcontext.gregs[18]
    );
    logStackContent((void**)uc->uc_mcontext.gregs[15]);
    #endif
#else
    redisLog(REDIS_WARNING,
        "  Dumping of registers not supported for this OS/arch");
#endif
}

/* Logs the stack trace using the backtrace() call. This function is designed
 * to be called from signal handlers safely. */
void logStackTrace(ucontext_t *uc) {
    /* Open the log file in append mode. */
	int fd = Logging::instance().logFD();
	if (fd == -1) {
		return;
	}

	void *trace[100];
    /* Generate the stack trace */
    int trace_size = backtrace(trace, 100);

    /* overwrite sigaction with caller's address */
	if (getMcontextEip(uc) != NULL){
        trace[1] = getMcontextEip(uc);
	}

    /* Write symbols to log file */
    backtrace_symbols_fd(trace, trace_size, fd);

    /* Cleanup */
	if (fd != STDOUT_FILENO) {
		close(fd);
	}
}

/* Log information about the "current" client, that is, the client that is
 * currently being served by Redis. May be NULL if Redis is not serving a
 * client right now. */
void logCurrentClient(void) {
    
}

void sigsegvHandler(int sig, siginfo_t *, void *secret) {
    ucontext_t *uc = (ucontext_t*) secret;
    //sds infostring, clients;
    struct sigaction act;
    //REDIS_NOTUSED(info);

    bugReportStart();
    redisLog(REDIS_WARNING,
        "    Redis %s crashed by signal: %d", REDIS_VERSION, sig);
   
    /* Log the stack trace */
    redisLog(REDIS_WARNING, "--- STACK TRACE");
    logStackTrace(uc);

    /* Log INFO and CLIENT LIST */
    redisLog(REDIS_WARNING, "--- INFO OUTPUT");
    //infostring = genRedisInfoString("all");
    //infostring = sdscatprintf(infostring, "hash_init_value: %u\n",
    //    dictGetHashFunctionSeed());
    //redisLogRaw(REDIS_WARNING, infostring);
    redisLog(REDIS_WARNING, "--- CLIENT LIST OUTPUT");
    //clients = getAllClientsInfoString();
    //redisLogRaw(REDIS_WARNING, clients);
    //sdsfree(infostring);
    //sdsfree(clients);

    /* Log the current client */
    logCurrentClient();

    /* Log dump of processor registers */
    logRegisters(uc);

    redisLog(REDIS_WARNING,
"\n=== REDIS BUG REPORT END. Make sure to include from START to END. ===\n\n"
"       Please report the crash opening an issue on github:\n\n"
"           http://github.com/antirez/redis/issues\n\n"
"  Suspect RAM error? Use redis-server --test-memory to veryfy it.\n\n"
);
    /* free(messages); Don't call free() with possibly corrupted memory. */
    //if (server.daemonize) unlink(server.pidfile);

    /* Make sure we exit with the right signal at the end. So for instance
     * the core will be dumped if enabled. */
    sigemptyset (&act.sa_mask);
    act.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESETHAND;
    act.sa_handler = SIG_DFL;
    sigaction (sig, &act, NULL);
    kill(getpid(),sig);
}
#endif /* HAVE_BACKTRACE */

}



