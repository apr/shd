
#include "shd-app.h"

// TODO For debugging, remove later.
#include <stdio.h>

#include <time.h>

#include <string>

#include "alarm-manager.h"
#include "callback.h"
#include "executor.h"
#include "plm-endpoint.h"
#include "plm-util.h"
#include "sunrise-sunset.h"



// TODO
class shd_light {
public:
    shd_light(const std::string &addr,
              plm::plm_endpoint *plm,
              net::executor *executor);
    ~shd_light();

    void light_on(callback *done);
    void light_off(callback *done);

    // is_done will return true if the state machine has finished irrespective
    // of success, it may be true when none of is_on, is_off is true.
    bool is_done() const;
    bool is_on() const;
    bool is_off() const;

private:
    shd_light(const shd_light &);
    shd_light& operator= (const shd_light &);

    void on_plm_response(plm::plm_endpoint::response_t r);
    void on_timeout();

private:
    enum state_t {
        INIT, SENT, DONE, ERROR
    };

    enum on_off_t {
        ON, OFF
    };

    std::string addr_;

    // not owned
    plm::plm_endpoint *plm_;
    net::executor *executor_;

    state_t state_;
    on_off_t on_off_;
    callback *done_;
};


shd_light::shd_light(const std::string &addr,
                     plm::plm_endpoint *plm,
                     net::executor *executor)
    : addr_(addr), plm_(plm), executor_(executor),
      state_(INIT), on_off_(OFF), done_(0)
{
}


shd_light::~shd_light()
{
}


void shd_light::light_on(callback *done)
{
    on_off_ = ON;
    done_ = done;
    state_ = SENT;
    plm_->send_light_on(addr_,
                        make_callback(this, &shd_light::on_plm_response));
}


void shd_light::light_off(callback *done)
{
    on_off_ = OFF;
    done_ = done;
    state_ = SENT;
    plm_->send_light_off(addr_,
                         make_callback(this, &shd_light::on_plm_response));
}


bool shd_light::is_done() const
{
    return state_ == DONE || state_ == ERROR || state_ == INIT;
}


bool shd_light::is_on() const
{
    return state_ == DONE && on_off_ == ON;
}


bool shd_light::is_off() const
{
    return state_ == DONE && on_off_ == OFF;
}


void shd_light::on_plm_response(plm::plm_endpoint::response_t r)
{
    if(r.status == plm::plm_endpoint::response_t::OK) {
        state_ = DONE;
    } else {
        state_ = ERROR;
    }

    executor_->run_later(done_);
    done_ = 0;
}


shd_app::shd_app(const shd_config *config,
                 net::event_manager *event_manager,
                 net::alarm_manager *alarm_manager,
                 net::executor *executor)
    : config_(config), event_manager_(event_manager),
      alarm_manager_(alarm_manager), executor_(executor),
      fd_(config->serial_device()),
      plm_(&fd_, alarm_manager, event_manager, executor),
      next_run_alarm_(0)
{
    for(size_t i = 0; i < config->outside_lights().size(); ++i) {
        std::string addr = config->outside_lights()[i];
        lights_.push_back(new shd_light(addr,
                                        &plm_,
                                        executor_));
    }
}


shd_app::~shd_app()
{
    if(next_run_alarm_) {
        next_run_alarm_->stop();
    }

    std::list<shd_light *>::iterator it = lights_.begin();
    for(; it != lights_.end(); ++it) {
        delete *it;
    }
}


void shd_app::run()
{
    next_run();
}


void shd_app::process_ligths()
{
    time_t time_now = time(0);
    struct tm *tm = localtime(&time_now);
    double hour_off = sunrise_hour(tm->tm_year + 1900,
                                   tm->tm_mon + 1,
                                   tm->tm_mday,
                                   config_->latitude(),
                                   config_->longitude());
    double hour_on = sunset_hour(tm->tm_year + 1900,
                                 tm->tm_mon + 1,
                                 tm->tm_mday,
                                 config_->latitude(),
                                 config_->longitude());
    double now = hour_now();

    std::list<shd_light *>::iterator it = lights_.begin();
    for(; it != lights_.end(); ++it) {
        shd_light *light = *it;

        if(now > hour_off && now < hour_on) {
            if(!light->is_off()) {
                // TODO remove
                printf("%s - now: %f, h_off: %f, h_on: %f, turning off\n",
                    ctime(&time_now), now, hour_off, hour_on);
                light->light_off(make_callback(this, &shd_app::light_done));
            }
        } else {
            if(!light->is_on()) {
                // TODO remove
                printf("%s - now: %f, h_off: %f, h_on: %f, turning on\n",
                    ctime(&time_now), now, hour_off, hour_on);
                light->light_on(make_callback(this, &shd_app::light_done));
            }
        }
    }

    // Check if all lights are done immediately and schedule next run.
    light_done();
}


void shd_app::next_run()
{
    next_run_alarm_ = 0;

    if(!plm_.is_ok()) {
        plm_.stop();
    }

    if(plm_.is_closed()) {
        plm_.start();
    }

    if(!plm_.is_ok() || plm_.is_closed()) {
        // TODO need to log if the connection cannot be opened, but only once.
        next_run_alarm_ = alarm_manager_->schedule_alarm(
            make_callback(this, &shd_app::next_run),
            60000);  // 1 min
        return;
    }

    process_ligths();
}


void shd_app::light_done()
{
    bool all_done = true;

    std::list<shd_light *>::iterator it = lights_.begin();
    for(; it != lights_.end(); ++it) {
        shd_light *light = *it;
        all_done = light->is_done() && all_done;
    }

    if(all_done) {
        next_run_alarm_ = alarm_manager_->schedule_alarm(
            make_callback(this, &shd_app::next_run),
            60000);  // 1 min
    }
}


double shd_app::hour_now()
{
    time_t time_now = time(0);
    struct tm *tm = localtime(&time_now);
    return double(tm->tm_hour) +
           double(tm->tm_min) / 60 +
           double(tm->tm_sec) / 3600;
}

