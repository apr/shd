
#ifndef CRON_SERVICE_H_
#define CRON_SERVICE_H_

#include <string>


class cron_schedule {
public:

    // minute (0 - 59)
    // hour (0 - 23)
    // day of month (0 - 30)
    // month (0 - 11)
    // day of week (0 - 6)

private:
};


class cron_job {
public:
    virtual ~cron_job() {}

    // Called by the cron service according to the schedule.
    virtual void run() = 0;

    virtual std::string name() const = 0;
};


class cron_service {
public:
    virtual ~cron_service() {}

    virtual void start() = 0;
    virtual void stop() = 0;

    // Does not assume ownership of the job.
    virtual void add_job(const cron_schedule &sched, cron_job *job) = 0;

    // TODO
    virtual void remove_job(cron_job *job) = 0;
};


#endif

