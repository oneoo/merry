#include "merry.h"

char bind_addr[20] = {0};
int bind_port = 1111;
char ssl_bind_addr[20] = {0};
int ssl_bind_port = 0;
const char *program_name = NULL;

int merry_start(int argc, const char **argv, void (*help)(), void (*master)(), void (*onexit)(), void (*worker)(),
                int worker_count)
{
    update_time();

    /// 初始化进程命令行信息
    init_process_title(argc, argv);

    int i = strlen(argv[0]);

    while(argv[0][--i] != '/');

    program_name = argv[0] + i + 1;

    if(getarg("help")) {
        help();
        exit(0);
    }

    if(getarg("log")) {
        LOGF_T = open_log(getarg("log"), 40960); // filename, bufsize
    }

    /// 把进程放入后台
    if(getarg("daemon")) {
        daemonize();
    }

    process_count = 1;

    if(is_daemon == 1) {
        process_count = atoi(getarg("daemon"));

        if(process_count < 1) {
            process_count = get_cpu_num();
        }

        if(process_count < 1) {
            process_count = 1;
        }
    }

    if(worker_count > 0 && process_count > worker_count) {
        process_count = worker_count;
    }

    sprintf(bind_addr, "0.0.0.0");

    if(getarg("bind")) {
        if(strstr(getarg("bind"), ".")) {
            sprintf(bind_addr, "%s", getarg("bind"));

        } else {
            int _be_port = atoi(getarg("bind"));

            if(_be_port > 0) {
                bind_port = _be_port;
            }
        }
    }

    char *_port = strstr(bind_addr, ":");

    if(_port) {
        bind_addr[strlen(bind_addr) - strlen(_port)] = '\0';
        _port = _port + 1;

        if(atoi(_port) > 0 && atoi(_port) < 99999) {
            bind_port = atoi(_port);
        }
    }

    sprintf(ssl_bind_addr, "0.0.0.0");

    if(getarg("ssl-bind")) {
        if(strstr(getarg("ssl-bind"), ".")) {
            sprintf(ssl_bind_addr, "%s", getarg("ssl-bind"));
            _port = strstr(ssl_bind_addr, ":");

            if(_port) {
                ssl_bind_addr[strlen(ssl_bind_addr) - strlen(_port)] = '\0';
                _port = _port + 1;

                if(atoi(_port) > 0 && atoi(_port) < 99999) {
                    ssl_bind_port = atoi(_port);
                }
            }

        } else {
            int _be_port = atoi(getarg("ssl-bind"));

            if(_be_port > 0) {
                ssl_bind_port = _be_port;
            }
        }
    }

    server_fd = network_bind(bind_addr, bind_port);

    if(ssl_bind_port > 0) {
        ssl_server_fd = network_bind(ssl_bind_addr, ssl_bind_port);
        LOGF(INFO, "bind %s:%d ssl:%d", bind_addr, bind_port, ssl_bind_port);

    } else {
        LOGF(INFO, "bind %s:%d", bind_addr, bind_port);
    }

    for(i = 0; i < process_count; i++) {
        if(is_daemon == 1) {
            fork_process(worker);

        } else {
            set_cpu_affinity(0);
            new_thread_p(worker, 0);
        }
    }

    /// 进入主进程处理
    start_master_main(master, onexit);

    return 1;
}
