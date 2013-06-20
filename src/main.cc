
#include <stdio.h>

#include <sys/time.h>

#include <functional>
#include <vector>

#include "plm-connection.h"
#include "select-server.h"
#include "test-connection.h"

#include "callback.h"


struct tt {
    void on_response(plm::plm_connection::plm_response r)
    {
        //printf("RESP: %d - %s\n", r.status, r.data.c_str());
    }

    void test()
    {
        time_t t = time(0);
        printf("now: %s\n", ctime(&t));
    }
};


int main()
{
    tt t;

    try {
        net::select_server ss;

t.test();
net::alarm *a = ss.schedule_alarm(make_callback(&t, &tt::test), 2000);
ss.schedule_alarm(make_callback(&t, &tt::test), 4000);
a->stop();

ss.loop();

return 0;
        plm::plm_connection pc("/dev/ttyUSB0", &ss);

        pc.start();

        //pc.send_command(0, "0260", make_callback(&t, &tt::on_response));
        //pc.send_command(0, "0262226A8F0F12FF", make_callback(&t, &tt::on_response));
        pc.send_command(0, "0262226A8F0F1300", make_callback(&t, &tt::on_response));

#if 0
        test_connection tc(&ss);

        ss.register_for_read(&tc);
#endif

        ss.loop();
    } catch(std::exception &ex) {
        printf("ERROR: %s\n", ex.what());
    }
}

