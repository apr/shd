
#ifndef ALARM_MANAGER_H_
#define ALARM_MANAGER_H_

#include <functional>


namespace net {


// A scheduled alarm handler. The main purpose of the handler is to provide the
// holder the means to cancel an alarm. The alarm can only be cancelled before
// it has run and stop() should only be called once. The validity of the
// handler is not guaranteed in any other case and the pointer to it should not
// be dereferenced.
class alarm {
public:
    virtual ~alarm() {}

    virtual void stop() = 0;
};


class alarm_manager {
public:
    virtual ~alarm_manager() {}

    // Schedules execution of the given callback after the given number of
    // milliseconds has elapesed. The returned alarm object is owned by the
    // alarm manager and is valid until either the callback finished execution
    // or stop() is called on the alarm object. Be careful, the pointer can
    // become invalid at any point and should not be dereferenced after either
    // the callback is done or the alarm is stopped.
    virtual alarm *schedule_alarm(const std::function<void()> &callback,
                                  int msecs) = 0;
};


}

#endif

