
#ifndef EVENT_MANAGER_H_
#define EVENT_MANAGER_H_

namespace net {


// An abstract connection interface.
class connection {
public:
    virtual ~connection() {}

    virtual int get_fd() = 0;

    virtual void on_read() {}
    virtual void on_write() {}
};


// IO event manager. The caller retains ownership of all connections registered
// with an event manager.
class event_manager {
public:
    virtual ~event_manager() {}

    virtual void register_for_read(connection *conn) = 0;
    virtual void deregister_for_read(connection *conn) = 0;

    virtual void register_for_write(connection *conn) = 0;
    virtual void deregister_for_write(connection *conn) = 0;

    // Deregister from receiving all events in one call.
    virtual void deregister_connection(connection *conn) = 0;
};

}

#endif

