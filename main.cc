
#include <stdio.h>

#include <sys/time.h>

#include <functional>
#include <vector>

#include "plm-connection.h"
#include "plm-util.h"
#include "select-server.h"
#include "test-connection.h"

#include "callback.h"


struct tt : public plm::plm_command_listener {
    void on_response(plm::plm_connection::plm_response r)
    {
        printf("RESP: %d\n", r.status);
        printf("DATA: ");
        for(size_t i = 0; i < r.data.size(); ++i) {
            printf("0x%x ", unsigned(r.data[i]));
        }
        printf("\n");
    }

    void test()
    {
        time_t t = time(0);
        printf("now: %s\n", ctime(&t));
    }

    void on_command(char cmd, const std::string &data)
    {
        printf("ON_CMD: %d\n", cmd);
        printf("DATA: ");
        for(size_t i = 0; i < data.size(); ++i) {
            printf("0x%x ", unsigned(data[i]));
        }
        printf("\n");
    }
};


int main()
{
    net::select_server ss;
    test_connection tc(&ss);

    ss.register_for_read(&tc);

    ss.loop();

#if 0
    tt t;

    try {
        net::select_server ss;

        plm::plm_connection pc("/dev/ttyUSB0", &ss);

        pc.add_listener(&t);
        pc.start();

        //pc.send_command(0x60, "", make_callback(&t, &tt::on_response));
        //pc.send_command(0x62, plm::hex_to_bin("226A8F0F12FF"), make_callback(&t, &tt::on_response));
        pc.send_command(0x62, plm::hex_to_bin("226A8F0F1300"), make_callback(&t, &tt::on_response));

#if 0
        test_connection tc(&ss);

        ss.register_for_read(&tc);
#endif

        ss.loop();
    } catch(std::exception &ex) {
        printf("ERROR: %s\n", ex.what());
    }
#endif
}

