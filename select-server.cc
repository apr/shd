
#include <stdio.h>  // for debugging

#include <errno.h>
#include <string.h>

#include <algorithm>
#include <list>

#include <sys/select.h>
#include <sys/time.h>

#include "callback.h"
#include "select-server.h"


namespace net {

namespace {

// TODO move out to time utils
double time_now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return double(tv.tv_sec) + double(tv.tv_usec) / 1000000;
}


void fill_timeval(double secs, struct timeval *tv)
{
    tv->tv_sec = secs;
    tv->tv_usec = (secs - tv->tv_sec) * 1000000;
}

}


class select_server::alarm_impl : public alarm {
public:
    explicit alarm_impl(callback *callback)
        : callback_(callback)
    {
    }

    ~alarm_impl() {
        delete callback_;
    }

    virtual void stop() {
        delete callback_;
        callback_ = 0;
    }

    void fire() {
        if(callback_) {
            callback_->run();
            callback_ = 0;
        }
    }

private:
    alarm_impl(const alarm_impl &);
    alarm_impl& operator= (const alarm_impl &);

    callback *callback_;
};


select_server::alarm_entry::alarm_entry(double time_to_run, alarm_impl *alarm)
    : time_to_run_(time_to_run), alarm_(alarm)
{
}


double select_server::alarm_entry::time_to_run() const
{
    return time_to_run_;
}


select_server::alarm_impl *select_server::alarm_entry::alarm() const
{
    return alarm_;
}


bool select_server::alarm_entry::operator> (const alarm_entry &rh) const
{
    return time_to_run_ > rh.time_to_run_;
}


select_server::select_server()
{
}


select_server::~select_server()
{
    execute_death_row();

    std::list<callback *>::iterator it = callbacks_.begin();
    for(; it != callbacks_.end(); ++it) {
        // TODO need to be careful if there ever exist permanent callbacks.
        delete *it;
    }

    while(!alarm_queue_.empty()) {
        delete alarm_queue_.top().alarm();
        alarm_queue_.pop();
    }
}


void select_server::register_for_read(connection *conn)
{
    read_registrations_.insert(conn);
}


void select_server::deregister_for_read(connection *conn)
{
    read_registrations_.erase(conn);
}


void select_server::register_for_write(connection *conn)
{
    write_registrations_.insert(conn);
}


void select_server::deregister_for_write(connection *conn)
{
    write_registrations_.erase(conn);
}


void select_server::deregister_connection(connection *conn)
{
    deregister_for_read(conn);
    deregister_for_write(conn);
}


void select_server::run_later(callback *callback)
{
    if(callback) {
        callbacks_.push_back(callback);
    }
}


alarm *select_server::schedule_alarm(callback *callback, int msecs) {
    alarm_impl *ret = new alarm_impl(callback);
    alarm_queue_.push(alarm_entry(time_now() + double(msecs) / 1000, ret));
    return ret;
}


void select_server::loop()
{
    fd_set read_set;
    fd_set write_set;

    while(true) {
        struct timeval timeout;
        int max_fd = init_fd_set(read_registrations_, &read_set);

        max_fd = std::max(
            init_fd_set(write_registrations_, &write_set),
            max_fd);

        // TODO hide this inside a function
        timeout.tv_sec = 9999;
        timeout.tv_usec = 0;

        if(!alarm_queue_.empty()) {
            double tm = alarm_queue_.top().time_to_run() - time_now();

            if(tm < 0) {
                tm = 0;
            }

            fill_timeval(tm, &timeout);
        }

        int ret = select(max_fd + 1, &read_set, &write_set, 0, &timeout);

        if(ret == -1 && errno == EINTR) {
            // TODO need to be more careful here, timers, death row
            continue;
        }

        if(ret == -1) {
            // TODO handle better
            printf("select error: %s\n", strerror(errno));
            continue;
        }

        if(ret > 0) {
            process_events(read_registrations_,
                           &read_set,
                           &connection::on_read);
            process_events(write_registrations_,
                           &write_set,
                           &connection::on_write);
        }

        maybe_fire_alarms();
        run_all_callbacks();
        execute_death_row();
    }
}


int select_server::init_fd_set(const connection_set &conns, fd_set *set)
{
    int max_fd = 0;

    FD_ZERO(set);

    connection_set::const_iterator it = conns.begin();
    for(; it != conns.end(); ++it) {
        int fd = (*it)->get_fd();

        FD_SET(fd, set);
        max_fd = std::max(fd, max_fd);
    }

    return max_fd;
}


void select_server::process_events(
    const connection_set &conns,
    fd_set *set,
    void (connection::*event)())
{
    // Need to copy the connection set because the original set can be modified
    // during the event processing invalidating the iterators.
    connection_set cs(conns);
    connection_set::iterator it = cs.begin();

    for(; it != cs.end(); ++it) {
        int fd = (*it)->get_fd();
        if(FD_ISSET(fd, set)) {
            ((*it)->*event)();
        }
    }
}


void select_server::execute_death_row()
{
    std::list<placeholder *>::iterator it = death_row_.begin();

    for(; it != death_row_.end(); ++it) {
        delete *it;
    }

    death_row_.clear();
}


void select_server::run_all_callbacks()
{
    // TODO this loop may cause starvation of even processing if running
    // callbacks constantly add new callabacks. Or is it a feature?
    std::list<callback *>::iterator it = callbacks_.begin();

    for(; it != callbacks_.end(); ++it) {
        (*it)->run();
    }

    callbacks_.clear();
}


void select_server::maybe_fire_alarms()
{
    double now = time_now();

    while(!alarm_queue_.empty() && alarm_queue_.top().time_to_run() < now) {
        alarm_impl *alarm = alarm_queue_.top().alarm();
        alarm->fire();
        delete alarm;
        alarm_queue_.pop();
    }
}


}

