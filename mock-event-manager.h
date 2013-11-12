
#ifndef MOCK_EVENT_MANAGER_H_
#define MOCK_EVENT_MANAGER_H_


#include <set>

#include "event-manager.h"


class mock_event_manager : public net::event_manager {
public:
    virtual void register_for_read(net::connection *conn) {
        reads_.insert(conn);
    }

    virtual void deregister_for_read(net::connection *conn) {
        reads_.erase(conn);
    }

    virtual void register_for_write(net::connection *conn) {
        writes_.insert(conn);
    }

    virtual void deregister_for_write(net::connection *conn) {
        writes_.erase(conn);
    }

    virtual void deregister_connection(net::connection *conn) {
        reads_.erase(conn);
        writes_.erase(conn);
    }

    void send_signal() {
        std::set<net::connection *>::iterator it;

        for (it = reads_.begin(); it != reads_.end(); ++it) {
            (*it)->on_read();
        }

        for (it = writes_.begin(); it != writes_.end(); ++it) {
            (*it)->on_write();
        }
    }

    bool empty() const {
        return reads_.empty() && writes_.empty();
    }

private:
    std::set<net::connection *> reads_;
    std::set<net::connection *> writes_;
};

#endif

