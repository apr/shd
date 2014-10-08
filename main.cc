
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger.h"
#include "shd-app.h"
#include "shd-config.h"
#include "select-server.h"


void usage(const char *exec_name)
{
    printf("Usage: %s [-d] [-h]\n", exec_name);
}


int main(int argc, char *argv[])
{
    int opt;
    bool debug_mode = false;

    while((opt = getopt(argc, argv, "dh")) != -1) {
        switch(opt) {
        case 'd':
            debug_mode = true;
            break;

        case 'h':
            usage(argv[0]);
            exit(0);

        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if(optind < argc) {
        usage(argv[0]);
        exit(1);
    }

    if(!debug_mode) {
        // Daemonize now. Do not change the working directory, close all
        // descriptors.
        if(daemon(true, false) == -1) {
            perror("Could not daemonize");
            exit(1);
        }
    }

    net::select_server ss;
    shd_config c;
    shd_app a(&c, &ss, &ss, &ss);

    a.run();

    if(!debug_mode) {
        log_info("started");
    }

    ss.loop();
}

