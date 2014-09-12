
#ifndef SHD_APP_H_
#define SHD_APP_H_

#include <list>
#include <string>

#include "plm-connection.h"
#include "shd-config.h"


namespace net {
class alarm;
class alarm_manager;
class event_manager;
class executor;
}


class shd_light;

class shd_app {
public:
    shd_app(const shd_config *config,
            net::event_manager *event_manager,
            net::alarm_manager *alarm_manager,
            net::executor *executor);
    ~shd_app();

    // TODO may or may not throw
    void run();

private:
    shd_app(const shd_app &);
    shd_app &operator= (const shd_app &);

    void process_ligths();
    void next_run();

    // Called when a light has finished processing a command.
    void light_done();

    // Returns current fractional day hour.
    static double hour_now();

private:
    // Not owned.
    const shd_config *config_;
    net::event_manager *event_manager_;
    net::alarm_manager *alarm_manager_;
    net::executor *executor_;

    plm::plm_fd fd_;
    plm::plm_connection conn_;

    net::alarm *next_run_alarm_;

    std::list<shd_light *> lights_;
};


#endif

