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
├── [merry.h](#merryh)		--框架总入口  
├── se							--epoll/kqueue 封装  
│   ├── se-kqueue.c  
│   ├── se-util.c				--常用TCP接口封装  
│   ├── [se-util.h](#se-utilh)  
│   ├── se.c  
│   └── [se.h](#seh)  
└── common  
    ├── actionmoni-client.c		--ActionMoni 监控服务（UDP）客户端  
    ├── [actionmoni-client.h](#actionmoni-clienth)  
    ├── base64.c  
    ├── [base64.h](#base64h)  
    ├── hash.c  
    ├── [hash.h](#hashh)  
    ├── is-binary.c				--判断某个文件内容是否为二进制文件  
    ├── [is-binary.h](#is-binaryh)  
    ├── log.c					--日志系统  
    ├── [log.h](#logh)  
    ├── md5.c  
    ├── [md5.h](#md5h)  
    ├── mime.c  
    ├── [mime.h](#mimeh)  
    ├── network.c				--常用网络函数  
    ├── [network.h](#networkh)  
    ├── process.c				--常用进程管理函数  
    ├── [process.h](#processh)  
    ├── sha1.c  
    ├── [sha1.h](#sha1h)  
    ├── sha256.c  
    ├── [sha256.h](#sha256h)  
    ├── shm.c					--共享内存的常用函数封装  
    ├── [shm.h](#shmh)  
    ├── smp.c					--简单的内存分配器（支持跟踪）  
    ├── [smp.h](#smph)  
    ├── strings.c  
    ├── [strings.h](#stringsh)  
    ├── timeouts.c				--定时器  
    ├── [timeouts.h](#timeoutsh)  
    ├── times.c  
    ├── [times.h](#timesh)  
    ├── urlcoder.c  
    └── [urlcoder.h](#urlcoderh)  

------

函数接口
--

merry.h
---

框架总入口，如只使用框架里面的一些方法，可不使用

#### merry_start
int merry\_start(int argc, const char \*\*argv, void (\*help)(), void (\*master)(), void (\*onexit)(), void (\*worker)(), int worker_count);

启动方法

	int argc			--命令行参数数量，从 int main() 方法中传递
    const char **argv,	--命令行参数
    void (*help)*(),	--输出Help信息的方法，框架自动判断命令行参数并调用该方法
    void (*master)(),	--主进程方法
    void (*worker)(),	--任务方法
    void (*onexit)(),	--如捕捉到进程关闭信号会调用该方法，再执行退出
    int worker_count	--最多开启多少个worker，默认0，指无限制，让框架自动按CPU核心数量进行设置
    

se.h
---

网络IO事件驱动方法的封装，支持 Linux epoll 和 BSD kqueue

    typedef int (*se_rw_proc_t)(se_ptr_t *ptr);
    typedef int (*se_waitout_proc_t)();

#### se\_create
`int se_create(int event_size);`

创建IO事件驱动的文件描述符

	int event_size		--事件循环的数组大小，一般为 4096

#### se\_loop

IO事件驱动的主体循环

`int se_loop(int loop_fd, int waitout, se_waitout_proc_t waitout_proc);`

	int loop_fd,		--IO事件驱动的文件描述符
    int waitout,		--主体循环过程中的超时设置，超时后可执行 waitout_proc 方法，再进入循环。以获取CPU处理的时间片段
    se_waitout_proc_t waitout_proc	--主体循环中的子方法，用于处理些IO事件以外的小任务（注意不能长时间堵塞，子任务应该能在10毫秒内完成，否则请开线程处理）
    

#### se\_add

绑定IO事件

`se_ptr_t *se_add(int loop_fd, int fd, void *data);`

	int loop_fd,		--IO事件驱动的文件描述符
    int fd,				--IO事件的文件描述符，如某个 client fd
    void *data			--针对该事件的额外数据
    
    返回 se_ptr_t * 类型的结构体

#### se\_delete

删除IO事件

`int se_delete(se_ptr_t *ptr);`

	se_ptr_t *ptr		--IO事件的结构体
    
    返回值 1 或 0

#### se\_be\_read

设置IO事件将可读取

`int se_be_read(se_ptr_t *ptr, se_rw_proc_t func);`

	se_ptr_t *ptr		--IO事件的结构体
    se_rw_proc_t func	--触发事件后对应执行的方法
    
    返回值 1 或 0

#### se\_be\_write

设置IO事件将可写入

`int se_be_write(se_ptr_t *ptr, se_rw_proc_t func);`

	se_ptr_t *ptr		--IO事件的结构体
    se_rw_proc_t func	--触发事件后对应执行的方法
    
    返回值 1 或 0

#### se\_be\_pri

设置IO事件将等待（暂不处理读写事件）

`int se_be_pri(se_ptr_t *ptr, se_rw_proc_t func);`

	se_ptr_t *ptr		--IO事件的结构体
    se_rw_proc_t func	--触发事件后对应执行的方法，一般为 NULL
    
    返回值 1 或 0

#### se\_be\_rw

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

#### se\_accept

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

#### se\_dns\_query

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

#### se\_connect

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

actionmoni-client.h
---

ActionMoni 监控服务客户端（UDP协议）

#### actionmoni\_open

连接到 ActionMoni 服务器

`int actionmoni_open(const char *host, int port);`

	const char *host,	--服务器IP
    int port			--服务器端口
    
    返回连接的文件描述符

#### actionmoni\_count

计数器加一操作

`int actionmoni_count(const char *_key);`

	const char *_key	--KEY

#### actionmoni\_counts

计数器加n操作

`int actionmoni_counts(const char *_key, uint32_t cs);`

	const char *_key,	--KEY
    uint32_t cs			--加n,n>=0

#### actionmoni\_ts

耗时|频率的计数操作

`int actionmoni_ts(const char *_key, int ts);`

	const char *_key,	--KEY
    int ts				--耗时|频率

#### actionmoni\_multi

批量操作

`int actionmoni_multi(int cnt, ...);`

	int cnt,			--该次批量操作的总操作数量（<= 10）
    ...					--具体的操作内容

#### actionmoni\_set\_keys

在ActionMoni服务器上标记KEYS

`int actionmoni_set_keys(const char *_keys, int _len);`

	const char *_keys,	--KEYS，以逗号间隔
    int _len			--字符串长度（<= 4080）

base64.h
---

base64 编解码方法

#### base64\_encoded\_length

计算字符串进行base64编码的可能长度

`#define base64_encoded_length(len) (((len + 2) / 3) * 4)`

#### base64\_decoded\_length

计算base64字符串进行解码后的可能长度

`#define base64_decoded_length(len) (((len + 3) / 4) * 3)`

#### base64_encode

base64 编码方法

`int base64_encode(unsigned char *dst, const unsigned char *src, int len);`

#### base64_decode

base64 解码方法

`int base64_decode(unsigned char *dst, const unsigned char *src, size_t slen);`

#### base64\_decode\_url

base64 解码方法（URL）

`int base64_decode_url(unsigned char *dst, const unsigned char *src, size_t slen);`

hash.h
---

常用哈希方法

#### fnv1a_32

`uint32_t fnv1a_32(const unsigned char *data, uint32_t len);`

#### fnv1a_64
`uint32_t fnv1a_64(const unsigned char *data, uint32_t len);`

#### MurmurHash64A
`uint64_t MurmurHash64A(const void *key, int len, unsigned int seed);`

#### MurmurHash2
`unsigned int MurmurHash2(const void *key, int len, unsigned int seed);`

#### XXH32
`unsigned int XXH32(const void *input, int len, unsigned int seed);`

#### XXH32_init
`void *XXH32_init(unsigned int seed);`

#### XXH32_update
`XXH_errorcode XXH32_update(void *state, const void *input, int len);`

#### XXH32_digest
`unsigned int XXH32_digest(void *state);`


is-binary.h
---

判断内容是否二进制格式

#### is_binary

`int is_binary(const char *buf, int buf_len);`

log.h
---

    #define DEBUG 1
    #define INFO 2
    #define NOTICE 3
    #define WARN 4
    #define ALERT 5
    #define ERR 6

日志系统，框架会自动判断程序执行的命令行参数，默认不写入 log 文件，如以 daemon 模式启动，也不会在屏幕输出 log，如非 daemon 模式启动会输出到屏幕。
执行程序的命令参数为：log=/path/filename,level
如指定 log 文件名，会同时写入log文件（支持多进程模式）

#### LOGF

记录日志

`#define LOGF(l,a,...)`

	l	--Level,(DEBUG|INFO|NOTICE|WARN|ALERT|ERR)
    a	--Format,%s%d （兼容 printf 所支持的格式）
    ...	--日志数据

#### open_log

打开日志文件

`logf_t *open_log(const char *fn, int sz);`

	const char *fn,	--日志文件名
    int sz			--共享内存中的缓存大小（建议 40KB）
    
    返回日志文件的结构体

#### log_destory

关闭并销毁日志文件结构体

`void log_destory(logf_t *logf);`

	logf_t *logf	--日志文件结构体

#### log_writef

写入日志

`int log_writef(logf_t *logf, const char *fmt, ...);`

	logf_t *logf,	--日志文件结构体
    const char *fmt,--格式，%s%d （兼容 printf 所支持的格式）
    ...				--日志数据

#### sync_logs

日志文件写入磁盘

`int sync_logs(logf_t *logf);`

	logf_t *logf	--日志文件结构体

md5.h
---

md5 方法

#### MD5Init

初始化MD5结构体

`void MD5Init(MD5_CTX *);`

	MD5_CTX *	--MD5结构体

#### MD5Update

输入内容

`void MD5Update(MD5_CTX *, const unsigned char *, unsigned int);`

	MD5_CTX *,				--MD5结构体
    const unsigned char *,	--内容
    unsigned int			--内容长度

#### MD5Final

获取结果

`void MD5Final(unsigned char [16], MD5_CTX *);`

	unsigned char [16],		--接收结果的char数组
    MD5_CTX *				--MD5结构体

#### md5

md5 方法（对以上3个方法的简化封装）

`void md5(const unsigned char *data, size_t len, char *hex);`

	const unsigned char *data,	--内容
    size_t len,					--内容长度
    char *hex					--接收结果的char数组（size >= 32）

mime.h
---

根据文件扩展名判断 MIME 类型

#### get\_mime\_type

获取 MIME 类型

`const char *get_mime_type(const char *filename);`

	const char *filename		--文件名
    
    返回值为 “text/plain” 类字符串

network.h
---

常用网络IO操作方法

#### set_nonblocking

设置文件描述符为非堵塞或堵塞

`int set_nonblocking(int fd, int blocking);`

	int fd,			--文件描述符
    int blocking	--是否堵塞

#### network_bind

绑定端口

`int network_bind(const char *addr, int port);`

	const char *addr,	--需绑定的IP
    int port			--需绑定的端口

#### network\_raw\_send

发送数据（堵塞模式）

`int network_raw_send(int client_fd, const char *contents, int length);`

	int client_fd,			--文件描述符
    const char* contents,	--内容
    int length				--内容长度

#### network\_raw\_read

读取数据（堵塞模式）

`char *network_raw_read(int cfd, int *datas_len);`

	int cfd,				--文件描述符
    int *datas_len			--读取到的长度
    
    返回值为 char* 类型

#### network\_raw\_sendfile

发送文件（堵塞模式）

`int network_raw_sendfile(int out_fd, int in_fd, off_t *offset, size_t count);`

	int out_fd,		--文件描述符
    int in_fd,		--待发送的文件描述符
    off_t *offset,	--
    size_t count	--

process.h
---

常用进程管理方法

#### get\_cpu\_num

获取CPU数量

`int get_cpu_num();`

#### set\_cpu\_affinity

设置当前进程的CPU亲和性

`int set_cpu_affinity(uint32_t active_cpu);`

#### set\_process\_title

设置当前进程的命令行标题（如top列表上的显示内容）

`void set_process_title(const char *title, int is_master);`

#### init\_process\_title

初始化当前进程的命令行标题（仅主进程启动时调用）

`char *init_process_title(int argc, const char **argv);`

#### start\_master\_main

执行主进行任务方法

`void start_master_main(void (*func)(), void (*onexit)());`

#### daemonize

进入daemon后台守护进程模式

`void daemonize();`

#### attach\_on\_exit

捕获程序退出信号

`void attach_on_exit(void *fun);`

#### set\_process\_user

设置当前进程的用户、用户组

`void set_process_user(const char *user, const char *group);`

#### getarg

获取命令行参数

`char *getarg(const char *key);`

#### fork_process

fork子进程

`int fork_process(void (*func)(int i));`

#### new\_thread\_p

创建线程

`int new_thread_p(void *func, void *i);`

sha1.h
---

`void sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);`

`int sha1_file(const char *path, unsigned char output[20]);`

`void sha1_hmac(const unsigned char *key, size_t keylen, const unsigned char *input, size_t ilen, unsigned char output[20]);`

sha256.h
---

shm.h
---

共享内存操作方法

`shm_t *shm_malloc(size_t size);`
`void shm_free(shm_t *shm);`
`int shm_lock(shm_t *shm);`
`int shm_unlock(shm_t *shm);`

smp.h
---

简单的内存分配器，引入merry.h或smp.h后，宏定义替换系统的malloc/realloc/free函数（需注意多个c文件之间是否一致，否则混合使用会导致内存错误）
（带调试支持）默认禁用调试，如需开启请在编译代码时加入 -D SMPDEBUG 参数（所有c代码）

#### dump\_smp\_link

输出当前内存分配器中的内存块列表（需打开SMPDEBUG模式）

`void dump_smp_link();`

`void *smp_malloc(unsigned int size);`

`void *smp_realloc(void *p, unsigned int _size);`

`int smp_free(void *p);`

strings.h
---

常用字符串方法

#### stricmp

字符串匹配（大小写不敏感）

`int stricmp(const char *str1, const char *str2);`

#### stristr

字符串搜索（大小写不敏感）

`char *stristr(const char *str, const char *pat, int length);`

#### random_string

产生随机字符串（性能不好，不建议频密使用）

`void random_string(char *string, size_t length, int s);`

#### _strtoul

字符串转换为unsigned long数字（支持 2～64 进制）

`unsigned long _strtol(char *str64, int base);`

#### _ultostr

把unsigned long数字转换为字符串（支持 2～64 进制）

`char *_ltostr(char *str, long val, unsigned base);`

#### strsplit

字符串切分方法（不会对原字符串进行任何修改）

`char *strsplit(const void *string_org, int org_len, const char *demial, char **last, int *len);`

	char *str = "abc,,ee,dd";
    int str_len = strlen(str);
    
    char *tok = NULL;
    char *last = NULL;
    int len = 0;
    
    tok = strsplit(str, str_len, ",", &last, &len);
    while(tok){
    	//...
    	tok = strsplit(str, str_len, ",", &last, &len);
    }

timeouts.h
---

定时器方法

#### add_timeout

创建一个定时任务

`timeout_t *add_timeout(void *ptr, int timeout, timeout_handle_cb handle);`

#### update_timeout

更新定时任务的到期时间

`void update_timeout(timeout_t *n, int timeout);`

#### delete_timeout

删除一个定时任务，注意:传入的timeout_t结构体会被 free

`void delete_timeout(timeout_t *n);`

#### check_timeouts

处理到期任务，一般在 epoll loop 内定期执行

`int check_timeouts();`

times.h
---

常用时间方法

#### longtime

获取当前时间（毫秒）
`long longtime();`

#### update_time

更新时间记录

`time_t update_time();`

#### 全局变量

    time_t now = 0;					--当前时间
    char now_gmt[32] = {0};			--当前时间GMT格式字符串
    char now_lc[32] = {0};			--当前本地时间字符串

使用以上全局变量，请注意定期执行 update_time 更新时间记录

urlcoder.h
---

`uintptr_t urlencode(u_char *dst, u_char *src, size_t size, unsigned int type);`

`void urldecode(u_char **dst, u_char **src, size_t size, unsigned int type);`