
#include "shd-app.h"

// TODO For debugging, remove later.
#include <stdio.h>

#include <time.h>

#include <string>

#include "alarm-manager.h"
#include "callback.h"
#include "executor.h"
#include "plm-connection.h"
#include "plm-util.h"
#include "sunrise-sunset.h"



// TODO
class shd_light : public plm::plm_command_listener{
public:
    shd_light(const std::string &addr,
              plm::plm_connection *conn,
              net::alarm_manager *alarm_manager,
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

    // Cancels the current command and resets state in preparation for the next.
    void reset_command();
    void send_command();

    void on_plm_response(plm::plm_connection::plm_response r);
    void on_timeout();

    // PLM listener callback;
    virtual void on_command(const std::string &data);

private:
    enum state_t {
        INIT, SENT, ON_WIRE, DONE, ERROR
    };

    enum on_off_t {
        ON, OFF
    };

    std::string addr_;

    // not owned
    plm::plm_connection *conn_;
    net::alarm_manager *alarm_manager_;
    net::executor *executor_;

    state_t state_;
    on_off_t on_off_;
    std::string cmd_;
    callback *done_;
    net::alarm *timeout_;

    int num_retries_;
};


shd_light::shd_light(const std::string &addr,
                     plm::plm_connection *conn,
                     net::alarm_manager *alarm_manager,
                     net::executor *executor)
    : addr_(addr), conn_(conn),
      alarm_manager_(alarm_manager), executor_(executor),
      state_(INIT), on_off_(OFF), done_(0), timeout_(0)
{
    conn_->add_listener(this);
}


shd_light::~shd_light()
{
    reset_command();
    conn_->remove_listener(this);
}


void shd_light::light_on(callback *done)
{
    reset_command();

    on_off_ = ON;
    cmd_ = char(0x62) + addr_ + plm::hex_to_bin("0F12FF");
    done_ = done;

    send_command();
}


void shd_light::light_off(callback *done)
{
    reset_command();

    on_off_ = OFF;
    cmd_ = char(0x62) + addr_ + plm::hex_to_bin("0F1300");
    done_ = done;

    send_command();
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


void shd_light::reset_command()
{
    if(timeout_) {
        timeout_->stop();
        timeout_ = 0;
    }

    state_ = INIT;
    cmd_.clear();
    delete done_;
    done_ = 0;

    num_retries_ = 5;
}


void shd_light::send_command()
{
    if(num_retries_-- <= 0) {
        state_ = ERROR;
        executor_->run_later(done_);
        done_ = 0;
        return;
    }

    state_ = SENT;
// TODO this is risky, it may happen that the timeout arrives earlier than the
// response from the connection and then the timeout handler will try to
// reexecute on top of the first command causing an exception to be thrown.
#if 0
    timeout_ = alarm_manager_->schedule_alarm(
        make_callback(this, &shd_light::on_timeout),
        1000);
#endif
    conn_->send_command(cmd_,
        make_callback(this, &shd_light::on_plm_response));
}


void shd_light::on_plm_response(plm::plm_connection::plm_response r)
{
    if(timeout_) {
        timeout_->stop();
        timeout_ = 0;
    }

    if(r.status != plm::plm_connection::plm_response::ACK) {
        send_command();
        return;
    }

    state_ = ON_WIRE;
    timeout_ = alarm_manager_->schedule_alarm(
        make_callback(this, &shd_light::on_timeout),
        3000);
}


void shd_light::on_timeout()
{
    timeout_ = 0;
    send_command();
}


void shd_light::on_command(const std::string &data)
{
    if(data[0] != 0x50) {
        // Wrong command, don't care.
        return;
    }

    std::string from_addr = data.substr(1, 3);
    if(from_addr != addr_) {
        // Not our light.
        return;
    }

    // cmd_[5] has the insteon command (0x12 or 0x13; on or off).
    if(data[8] != cmd_[5]) {
        // Response to a wrong command.
        return;
    }

    if(timeout_) {
        timeout_->stop();
        timeout_ = 0;
    }

    char flags = data[7];
    if((flags & 0xf0) != 0x20) {
        // NACK
        send_command();
        return;
    }

    state_ = DONE;
    executor_->run_later(done_);
    done_ = 0;
}



// TODO
// - make a class shd_light that would manage the lifecycle of
//   on/off signal sending
// - the app will upon a timed signal go through the list of lights and
//   command them
// - once done a light will call a callback back to the app
// - the app on each light callback call will go through the list
//   checking that all lights are done (either with success or
//   with error)
// - once all are done the app will set up next alarm to check
//   time and command the lights


shd_app::shd_app(const shd_config *config,
                 net::event_manager *event_manager,
                 net::alarm_manager *alarm_manager,
                 net::executor *executor)
    : config_(config), event_manager_(event_manager),
      alarm_manager_(alarm_manager), executor_(executor),
      fd_(config->serial_device()),
      conn_(&fd_, event_manager, executor),
      next_run_alarm_(0)
{
    for(size_t i = 0; i < config->outside_lights().size(); ++i) {
        std::string addr = config->outside_lights()[i];
        lights_.push_back(new shd_light(addr,
                                        &conn_,
                                        alarm_manager_,
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

    // Provide a half-hour margine around actual sunrise/sunset times.
    hour_off += 0.5;
    hour_on -= 0.5;

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

    if(!conn_.is_ok()) {
        conn_.stop();
    }

    if(conn_.is_closed()) {
        conn_.start();
    }

    if(!conn_.is_ok() || conn_.is_closed()) {
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

