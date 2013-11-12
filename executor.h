
#ifndef EXECUTOR_H_
#define EXECUTOR_H_

class callback;


namespace net {


// TODO document use and nuances
class executor {
public:
    virtual ~executor() {}

    // TODO rename?
    // TODO document ownership
    virtual void run_later(callback *callback) = 0;
};

}

#endif

