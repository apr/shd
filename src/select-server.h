
#ifndef SELECT_SERVER_H_
#define SELECT_SERVER_H_

#include <functional>
#include <list>
#include <queue>
#include <set>
#include <vector>

#include <sys/select.h>


class callback;


namespace net {

// TODO
class connection {
public:
    virtual ~connection() {}

    virtual int get_fd() = 0;

    virtual void on_read() {};
    virtual void on_write() {};
    virtual void on_error() {};
};


// TODO
class alarm {
public:
    virtual ~alarm() {}

    virtual void stop() = 0;
};


class select_server {
public:
    select_server();
    ~select_server();

    void register_for_read(connection *conn);
    void deregister_for_read(connection *conn);

    void register_for_write(connection *conn);
    void deregister_for_write(connection *conn);

    void register_for_error(connection *conn);
    void deregister_for_error(connection *conn);

    // Deregister from receiving all events in one call.
    void deregister_connection(connection *conn);

    void run_later(callback *callback);

    // Schedules execution of the given callback after the given number of
    // milliseconds has elapesed. The returned alarm object is owned by the
    // select server and is valid until either the callback finished execution
    // of stop() is called on the alarm object. Careful, the pointer can become
    // invalid at any point and should not be dereferenced after either the
    // callback is done or the alarm is stopped.
    alarm *schedule_alarm(callback *callback, int msecs);

    void loop();

    // TODO how to exit?

    // Registers the given object for deletion. The deletion will happen at
    // some time during loop execution.
    template<class T>
    void register_for_delete(T *t);

private:
    typedef std::set<connection *> connection_set;

    select_server(const select_server &);
    select_server& operator= (const select_server &);

    // Initializes the given set with fds for the connections, returns the max
    // fd encountered.
    static int init_fd_set(const connection_set &conns, fd_set *set);

    // For all connection in the 'conns' set that are indicated as being
    // triggered in the fd_set call the given event function.
    static void process_events(
        const connection_set &conns,
        fd_set *set,
        void (connection::*event)());

    // Delete all objects that were scheduled for deletion.
    void execute_death_row();

    void run_all_callbacks();
    void maybe_fire_alarms();

private:
    // Connections on these lists are not owned by the select server.
    connection_set read_registrations_;
    connection_set write_registrations_;
    connection_set error_registrations_;

    // Callbacks for delayed execution.
    std::list<callback *> callbacks_;

    struct placeholder {
        placeholder() {}
        virtual ~placeholder() {}
    };

    template<class T>
    struct holder : public placeholder {
        explicit holder(T *obj) : obj_(obj) {}
        ~holder() { delete obj_; }
        T *obj_;
    };

    std::list<placeholder *> death_row_;

    class alarm_impl;

    class alarm_entry {
    public:
        alarm_entry(double time_to_run, alarm_impl *alarm);

        double time_to_run() const;
        alarm_impl *alarm() const;

        bool operator> (const alarm_entry &rh) const;

    private:
        double time_to_run_;
        alarm_impl *alarm_;
    };

    typedef std::priority_queue<
        alarm_entry,
        std::vector<alarm_entry>,
        std::greater<alarm_entry> > alarm_queue_t;

    alarm_queue_t alarm_queue_;
};


template<class T>
void select_server::register_for_delete(T *t)
{
    death_row_.push_back(new holder<T>(t));
}

}

#endif

