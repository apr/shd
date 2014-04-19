
#ifndef MOCK_EXECUTOR_H_
#define MOCK_EXECUTOR_H_

#include <vector>

#include "callback.h"
#include "executor.h"


class mock_executor : public net::executor {
public:
    virtual void run_later(callback *callback) {
        callbacks_.push_back(callback);
    }

    void run_all_now() {
        std::vector<callback *> cs;

        cs.swap(callbacks_);
        for(size_t i = 0; i < cs.size(); ++i) {
            cs[i]->run();
        }
    }

    void run_until_empty() {
        while(!empty()) {
            run_all_now();
        }
    }

    bool empty() const { return callbacks_.empty(); }

private:
    std::vector<callback *> callbacks_;
};

#endif

