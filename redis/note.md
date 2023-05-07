# 2.Socket介绍

[socket APIs Doc](https://beej-zhcn.netdpi.net/)

Redis是典型的C/S结构，多个客户端通过TCP协议连接一个服务端。



```c
// 传入socket通信域（ipv4还是v6），流还是数据报，协议类型（tcp/udp）等。返回socket的文件描述符
int socket(int domain, int type, int protocol);

//将地址绑定到socket
int bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen);
//监听socket，返回0表示成功
int listen(int s, int backlog);
//当有listening的socket被连接时，返回连接socket
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
//通过socket连接指定地址
int connect(int socket, const struct sockaddr *address, socklen_t address_len);
//设置socket的option
int setsockopt(int s, int level, int optname, const void *optval,
               socklen_t optlen);


//示例
fd = socket()
bind(fd, address)
listen(fd)
while True:
    conn_fd = accept(fd)
    do_something_with(conn_fd)
    close(conn_fd)
```

# 4.协议解析

最简单的协议解析方式是根据长度来划分不同的部分。

```c
//redis中的ziplist实现
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------
```

可以使用“缓冲 IO”来减少系统调用的数量。也就是说：一次尽可能多地读取缓冲区，然后尝试解析来自该缓冲区的多个请求。

有些协议使用文本而不是二进制数据。虽然文本协议具有人类可读的优点，但文本协议确实比二进制协议需要更多的解析，后者更易于编码和错误处理。使协议解析复杂化的另一件事是，某些协议没有直接拆分消息的方法，这些协议可能使用分隔符，或者需要特定解析器来拆分消息。当协议携带任意数据时，在协议中使用分隔符会增加复杂性，因为数据中的分隔符是需要“转义”的。

# 5.事件循环和NIO

在服务器端网络编程中，有 3 种方法可以处理并发连接。它们是：fork()、多线程和事件循环。forking为每个客户端连接创建新进程以实现并发。多线程使用线程代替进程。事件循环使用poll和NIO的方式，在单个线程上处理多个客户端连接的请求。

```python
# event loop with poll()的伪代码实现
# reactor模式事件分发
all_fds = [...]
while True:
    active_fds = poll(all_fds)
    for each fd in active_fds:
        do_something_with(fd)

def do_something_with(fd):
    if fd is a listening socket:
        add_new_client(fd)
    elif fd is a client connection:
        while work_not_done(fd):
            do_something_to_client(fd)

def do_something_to_client(fd):
    if should_read_from(fd):
        data = read_until_EAGAIN(fd)
        process_incoming_data(data)
    while should_write_to(fd):
        write_until_EAGAIN(fd)
    if should_close(fd):
        destroy_client(fd)
```

使用poll()获取可以立即处理的fd。在 Linux 上，除了`poll`系统调用之外，还有 `select` 和` epoll`。select与poll基本相同，只是最大 fd 数比较多，因此它在现代应用程序中已经过时。epoll API 由 3 个系统调用组成：`epoll_create`、`epoll_wait` 和 `epoll_ctl`。epoll API 是有状态的，而不是提供一组 fds 作为系统调用参数，epoll_ctl用于操作由 epoll_create 创建的 fd ，epoll_wait会将事件从内核缓冲区中复制到用户空间缓冲区，并返回发生事件的文件描述符。

```c
//int epfd： epoll_create()函数返回的epoll实例的句柄。
//struct epoll_event * events： 接口的返回参数，epoll把发生的事件的集合从内核复制到 events数组中。events数组是一个用户分配好大小的数组，数组长度大于等于maxevents。（events不可以是空指针，内核只负责把数据复制到这个 events数组中，不会去帮助我们在用户态中分配内存）
//int maxevents： 表示本次可以返回的最大事件数目，通常maxevents参数与预分配的events数组的大小是相等的。
//int timeout： 表示在没有检测到事件发生时最多等待的时间，超时时间(>=0)，单位是毫秒ms，-1表示阻塞，0表示不阻塞。
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);

```

### 5.1poll实例

```c
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        die("socket()");
    }

    int val = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(listen_fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(listen_fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    // a map of all client connections, keyed by fd
    std::vector<Conn *> fd2conn;

    // set the listen fd to nonblocking mode
    fd_set_nb(listen_fd);

    // the event loop
    std::vector<struct pollfd> poll_args;
    while (true) {
        // prepare the arguments of the poll()
        poll_args.clear();
        // for convenience, the listening fd is put in the first position
        //初始化放入server fd
        struct pollfd pfd = {listen_fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // connection fds
        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        // poll for active fds
        // the timeout argument doesn't matter here
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if (rv < 0) {
            die("poll");
        }

        // process active connections
        //从1开始，因为0是server fd
        for (size_t i = 1; i < poll_args.size(); ++i) {
            //如果有事件发生
            if (poll_args[i].revents) {
                //从fd获取conn
                Conn *conn = fd2conn[poll_args[i].fd];
                //通过conn路由
                connection_io(conn);
                if (conn->state == STATE_END) {
                    // client closed normally, or something bad happened.
                    // destroy this connection
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        // try to accept a new connection if the listening fd is active
        if (poll_args[0].revents) {
            (void)accept_new_conn(fd2conn, listen_fd);
        }
    }

    return 0;
```

```c
//创建新连接并加入集合
static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    // creating the struct Conn
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}
```

### 5.2单线程epoll实例

```c
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        die("socket()");
    }

    int val = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(listen_fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(listen_fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    // a map of all client connections, keyed by fd
    std::vector<Conn *> fd2conn;

    // set the listen fd to nonblocking mode
    fd_set_nb(listen_fd);

    int epoll_fd = epoll_create(1);

    if(epoll_fd == -1) {
        close(epoll_fd);
        return -1;
    }
    memset(&ev,0,sizeof(ev));
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.fd = listen_fd;

    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&ev) == -1){
        return -1;
    }

    // the event loop
    std::vector<struct pollfd> poll_args;
    while (true) {

        int events_num = epoll_wait(epoll_fd,events,MAX_EVENTS,-1);
        for(int i=0;i<events_num;i++){
            ev = events[i];
            if(ev.data.fd == listen_fd){
                //建立新连接
                (void)accept_new_conn(epoll_fd, fd2conn, listen_fd);
            }
            else{
                //处理读写
                connection_io(fd2conn[ev.data.fd]);
            }
        }
    }

    return 0;
```

```c
//创建新连接并加入集合
static int32_t accept_new_conn(int epoll_fd, std::vector<Conn *> &fd2conn, int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    // creating the struct Conn
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    ev.data.fd = connfd;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,connfd,&ev) == -1) {
        close(epoll_fd);
        return -1;
    }
    conn->fd = connfd;
    conn->epoll_fd = epoll_fd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}
```

### 5.3线程池的实现

1. 定义线程池结构体

首先需要定义一个线程池结构体，包括线程池的属性和任务队列：

```c
typedef struct {
    pthread_mutex_t lock; //锁
    sem_t m_sem;          //是否有任务需要处理
    pthread_t *threads;
    Queue* task_queue;
    int thread_count;
    int task_count;

    int status; //线程池状态，0为结束，1为启动
} ThreadPool;
```

其中Task是任务结构体，包括任务函数指针和参数等：

```c
typedef struct {
    void (*function)(void *arg);  //任务函数指针
    void *arg;                    //任务参数
} Task;
```

2. 初始化线程池

初始化线程池时需要进行一些基本的设置，如初始化锁和条件变量、创建线程、启动线程池等：

```c
ThreadPool* threadpool_create(int thread_count,int task_count){
    ThreadPool* pool;

    if((pool = (ThreadPool*) malloc(sizeof(ThreadPool))) == NULL){
        return NULL;
    }

    pool->thread_count = 0;
    pool->task_count = 0;
    pool->status = 1;

    pool->task_queue = create_queue(task_count);

    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);

    pthread_mutex_init(&pool->lock,NULL);
    sem_init(&pool->m_sem, 0, 0);

    for(int i=0;i<thread_count;i++){
        if(pthread_create(&pool->threads[i],NULL,threadpool_worker,(void *)pool) != 0){
            printf("pthread create failed\n");
            destory_threadpool(pool);
            return NULL;
        }

        pool->thread_count++;
    }

    return pool;
}
```

其中threadpool_worker是线程池中的工作线程函数，用于处理任务队列中的任务。

```c
void thread_run(ThreadPool * pool){
    while(pool->status){
      	sem_wait(&pool->m_sem);
        pthread_mutex_lock(&pool->lock);
        if(is_empty(pool->task_queue)){
            pthread_mutex_unlock(&pool->lock);
            continue;
        }
        
        Task* task = (Task *)pop(pool->task_queue);
        pthread_mutex_unlock(&pool->lock);
        task->func(task->arg);
    }
}

void* threadpool_worker(void* arg){
    ThreadPool* pool = (ThreadPool*)arg;
    thread_run(pool);
    return pool;
}
```



3. 往线程池中添加任务

向线程池中添加任务时，需要加锁，将任务添加到任务队列中。

```c
int append_task(ThreadPool* pool,Task* task){
    pthread_mutex_lock(&pool->lock);

    if(is_full(pool->task_queue)){
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    push(pool->task_queue,(long)task);

    pthread_mutex_unlock(&pool->lock);
  	sem_post(&pool->m_sem);
    return 1;
}
```



# 7.Parser

## 1.DT设计

```c
//command
+------+-----+------+-----+------+-----+-----+------+
| nstr | len | str1 | len | str2 | ... | len | strn |
+------+-----+------+-----+------+-----+-----+------+
```

命令设计为一串字符串，nstr是后面字符串的数量，而len是后面字符串长度，这样可以使用最少的空间存储一个命令。

```c
//response
+-----+---------+
| res | data... |
+-----+---------+
```

res是一个32位的code，后面是字符串信息。

```c
//parser
static int32_t do_request(
    const uint8_t *req, uint32_t reqlen,
    uint32_t *rescode, uint8_t *res, uint32_t *reslen)
{
    std::vector<std::string> cmd;
    if (0 != parse_req(req, reqlen, cmd)) {
        msg("bad req");
        return -1;
    }
    if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        *rescode = do_get(cmd, res, reslen);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        *rescode = do_set(cmd, res, reslen);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        *rescode = do_del(cmd, res, reslen);
    } else {
        // cmd is not recognized
        *rescode = RES_ERR;
        const char *msg = "Unknown cmd";
        strcpy((char *)res, msg);
        *reslen = strlen(msg);
        return 0;
    }
    return 0;
}

static int32_t parse_req(

    const uint8_t *data, size_t len, std::vector<std::string> &out)
{
    if (len < 4) {
        return -1;
    }
  //先取出nstr
    uint32_t n = 0;
    memcpy(&n, &data[0], 4);
    if (n > k_max_args) {
        return -1;
    }

    size_t pos = 4;
  //循环处理nstr个字符串
    while (n--) {
        if (pos + 4 > len) {
            return -1;
        }
      //根据len取后面的字符串，放入list中
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4);
        if (pos + 4 + sz > len) {
            return -1;
        }
        out.push_back(std::string((char *)&data[pos + 4], sz));
        pos += 4 + sz;
    }

    if (pos != len) {
        return -1;  // trailing garbage
    }
    return 0;
}


```

## 2.序列化与反序列化

使用type-length-value(TLV)的格式，这是常用的序列化方法，其有几点优势：

- 像Json和XML一样易于人类直接阅读理解
- 可以编码任意的嵌套数据



# 2.数据结构

## 1.Hashtable

解决hash冲突的两种办法：

- 拉链法

- 开放地址法



### 拉链法实现hash表

#### 增删改查

```c
//hash节点
struct HNode {
    HNode *next = NULL;
    uint64_t hcode = 0;
};

// a simple fixed-sized hashtable
struct HTab {
    HNode **tab = NULL;
    size_t mask = 0;
    size_t size = 0;
};

// n must be a power of 2,此时x & n-1 就是x的hashcode
static void h_init(HTab *htab, size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);
    htab->tab = (HNode **)calloc(sizeof(HNode *), n);
    htab->mask = n - 1;
    htab->size = 0;
}

// hashtable insertion
static void h_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}
//查找，找到对应位置然后遍历链表
static HNode **h_lookup(
    HTab *htab, HNode *key, bool (*cmp)(HNode *, HNode *))
{
    if (!htab->tab) {
        return NULL;
    }

    size_t pos = key->hcode & htab->mask;
    HNode **from = &htab->tab[pos];
    while (*from) {
        if (cmp(*from, key)) {
            return from;
        }
        from = &(*from)->next;
    }
    return NULL;
}
// remove a node from the chain
//要传指针的指针，否则不能影响原表
static HNode *h_detach(HTab *htab, HNode **from) {
    HNode *node = *from;
    *from = (*from)->next;
    htab->size--;
    return node;
}
```

`malloc`和`calloc`的区别。

1、参数个数上的区别：

malloc函数：malloc(size_t size)函数有一个参数，即要分配的内存空间的大小。

calloc函数：calloc(size_t numElements,size_t sizeOfElement)有两个参数，分别为元素的数目和每个元素的大小，这两个参数的乘积就是要分配的内存空间的大小。

2、初始化内存空间上的区别：

malloc函数：不能初始化所分配的内存空间，在动态分配完内存后，里边数据是随机的垃圾数据，malloc基于brk（直接设置栈顶指针位置）或mmap。

calloc函数：能初始化所分配的内存空间，在动态分配完内存后，自动初始化该内存空间为零。

#### 渐进式rehash

```c
struct HMap {
    HTab ht1;
    HTab ht2; //使用两个hash表，避免一次性rehash占用太多时间。
    size_t resizing_pos = 0;
};
//在查找之前先进行一下help rehash
HNode *hm_lookup(
    HMap *hmap, HNode *key, bool (*cmp)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
  //现在h1中查找，若查找不到说明正在rehash，可以去h2中查找
    HNode **from = h_lookup(&hmap->ht1, key, cmp);
    if (!from) {
        from = h_lookup(&hmap->ht2, key, cmp);
    }
    return from ? *from : NULL;
}

```

```c
//设置该参数以免rehash占用太久时间
const size_t k_resizing_work = 128;

static void hm_help_resizing(HMap *hmap) {
  //如果h2没初始化，表明没有进行rehash
    if (hmap->ht2.tab == NULL) {
        return;
    }

    size_t nwork = 0;
    while (nwork < k_resizing_work && hmap->ht2.size > 0) {
        // scan for nodes from ht2 and move them to ht1
        HNode **from = &hmap->ht2.tab[hmap->resizing_pos];
        if (!*from) {
            hmap->resizing_pos++;
            continue;
        }
      //从h2中删除node，并添加到h1中
        h_insert(&hmap->ht1, h_detach(&hmap->ht2, from));
        nwork++;
    }

    if (hmap->ht2.size == 0) {
        // done
        free(hmap->ht2.tab);
        hmap->ht2 = HTab{};
    }
}
```

插入要注意在hashtable快满时触发resizing

```c
const size_t k_max_load_factor = 8;

void hm_insert(HMap *hmap, HNode *node) {
  //h1为空时要初始化
    if (!hmap->ht1.tab) {
        h_init(&hmap->ht1, 4);
    }
    h_insert(&hmap->ht1, node);
  //h2为空时，要检查是否需要扩容
    if (!hmap->ht2.tab) {
        // check whether we need to resize
      //计算负载因子，若大于阈值则需要扩容
        size_t load_factor = hmap->ht1.size / (hmap->ht1.mask + 1);
        if (load_factor >= k_max_load_factor) {
            hm_start_resizing(hmap);
        }
    }
    hm_help_resizing(hmap);
}

static void hm_start_resizing(HMap *hmap) {
    assert(hmap->ht2.tab == NULL);
    // create a bigger hashtable and swap them
  //把原h1放到h2，然后把h1初始化为两倍
    hmap->ht2 = hmap->ht1;
    h_init(&hmap->ht1, (hmap->ht1.mask + 1) * 2);
  //记录rehash的位置
    hmap->resizing_pos = 0;
}
```

删除时注意两个表都要查找

```c
HNode *hm_pop(
    HMap *hmap, HNode *key, bool (*cmp)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->ht1, key, cmp);
    if (from) {
        return h_detach(&hmap->ht1, from);
    }
    from = h_lookup(&hmap->ht2, key, cmp);
    if (from) {
        return h_detach(&hmap->ht2, from);
    }
    return NULL;
}
```

## 2.AVL树



## 数据结构的使用

```c
// the structure for the key
struct Entry {
    struct HNode node;
    std::string key;
    std::string val;
};
```

不使用数据结构包含数据，而是将哈希表节点结构嵌入到数据的Entry中。这是在 C 中创建通用数据结构的标准方法。除了使数据结构更泛用之外（因为不包含特殊数据组织），还可以减少不必要的内存管理。

```c
//把0地址强转成结构体指针，然后即可算出member到结构体首地址的偏移量
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
//先获取member的位置指针，然后通过偏移量获取结构体首地址。用于根据成员指针获取结构体指针
#define container_of(ptr, type, member) 
({                  
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    
    (type *)( (char *)__mptr - offsetof(type, member) );
})
//调用方式，可以通过HNode指针返回Entry指针
container_of(lhs, struct Entry, node);
```

[相关参考资料链接](https://cubox.pro/my/card?id=ff80808187a310740187a8db04d21bc1)

hmap的使用：

```c
// The data structure for the key space.
static struct {
    HMap db;
} g_data;

static uint32_t do_get(
    std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
  
  //从hashtable中找到对应的数据结构
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (!node) {
        return RES_NX;
    }
  //从hash node获取Entry中的val
    const std::string &val = container_of(node, Entry, node)->val;
    assert(val.size() <= k_max_msg);
  //把val拷贝到response
    memcpy(res, val.data(), val.size());
    *reslen = (uint32_t)val.size();
    return RES_OK;
}

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return lhs->hcode == rhs->hcode && le->key == re->key;
}
```

