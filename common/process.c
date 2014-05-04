#include "process.h"
#include "times.h"
#include "log.h"

char hostname[1024];
char process_chdir[924];
char process_name[100];
int is_daemon = 0;
int process_count = 1;
int pid = -1;

int server_fd = 0;
int loop_fd = 0;

extern logf_t *LOGF_T;

static const char *process_char_last;

static char buf_4096[4096] = {0};

int get_cpu_num()
{
    FILE *f = fopen("/proc/cpuinfo", "r");

    if(f == NULL) {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

    int cpu_cores = 0;
    int phisical_id = 0;

    while(fgets(buf_4096, 4096, f)) {
        if(strstr(buf_4096, "cpu cores")) {
            char *x = strchr(buf_4096, ':');

            if(x) {
                x += 1;
                cpu_cores = atoi(x);
            }

        } else if(strstr(buf_4096, "physical id")) {
            char *x = strchr(buf_4096, ':');

            if(x) {
                x += 1;
                int temp = atoi(x);

                if(temp > phisical_id) {
                    phisical_id = temp;
                }
            }
        }
    }

    fclose(f);

    return cpu_cores * (phisical_id + 1);
}

int set_cpu_affinity(uint32_t n)
{
#ifdef linux
    int m = 0;
    cpu_set_t cpu_mask;

    m = sysconf(_SC_NPROCESSORS_ONLN);
    n = n % m;

    if(sched_getaffinity(0, sizeof(cpu_set_t), &cpu_mask) == -1) {
        return 0;
    }

    CPU_ZERO(&cpu_mask);
    CPU_SET(n, &cpu_mask);

    if(sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask) == -1) {
        return 0;
    }

    return 1;
#else
    return 0;
#endif
}

void set_process_title(const char *title, int is_master)
{
    pid = getpid();
    _argv[1] = 0;
    char *p = (char *)_argv[0];
    bzero(p, process_char_last - p);

    if(is_master) {
        snprintf(p, process_char_last - p, "%s: %s %s%s", process_name, title,
                 process_chdir, process_name);
        int i = 1;

        for(i = 1; i < _argc; i++) {
            sprintf(p, "%s %s", p, environ[i]);
        }

    } else if(is_daemon) {
        snprintf(p, process_char_last - p, "%s: %s", process_name, title);
    }
}

char *init_process_title(int argc, const char **argv)
{
    pid = getpid();
    _argc = argc;
    _argv = argv;
    size_t n = 0;
    int i = 0;
#ifdef linux
    n = readlink("/proc/self/exe" , process_chdir , sizeof(process_chdir));
#else
    uint32_t new_argv0_s = sizeof(process_chdir);

    if(_NSGetExecutablePath(process_chdir, &new_argv0_s) == 0) {
        n = strlen(process_chdir);
    }

#endif
    i = n;

    int cped = 0;

    while(n > 1) {
        if(process_chdir[n--] == '/') {
            if(cped++ == 0) {
                strncpy(process_name, ((char *) process_chdir) + n + 2, i - n);
            }

            if(process_chdir[n] == '.') {
                n--;
                continue;
            }

            process_chdir[n + 2] = '\0';
            break;
        }
    }

    n = chdir(process_chdir);

    n = 0;

    for(i = 0; argv[i]; ++i) {
        n += strlen(argv[i]) + 1;
    }

    static char *raw_environ = NULL;

    if(raw_environ) {
        free(raw_environ);
    }

    raw_environ = malloc(n);

    char *raw = raw_environ;

    for(i = 0; argv[i]; ++i) {
        memcpy(raw, argv[i], strlen(argv[i]) + 1);
        environ[i] = raw;
        raw += strlen(argv[i]) + 1;
    }

    process_char_last = argv[0];
    /*
    for(i = 0; i < argc; ++i) {
        process_char_last += strlen(argv[i]) + 1;
    }*/

    for(i = 0; environ[i]; i++) {
        process_char_last += strlen(environ[i]) + 1;
    }

    return process_chdir;
}

static int _signal_handler = 0;
int check_process_for_exit()
{
    return _signal_handler == 1;
}

int signal_handler(int sig, siginfo_t *info, void *secret)
{
    switch(sig) {
        case SIGHUP:
            break;

        case SIGTERM:
            _signal_handler = 1;
            break;
    }

    return 0;
}

void daemonize()
{
    int i = 0;

    if(getppid() == 1) {
        return;
    }

    i = fork();

    if(i < 0) {
        exit(1);
    }

    if(i > 0) {
        exit(0);
    }

    setsid();

    struct sigaction mc;
    mc.sa_handler = (void *)signal_handler;
    sigemptyset(&mc.sa_mask);
    mc.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGHUP, &mc, NULL);    /// 终端挂起
    sigaction(SIGTERM, &mc, NULL);   /// 终止信号

    /// 忽略以下信号
    mc.sa_handler = SIG_IGN;

    sigaction(SIGCHLD, &mc, 0);
    sigaction(SIGTSTP, &mc, 0);
    sigaction(SIGTTOU, &mc, 0);
    sigaction(SIGTTIN, &mc, 0);
    sigaction(SIGPIPE, &mc, 0);

    is_daemon = 1;
}

static void (*on_master_exit_func)() = NULL;
static void (*on_worker_exit_func)() = NULL;
static void on_process_exit_handler(int sig, siginfo_t *info, void *secret)
{
    if(on_worker_exit_func) {
        copy_buf_to_shm_log_buf(LOGF_T);
        on_worker_exit_func();
        copy_buf_to_shm_log_buf(LOGF_T);
        smp_free_all();
    }

    if(on_master_exit_func) {
        on_master_exit_func();
        log_destory(LOGF_T);
        smp_free_all();
    }

    exit(0);
}

