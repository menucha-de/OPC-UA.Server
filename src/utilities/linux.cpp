#include "linux.h"
#include <limits.h> // PATH_MAX
#include <signal.h> // sigaction
#include <stdio.h> // snprintf
#include <string.h> // memset
#include <unistd.h> // readlink
#include <sys/time.h> // gettimeofday
#include <time.h> // gmtime_r, strftime

/* shutdown flag */
static volatile unsigned int g_ShutDown = 0;

/* Return shutdown flag. */
unsigned int getShutDownFlag() {
    return g_ShutDown;
}

/** Signal handler for SIG_INT and SIGTERM. */
void signalHandler(int signo) {
    g_ShutDown = 1;
}

void registerSignalHandler() {
    /* register signal handlers. */
    struct sigaction new_action, old_action;

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = signalHandler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    /* install new signal handler for SIGINT */
    sigaction(SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) {
        sigaction(SIGINT, &new_action, NULL);
    }
    /* install new signal handler for SIGTERM */
    sigaction(SIGTERM, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) {
        sigaction(SIGTERM, &new_action, NULL);
    }

    /* Set up the structure to prevent program termination on interrupted connections. */
    new_action.sa_handler = SIG_IGN;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    /* install new signal handler for SIGPIPE */
    sigaction(SIGPIPE, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) {
        sigaction(SIGPIPE, &new_action, NULL);
    }
}

char* getAppPath() {
    // get app path
    char appPath[PATH_MAX];
    memset(appPath, 0, sizeof (appPath)); /* initialize with zeros, readlink does not null terminate the string */
    readlink("/proc/self/exe", appPath, PATH_MAX - 1);
    // cut off app name
    char* pszFind = strrchr(appPath, '/');
    if (pszFind != NULL) {
        *pszFind = 0;
    }
    // reduce size of char array and return it
    size_t appPathLen = strlen(appPath);
    char* ret = new char [appPathLen + 1];
    ret[appPathLen] = 0;
    strncpy(ret, appPath, appPathLen);
    return ret;
}

long long getTime() {
    timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

char* getTimeString() {
    timeval t;
    gettimeofday(&t, 0);
    struct tm tmTime;
    gmtime_r(&t.tv_sec, &tmTime);
    char* ret = new char[14];
    strftime(ret, 14, "%T.000Z", &tmTime);
    snprintf(&ret[9], 5, "%03iZ", t.tv_usec / 1000);
    return ret;
}
