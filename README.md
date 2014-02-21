Merry
===
网络服务层开发框架

特性
---
* 多进程 epoll 驱动模型（兼容 kqueue）
* 高性能 timeout 事件管理器
* 进程管理、自我维护
* 高性能日志接口
* 共享内存接口
* 精简的内存池实现
* 常用字符串方法（urlcoder / base64）

文件列表
---

.  
├── README.md  
├── merry.c  
├── [merry.h](#merry_start)		--框架总入口  
├── se							--epoll/kqueue 封装  
│   ├── se-kqueue.c  
│   ├── se-util.c				--常用TCP接口封装  
│   ├── [se-util.h](#se-util.h)  
│   ├── se.c  
│   └── [se.h](#se.h)  
└── common  
    ├── actionmoni-client.c		--ActionMoni 监控服务客户端  
    ├── actionmoni-client.h  
    ├── base64.c  
    ├── base64.h  
    ├── hash.c  
    ├── hash.h  
    ├── is-binary.c				--判断某个文件内容是否为二进制文件  
    ├── is-binary.h  
    ├── log.c					--日志系统  
    ├── log.h  
    ├── md5.c  
    ├── md5.h  
    ├── mime.c  
    ├── mime.h  
    ├── network.c				--常用网络函数  
    ├── network.h  
    ├── process.c				--常用进程管理函数  
    ├── process.h  
    ├── sha1.c  
    ├── sha1.h  
    ├── sha256.c  
    ├── sha256.h  
    ├── shm.c					--共享内存的常用函数封装  
    ├── shm.h  
    ├── smp.c					--简单的内存分配器（支持跟踪）  
    ├── smp.h  
    ├── strings.c  
    ├── strings.h  
    ├── timeouts.c				--定时器  
    ├── timeouts.h  
    ├── times.c  
    ├── times.h  
    ├── urlcoder.c  
    └── urlcoder.h  

------

函数接口
--

merry.h
---

框架总入口，如只使用框架里面的一些方法，可不使用

####merry_start
int merry\_start(int argc, const char \*\*argv, void (\*help)(), void (\*master)(), void (\*onexit)(), void (\*worker)(), int worker_count);

启动方法

	int argc			--命令行参数数量，从 int main() 方法中传递
    const char **argv,	--命令行参数
    void (*help)*(),	--输出Help信息的方法，框架自动判断命令行参数并调用该方法
    void (*master)(),	--主进程方法
    void (*worker)(),	--任务方法
    void (*onexit)(),	--如捕捉到进程关闭信号会调用该方法，再执行退出
    int worker_count	--最多开启多少个worker，默认0，指无限制，让框架自动按CPU核心数量进行设置
    

###se.h

网络IO事件驱动方法的封装，支持 Linux epoll 和 BSD kqueue

    typedef int (*se_rw_proc_t)(se_ptr_t *ptr);
    typedef int (*se_waitout_proc_t)();

####se\_create
`int se_create(int event_size);`

创建IO事件驱动的文件描述符

	int event_size		--事件循环的数组大小，一般为 4096

####se\_loop

IO事件驱动的主体循环

`int se_loop(int loop_fd, int waitout, se_waitout_proc_t waitout_proc);`

	int loop_fd,		--IO事件驱动的文件描述符
    int waitout,		--主体循环过程中的超时设置，超时后可执行 waitout_proc 方法，再进入循环。以获取CPU处理的时间片段
    se_waitout_proc_t waitout_proc	--主体循环中的子方法，用于处理些IO事件以外的小任务（注意不能长时间堵塞，子任务应该能在10毫秒内完成，否则请开线程处理）
    

####se\_add

绑定IO事件

`se_ptr_t *se_add(int loop_fd, int fd, void *data);`

	int loop_fd,		--IO事件驱动的文件描述符
    int fd,				--IO事件的文件描述符，如某个 client fd
    void *data			--针对该事件的额外数据
    
    返回 se_ptr_t * 类型的结构体

####se\_delete

删除IO事件

`int se_delete(se_ptr_t *ptr);`

	se_ptr_t *ptr		--IO事件的结构体
    
    返回值 1 或 0

####se\_be\_read

设置IO事件将可读取

`int se_be_read(se_ptr_t *ptr, se_rw_proc_t func);`

	se_ptr_t *ptr		--IO事件的结构体
    se_rw_proc_t func	--触发事件后对应执行的方法
    
    返回值 1 或 0

####se\_be\_write

设置IO事件将可写入

`int se_be_write(se_ptr_t *ptr, se_rw_proc_t func);`

	se_ptr_t *ptr		--IO事件的结构体
    se_rw_proc_t func	--触发事件后对应执行的方法
    
    返回值 1 或 0

####se\_be\_pri

设置IO事件将等待（暂不处理读写事件）

`int se_be_pri(se_ptr_t *ptr, se_rw_proc_t func);`

	se_ptr_t *ptr		--IO事件的结构体
    se_rw_proc_t func	--触发事件后对应执行的方法，一般为 NULL
    
    返回值 1 或 0

####se\_be\_rw

设置IO事件将可读取或写入

`int se_be_rw(se_ptr_t *ptr, se_rw_proc_t rfunc, se_rw_proc_t wfunc);`

	se_ptr_t *ptr		--IO事件的结构体
    se_rw_proc_t rfunc	--触发事件后对应执行的读取方法
    se_rw_proc_t wfunc	--触发事件后对应执行的写入方法
    
    返回值 1 或 0

se-util.h
---

    typedef void (*se_be_accept_cb)(int fd, struct in_addr client_addr);
    typedef void (*se_be_dns_query_cb)(void *data, struct sockaddr_in addr);
    typedef void (*se_be_connect_cb)(void *data, int fd);

####se\_accept

设置IO事件将可收到网络连接请求

`int se_accept(int loop_fd, int server_fd, se_be_accept_cb _be_accept);`

	int loop_fd,				--IO事件驱动的文件描述符
    int server_fd,				--服务端的IO事件文件描述符
    se_be_accept_cb _be_accept	--触发事件后对应执行的 accept 方法
    
    返回结果 1 或 0
    
    //typedef void (*se_be_accept_cb)(int fd, struct in_addr client_addr);
    _be_accept 接收方法中，
    	int fd 为该次连接新建的文件描述符，
        client_addr 为客户端IP

####se\_dns\_query

发起一个异步 DNS 查询

`int se_dns_query(int loop_fd, const char *name, int timeout, se_be_dns_query_cb cb, void *data);`

	int loop_fd,			--IO事件驱动的文件描述符
    const char *name,		--需查询的域名
    int timeout,			--查询超时时间（秒）
    se_be_dns_query_cb cb,	--接收查询结果的方法
    void *data				--该次查询所带的额外数据
    
    返回值 1 或 0
    
    //typedef void (*se_be_dns_query_cb)(void *data, struct sockaddr_in addr);
    cb 接收方法中，
    	void *data 为该次查询所带的额外数据，
        addr 为查询结果，如 addr == {0} 表示查询失败（查询超时也返回 {0}）
        
    另外可通过全局变量 se_errno 判断错误类型，如 se_errno == SE_DNS_QUERY_TIMEOUT

####se\_connect

发起一个异步网络连接请求

`int se_connect(int loop_fd, const char *host, int port, int timeout, se_be_connect_cb _be_connect, void *data);`

	int loop_fd,					--IO事件驱动的文件描述符
    const char *host,				--服务端IP或域名（自动发起异步DNS查询）
    int port,						--服务端端口
    int timeout,					--连接超时时间（秒）
    se_be_connect_cb _be_connect,	--接收连接结果的方法
    void *data						--该次连接所带的额外数据
    
    返回值 1 或 0
    
    //typedef void (*se_be_connect_cb)(void *data, int fd);
    _be_connect 接收方法中，
    	void *data 为该次连接所带的额外数据，
        int fd 为连接的文件描述符
    
    如 fd < 0 即连接失败（连接超时也为 -1）
    另外可通过全局变量 se_errno 判断错误类型，如 se_errno == SE_CONNECT_TIMEOUT