void attach_on_exit(void *fun)
{
    struct sigaction mc;
    mc.sa_handler = (void *)on_process_exit_handler;
    on_worker_exit_func = fun;
    sigemptyset(&mc.sa_mask);
    mc.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGHUP, &mc, NULL);     /// 终端挂起
    sigaction(SIGINT, &mc, NULL);     /// CTRL+C
    sigaction(SIGCHLD, &mc, NULL);    /// 子进程退出

    sigaction(SIGQUIT, &mc, NULL);    //键盘的退出键
    sigaction(SIGILL, &mc, NULL);     //非法指令
    sigaction(SIGABRT, &mc, NULL);    //由 abort(3) 发出的退出指令
    sigaction(SIGFPE, &mc, NULL);     //浮点异常

    sigaction(SIGSEGV, &mc, NULL);    //无效的内存引用
    sigaction(SIGALRM, &mc, NULL);    //由 alarm(2) 发出的信号
    sigaction(SIGTERM, &mc, NULL);    //终止信号
    sigaction(SIGUSR1, &mc, NULL);    //用户自定义
    sigaction(SIGUSR2, &mc, NULL);    //用户自定义

    sigaction(SIGBUS, &mc, NULL);     //总线错误（内存访问错误）
    sigaction(SIGSYS, &mc, NULL);     //非法的系统调用

    sigaction(SIGTRAP, &mc, NULL);    //跟踪/断点自陷
    sigaction(SIGXCPU, &mc, NULL);    //超过CPU时限
    sigaction(SIGXFSZ, &mc, NULL);    //超过文件长度限制
    sigaction(SIGIOT, &mc, NULL);     //IOT自陷

    /// 忽略以下信号
    mc.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &mc, 0);
    sigaction(SIGTSTP, &mc, 0);       //控制终端（tty）上按下停止键
    sigaction(SIGTTIN, &mc, 0);       //后台进程企图从控制终端读
    sigaction(SIGTTOU, &mc, 0);       //后台进程企图从控制终端写
}

static int _workerprocess[200] = {0};
static int _workerprocess_count = 0;
static void (*_workerprocess_func[200])();
int fork_process(void (*func)())
{
    int ret = fork();

    if(ret == 0) {
        set_process_title("worker process", 0);
        set_cpu_affinity(_workerprocess_count);
        func(_workerprocess_count);
    }

    if(ret > 0) {
        _workerprocess[_workerprocess_count] = ret;
        _workerprocess_func[_workerprocess_count] = func;
        _workerprocess_count++;
    }

    return ret;
}

void safe_process()
{
    int i;

    for(i = 0; i < _workerprocess_count; i++) {
        if(waitpid(_workerprocess[i], NULL, WNOHANG) == -1) {
            int ret = fork();

            if(ret == 0) {
                set_process_title("worker process", 0);
                set_cpu_affinity(i);
                _workerprocess_func[i](i);
            }

            _workerprocess[i] = ret;
        }
    }
}

void start_master_main(void (*func)(), void (*onexit)())
{
    set_process_title("master process", 1);

    if(is_daemon == 0) {
        on_master_exit_func = onexit;
    }

    while(1) {
        // master 的主要工作是维护子进程的状态，如退出了会自动重启
        if(check_process_for_exit()) {
            // 检查是否收到 kill 退出信号，如果是则关闭所有子进程后，再退出 master
            kill(0, SIGTERM); // 关闭子进程

            wait_for_child_process_exit();

            if(onexit) {
                onexit();
            }

            log_destory(LOGF_T);
            smp_free_all();

            exit(0);

        } else {
            // 维护子进程的状态，如退出了会自动重启
            safe_process();
        }

        update_time();
        func();
        sleep(1);
    }
}

void wait_for_child_process_exit()
{
    int i = 0, k = 0;

    while(1) {
        k = 0;

        for(i = 0; i < _workerprocess_count; i++) {
            if(_workerprocess[i] == 0 || waitpid(_workerprocess[i], NULL, WNOHANG) == -1) {
                _workerprocess[i] = 0;
                k++;
            }
        }

        if(k == _workerprocess_count) {
            break;
        }

        sleep(1);
    }

    log_destory(LOGF_T);
}

void set_process_user(const char *user, const char *group)
{
    struct passwd *pw;
    const char *username = "nobody";
    const char *groupname = "nobody";

    if(user) {
        username = user;
    }

    if(group) {
        groupname = group;
    }

    pw = getpwnam(username);

    if(!pw) {
        printf("user %s is not exist\n", username);
        exit(1);
    }

    if(setuid(pw->pw_uid)) {
        setgid(pw->pw_gid);
        return;
    }

    struct group *grp = getgrnam(groupname);

    if(grp) {
        setgid(grp->gr_gid);

    } else {
        grp = getgrnam(groupname);

        if(grp) {
            setgid(grp->gr_gid);
        }
    }
}

char *getarg(const char *key)
{
    int i = 0;
    int addp;

    while(i < _argc) {
        addp = environ[i][0] == '-'
               && environ[i][1] == '-' ? 2 : (environ[i][0] == '-' ? 1 : 0);

        if(strncmp(environ[i] + addp, key, strlen(key)) == 0
           && (environ[i][strlen(key) + addp] == '='
               || environ[i][strlen(key) + addp] == '\0')) {
            return environ[i] + strlen(key) + addp + 1;
        }

        i++;
    }

    return NULL;
}

int new_thread(void *func)
{
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int i = 0;

    if(pthread_create(&thread, &attr, func, (void *)&i)) {
        return 0;
    }

    return 1;
}

int new_thread_p(void *func, void *i)
{
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if(pthread_create(&thread, &attr, func, i)) {
        return 0;
    }

    return 1;
}
