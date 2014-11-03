
#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include <functional>


namespace net {


// A service that provides delayed execution of callbacks.
class executor {
public:
    virtual ~executor() {}

    // Schedules execution of the given callback sometime in the future.
    virtual void run_later(const std::function<void()> &callback) = 0;
};

}

#endif

