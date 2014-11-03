
#ifndef MOCK_EXECUTOR_H_
#define MOCK_EXECUTOR_H_

#include <functional>
#include <vector>
#include <sys/types.h>

#include "executor.h"


class mock_executor : public net::executor {
public:
    mock_executor() {}

    void run_later(const std::function<void()> &callback) override {
        callbacks_.push_back(callback);
    }

    void run_all_now() {
        std::vector<std::function<void()>> cs;

        cs.swap(callbacks_);
        for(size_t i = 0; i < cs.size(); ++i) {
            cs[i]();
        }
    }

    void run_until_empty() {
        while(!empty()) {
            run_all_now();
        }
    }

    bool empty() const { return callbacks_.empty(); }

private:
    std::vector<std::function<void()>> callbacks_;
};

#endif

