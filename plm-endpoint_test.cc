
#include <memory>

#include "logger.h"
#include "mock-alarm-manager.h"
#include "mock-event-manager.h"
#include "mock-executor.h"
#include "mock-plm-fd.h"
#include "plm-endpoint.h"

#include <gtest/gtest.h>


namespace plm {


class PlmEndpointTest : public testing::Test {
public:
    PlmEndpointTest()
        : response_(plm_endpoint::response_t::OK)
    {
    }


    virtual void SetUp() {
        disable_logging();

        executor_.reset(new mock_executor);
        event_manager_.reset(new mock_event_manager);
        fd_.reset(new mock_plm_fd);
        alarm_manager_.reset(new mock_alarm_manager);
        endpoint_.reset(new plm_endpoint(
            fd_.get(),
            alarm_manager_.get(),
            event_manager_.get(),
            executor_.get()));
        done_ = false;
    }


    void loop_once() {
        done_ = false;
        event_manager_->send_signal();
        executor_->run_until_empty();
    }


    void done_callback(plm_endpoint::response_t r) {
        response_ = r;
        done_ = true;
    }


    callback1<plm_endpoint::response_t> *make_done_callback() {
        return make_callback(
            (PlmEndpointTest *)this,
            &PlmEndpointTest::done_callback);
    }


    std::unique_ptr<mock_executor> executor_;
    std::unique_ptr<mock_event_manager> event_manager_;
    std::unique_ptr<mock_plm_fd> fd_;
    std::unique_ptr<mock_alarm_manager> alarm_manager_;
    std::unique_ptr<plm_endpoint> endpoint_;

    bool done_;
    plm_endpoint::response_t response_;
};


TEST_F(PlmEndpointTest, StartStop) {
    endpoint_->start();
    EXPECT_FALSE(fd_->is_closed());
    endpoint_->stop();
    EXPECT_TRUE(fd_->is_closed());
}


TEST_F(PlmEndpointTest, SendLightOn) {
    endpoint_->start();
    endpoint_->send_light_on("\x01\x02\x03", make_done_callback());

    loop_once();

    EXPECT_EQ("\x02\x62\x01\x02\x03\x0f\x12\xff", fd_->get_write_buf());
    EXPECT_FALSE(done_);

    // First return ack from the modem.
    fd_->set_read_buf("\x02\x62\x01\x02\x03\x0f\x12\xff\x06");
    loop_once();
    EXPECT_FALSE(done_);

    // And now ack from the device (device addr 04 05 06).
    fd_->set_read_buf("\x02\x50\x04\x05\x06\x01\x02\x03\x2f\x12\xff");
    loop_once();
    EXPECT_TRUE(done_);
    EXPECT_EQ(plm_endpoint::response_t::OK, response_.status);
}


TEST_F(PlmEndpointTest, SendLightOff) {
    endpoint_->start();
    endpoint_->send_light_off("\x01\x02\x03", make_done_callback());

    loop_once();

    EXPECT_EQ(std::string("\x02\x62\x01\x02\x03\x0f\x13\x00", 8),
              fd_->get_write_buf());
    EXPECT_FALSE(done_);

    // First return ack from the modem.
    fd_->set_read_buf(std::string("\x02\x62\x01\x02\x03\x0f\x13\x00\x06", 9));
    loop_once();
    EXPECT_FALSE(done_);

    // And now ack from the device (device addr 04 05 06).
    fd_->set_read_buf(
        std::string("\x02\x50\x04\x05\x06\x01\x02\x03\x2f\x13\x00", 11));
    loop_once();
    EXPECT_TRUE(done_);
    EXPECT_EQ(plm_endpoint::response_t::OK, response_.status);
}


// TODO test
// - NACKs
// - errors
// - timeouts


}

