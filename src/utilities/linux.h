#ifndef UTILITIES_LINUX_H
#define UTILITIES_LINUX_H

#define SHUTDOWN_SEQUENCE "CTRL-C"

/* Call this on startup. */
void registerSignalHandler();

/* Use this to check if the shutdown flag is set. */
unsigned int getShutDownFlag();

// Gets the application path.
// The returned char array must be deleted by the caller.
char* getAppPath();

// Gets the time in milliseconds since 01.01.1970
long long getTime();

// Gets the current time in UTC as string like "12:43:41.259Z" (ISO 8601).
// The returned char array must be deleted by the caller.
char* getTimeString();

#endif // UTILITIES_LINUX_H
