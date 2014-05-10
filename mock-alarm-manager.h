
#ifndef MOCK_ALARM_MANAGER_H_
#define MOCK_ALARM_MANAGER_H_

#include <list>

#include "alarm-manager.h"
#include "callback.h"


class mock_alarm_manager : public net::alarm_manager {
private:
    class alarm_impl : public net::alarm {
    public:
        explicit alarm_impl(callback *c) : callback_(c) {}

        ~alarm_impl() {
            delete callback_;
        }

        virtual void stop() {
            delete callback_;
            callback_ = 0;
        }

        void fire() {
            if(callback_) callback_->run();
            callback_ = 0;
        }

    private:
        alarm_impl(const alarm_impl &);
        alarm_impl &operator= (const alarm_impl &);

        callback *callback_;
    };

public:
    mock_alarm_manager() {
    }

    ~mock_alarm_manager() {
        std::list<alarm_impl *>::iterator it = alarms_.begin();
        for(; it != alarms_.end(); ++it) {
            delete *it;
        }
    }

    virtual net::alarm *schedule_alarm(callback* callback, int msecs) {
        alarm_impl *ret = new alarm_impl(callback);
        alarms_.push_back(ret);
        return ret;
    }

    void fire_all_alarms() {
        std::list<alarm_impl *> alarms;
        alarms.swap(alarms_);

        std::list<alarm_impl *>::iterator it = alarms.begin();
        for(; it != alarms.end(); ++it) {
            (*it)->fire();
            delete *it;
        }
    }

private:
    mock_alarm_manager(const mock_alarm_manager &);
    mock_alarm_manager &operator= (const mock_alarm_manager &);

    std::list<alarm_impl *> alarms_;
};


#endif

