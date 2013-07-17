/************************************************************************/
/*                                                                      */
/************************************************************************/
#include "redis.h"

#include <string.h>

namespace redis{ namespace{

	/* Returns 1 if there is --sentinel among the arguments or if
	* argv[0] is exactly "redis-sentinel". */
	int checkForSentinelMode(int argc, char **argv) {
		if (strstr(argv[0],"redis-sentinel") != NULL) {
			return 1;
		}
		for (int j = 1; j < argc; j++){
			if (!strcmp(argv[j],"--sentinel")) {
				return 1;
			}
		}
		return 0;
	}

	void initServerConfig(){

	}
}

// golbal server object
redisServer server;

int Main(int argc, char **argv){
	server.sentinel_mode = checkForSentinelMode(argc,argv);
	return 0;
}

}


int main(int argc, char **argv){
	return redis::Main(argc,argv);
}
