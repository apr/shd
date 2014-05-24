
#ifndef LOGGER_H_
#define LOGGER_H_

// Sends a message to the system logger.
void log_error(const char *format, ...);
void log_info(const char *format, ...);


// Turns the logger off. Primarily to be used for tests.
void disable_logging();


#endif

