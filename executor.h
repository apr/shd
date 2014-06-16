
#ifndef EXECUTOR_H_
#define EXECUTOR_H_

class callback;


namespace net {


// A service that provides delayed execution of callbacks.
class executor {
public:
    virtual ~executor() {}

    // Schedules execution of the given callback sometime in the future. Takes
    // ownership of the callback.
    virtual void run_later(callback *callback) = 0;
};

}

#endif

