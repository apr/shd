
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
        for(size_t i = 0; i < callbacks_.size(); ++i) {
            callbacks_[i]->run();
        }

        callbacks_.clear();
    }

    bool empty() const { return callbacks_.empty(); }

private:
    std::vector<callback *> callbacks_;
};

#endif

