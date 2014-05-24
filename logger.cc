
#include "logger.h"

#include <stdarg.h>
#include <syslog.h>


static bool logging_disabled = false;


void log_error(const char *format, ...)
{
    if(logging_disabled) {
        return;
    }

    va_list ap;

    va_start(ap, format);
    vsyslog(LOG_ERR, format, ap);
    va_end(ap);
}


void log_info(const char *format, ...)
{
    if(logging_disabled) {
        return;
    }


    va_list ap;

    va_start(ap, format);
    vsyslog(LOG_INFO, format, ap);
    va_end(ap);
}


void disable_logging()
{
    logging_disabled = true;
}

