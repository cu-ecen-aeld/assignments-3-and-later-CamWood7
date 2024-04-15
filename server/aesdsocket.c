#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

//Declare global variables

#define BUFFER_SIZE 4096
#define PORT "9000"
static int sigRec = 0;

static void signal_handler(int z) {
    sigRec = 1;
}

static bool reader(int socket, int fid) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes = 0;
    off_t offset = lseek(fid, 0, SEEK_END);
    char *newChar = NULL;
    bool complete = false;

    while (!sigRec && !complete) {
        bytes = recv(socket, buffer, sizeof(buffer), 0);

	if (bytes == -1) {
	    syslog(LOG_ERR, "Recv Error: %s", strerror(errno));
	    return false;
	
	}
	else if (bytes == 0) {
	    return false;
	
	}

        newChar = memchr(buffer, '\n', bytes);
	if (newChar) {
	    bytes = newChar + 1 - buffer;
	    complete = true;
	}

	if (write(fid, buffer, bytes) == -1) {
	    syslog(LOG_ERR, "Could not write: %s", strerror(errno));
	    ftruncate(fid, offset);
	    return false;
	
	}
    
    }

    if (sigRec) {
        return false;
    }

    return true;

}

static void respond(int socket, int fid) {
    ssize_t bytes = 0;
    char buffer[BUFFER_SIZE];
    
    if ((lseek(fid, 0, SEEK_SET)) == -1) {
        syslog(LOG_ERR, "lseek error: %s", strerror(errno));
	return;
    
    }

    while (!sigRec) {
        bytes = read(fid, buffer, sizeof(buffer));

	if (bytes == 0) {
	    return;
	
	} 
	else if (bytes == -1) {
	    syslog(LOG_ERR, "Cannot read: %s", strerror(errno));
	    return;
	
	}

	if (send(socket, buffer, bytes, 0) == -1) {
	    syslog(LOG_ERR, "Cannot send: %s", strerror(errno));
	    return;
	
	}
    
    }

}

int main(int argc, char **argv) {
    
    // Initialization
    openlog(NULL, 0, LOG_USER);

    struct addrinfo sockType;
    memset(&sockType, 0, sizeof(sockType));

    // Check for daemon
    int daemonOn = 0;	
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-d")) {
	    daemonOn = 1;
	
	}
    
    }

    // Check socket, all errors checked at the end
    int ogSock = -1;
    int success = 1;

    if ((ogSock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_ERR, "Cannot open file: %s", strerror(errno));
	goto errorExit;
    
    }

    int z = 1;

    if (setsockopt(ogSock, SOL_SOCKET, SO_REUSEADDR, &z, sizeof(z)) == -1) {
        syslog(LOG_ERR, "setsockopt error: %s", strerror(errno));
	goto errorExit;
    
    }
    
    int returnVal = 0;
    struct addrinfo *addInfo = NULL;
    sockType.ai_flags = AI_PASSIVE;
    sockType.ai_socktype = SOCK_STREAM;
    sockType.ai_family = PF_INET;

    if ((returnVal = getaddrinfo(NULL, PORT, &sockType, &addInfo)) != 0) {
        syslog(LOG_ERR, "getAddrInfo error: %s", gai_strerror(returnVal));
	goto errorExit;
    
    }

    if (!addInfo) {
        syslog(LOG_ERR, "No returnVal for %s", PORT);
	goto errorExit;
    
    }

    if (bind(ogSock, addInfo->ai_addr, addInfo->ai_addrlen) == -1) {
        syslog(LOG_ERR, "Bind error: %s", strerror(errno));
	goto errorExit;
    
    }

    if (daemonOn) {
        pid_t pid = fork();
	if (pid != 0) {
	    _exit(0);
	
	}
	setsid();
	
	chdir("/");
    	if ((freopen("/dev/null", "r", stdin) == NULL) || (freopen("/dev/null", "w", stdout) == NULL) || (freopen("/dev/null", "r", stderr) == NULL)) {
            syslog(LOG_ERR, "Error in redirecting i/o!: %s", strerror(errno));
      	    goto errorExit;
    	}
    }

    int file = open("/var/tmp/aesdsocketdata", O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (file == -1) {
        syslog(LOG_ERR, "Cannot open file: %s", strerror(errno));
	goto errorExit;
    }

    if (listen(ogSock, 5) == -1) {
        syslog(LOG_ERR, "Cannot listen: %s", strerror(errno));
	goto errorExit;
    
    }

    //Set up Signal Handler
    struct sigaction SA;
    memset(&SA, 0, sizeof(SA));
    SA.sa_handler = signal_handler;
    sigaction(SIGINT, &SA, NULL);
    sigaction(SIGTERM, &SA, NULL);
    
    struct sockaddr sockAdd;
    int childAccepted = -1;

    // IP length can be max 40 chars for IPv6
    char reqIP[40];

    while (!sigRec) {
        socklen_t len = sizeof(sockAdd);
        if ((childAccepted = accept(ogSock, &sockAdd, &len)) == -1) {
	    syslog(LOG_ERR, "Not accepted: %s", strerror(errno));
	    continue;
	
	}

	if (inet_ntop(AF_INET, &((struct sockaddr_in *)&sockAdd)->sin_addr, reqIP, sizeof(reqIP)) == NULL) {
	    strncpy(reqIP, "???", sizeof(reqIP));
	
	}
	syslog(LOG_DEBUG, "Accepted connection from %s", reqIP);

	if (!reader(childAccepted, file)) {
	    syslog(LOG_ERR, "Cannot read packet");
	
	}
	else 
	{
            respond(childAccepted, file);
	
	}
        
	// Start closing down
	close(childAccepted);
	childAccepted = -1;
	syslog(LOG_DEBUG, "Closed connected from %s", reqIP);
    
    }

    if (sigRec) {
        syslog(LOG_ERR, "Signal Recieved");
    }

    if (childAccepted != -1) {
        close(childAccepted);
	childAccepted = -1;
	syslog(LOG_DEBUG, "Closed connected from %s", reqIP);
    
    }

    success = 0;

errorExit:
    if (file != -1) {
        close(file);
    }
    if (ogSock != -1) {
        close(ogSock);
    } 
    if (childAccepted != -1) {
        close(childAccepted);
    }
    if (addInfo) {
        freeaddrinfo(addInfo);
    }

    closelog();
    return success;

}
