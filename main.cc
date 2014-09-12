
#include <stdio.h>

#include "shd-app.h"
#include "shd-config.h"
#include "select-server.h"


int main()
{
    net::select_server ss;
    shd_config c;
    shd_app a(&c, &ss, &ss, &ss);

    a.run();

    ss.loop();
}

