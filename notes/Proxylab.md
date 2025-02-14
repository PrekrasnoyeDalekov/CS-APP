#csapp #csapplab 

# 入手准备
在这个实验中,我将动手写一个Web代理服务器.对于身处中国大陆并且学习过计算机网络的我来说,代理服务器的原理并不复杂.
本实验分为三个部分:
1. 实现顺序代理服务器
2. 处理并发请求
3. 缓存Web页面

./driver.sh文件提供了自动测试的功能,但首先要确保安装了依赖工具:
```shell
sudo apt upgrade 
sudo apt install net-tools
sudo apt install curl
```

tiny文件夹下是CS:APP课程的一个简单Web服务器示例.利用它可以进行简单的调试.
在15214端口启动tiny server:
```shell
./tiny 15214
```
然后使用`telnet`工具发送请求:`telnet localhost 15214`
`GET /home.html HTTP/1.0`
能得到如下反响:
![[attachments/Pasted image 20250213164706.png|600]]
Writeup提示我,不应让服务器轻易崩溃,所以我注释掉csapp.h文件的`unix_error`函数声明,在proxy.c文件中重载为stactic类型,这个奇思妙想解决了这个问题.
Writeup还提到了:your proxy must ignore SIGPIPE signals and should deal gracefully with write operations that return EPIPE errors.
不是很懂,暂时不管.


# Part I:实现顺序Web代理服务器
HTTP请求有多种,本实验只要求处理GET HTTP/1.0请求.
Writeup提到了,在浏览器地址栏中输入URL如 http://www.cmu.edu/hub/index.html
会向代理服务器发送如下请求行:
`GET  http://www.cmu.edu/hub/index.html HTTP/1.1`
我需要转换成
```
GET  /hub/index.html HTTP/1.0
Host: www.cmu.edu
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3
Connection: close
Proxy-Connection: close
```
其他请求头不变.
我第一次使用csapp.c封装的I/O函数,由于不熟悉减慢了一些速度.好在不难理解.
对原始URI 的分割和讨论挺烦的,没用上正则表达式.

记录一下主函数,我借鉴了tiny服务器的设计,可以看到非常简洁:
```C
int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    if(argc!=2){
        printf("Usage: %s <port>\n",argv[0]);
        return 0;
    }
    listenfd = Open_listenfd(argv[1]);
    while(1){
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    proxy(connfd);
    Close(connfd);
    }
    return 0;
}
```
`proxy`函数也具有较好可读性:
```C
void proxy(int connfd){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char serv_req[MAXLINE];
    rio_t rio;
  
    Rio_readinitb(&rio,connfd);
    if(!Rio_readlineb(&rio,buf,MAXLINE)) return;
    fprintf(stdout,"%s",buf);
    sscanf(buf,"%s %s %s",method,uri,version);
    if(strcasecmp(method,"GET")){
        unix_error("Proxy server does not implement this method");
        return ;
    }
    URI *uri_data = (URI *)malloc(sizeof(URI));
    parse_uri(uri_data,uri);
    int serverfd = Open_clientfd(uri_data->host,uri_data->port);
    sprintf(serv_req,"GET %s HTTP/1.0\r\nHost: %s\r\n",uri_data->path,version,uri_data->host);;
    printf("%s\n",serv_req);
    Rio_writen(serverfd,serv_req,strlen(serv_req));
    readDeal_hdrs(serverfd,&rio);
    writeback_response(serverfd,connfd);
    Close(serverfd);
}
```

## 测试结果
```shell
$ ./driver
*** Basic ***
Starting tiny on 4663
Starting proxy on 27345
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy
   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40
```
通过!

# Part II:处理并发请求
我已经体验过基于进程的并发了,这次,我决定实现线程池技术实现并发.
回顾[[同步#生产者-消费者问题]],我们需要维护一个线程池.
我创建了sbuf.h和sbuf.c文件,并修改Makefile.
没学过CMake,但是照猫画虎:
```CMake
sbuf.o: sbuf.c sbuf.h
    $(CC) $(CFLAGS) -c sbuf.c
  
proxy: proxy.o csapp.o sbuf.o
    $(CC) $(CFLAGS) proxy.o csapp.o sbuf.o -o proxy $(LDFLAGS)
```
然后,查看CS:APP课本(好像是690多页)的代码,稍作修改,非常轻松就实现了线程池并发.
## 留一份完整代码:
```C
#include <stdio.h>  
#include "csapp.h"  
#include "sbuf.h"  
/* Recommended max cache and object sizes */  
#define MAX_CACHE_SIZE 1049000  
#define MAX_OBJECT_SIZE 102400  
  
typedef struct {  
    char host[MAXLINE];  
    char port[MAXLINE];  
    char path[MAXLINE];  
}URI;  
  
/* You won't lose style points for including this long line in your code */  
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";  
  
  
/* Reload unix_error function to avoid exit */  
static void unix_error(char *msg);  
  
/* share buf */  
sbuf_t sbuf;  
#define SBUFSIZE 20    // sbuf/线程池的大小  
#define NTHREADS 4  // 4个线程  
  
  
/* the proxy server */  
int parse_uri(URI *rebuild_uri,char raw_uri[MAXLINE]){  
    char *hostptr = strstr(raw_uri,"://");  
    char *pathptr = NULL;  
    char *portptr = NULL;  
    if(!hostptr){// 不以 http:// 形式请求  
        rebuild_uri->host[0] = 0;  
        strcpy(rebuild_uri->port,"80");  
        if(pathptr = strstr(raw_uri,"/"))  
            strcpy(rebuild_uri->path,pathptr);  
        return 1;  
    }  
    else{  
        portptr = strstr(hostptr+3,":");  
        if(portptr){  
            int port;  
            sscanf(portptr+1,"%d%s",&port,rebuild_uri->path);  
            sprintf(rebuild_uri->port,"%d",port);  
            *portptr = 0;  
        }  
        else{  
            pathptr = strstr(hostptr+3,"/");  
            strcpy(rebuild_uri->path,pathptr);  
            *pathptr = 0;  
                    }  
        strcpy(rebuild_uri->host,hostptr+3);  
    }  
    return 2;  
}  
void readDeal_hdrs(int sendfd,rio_t *rp){ // 发送请求头  
    char buf[MAXLINE];  
    sprintf(buf,"%sConnection: close\r\nProxy-Connection: close\r\n",user_agent_hdr);  
    Rio_writen(sendfd,buf,strlen(buf));  
    /* 保留其他头部信息 */  
    for(Rio_readlineb(rp,buf,MAXLINE);strcmp(buf,"\r\n");Rio_readlineb(rp,buf,MAXLINE)){  
        if(!strncmp(buf,"Host:",5) || !strncmp(buf,"Connection:",11) ||  
            !strncmp(buf,"User-Agent:",11) || !strncmp(buf,"Proxy-Connection:",17))  
            continue;  
        else  
            Rio_writen(sendfd,buf,strlen(buf));  
        printf("%s\n",buf);  
    }  
    Rio_writen(sendfd,buf,strlen(buf));  
  
}  
void writeback_response(int servfd,int clientfd){  
    rio_t rio;  
    int n;  
    char buf[MAXLINE];  
    Rio_readinitb(&rio,servfd);  
    while(n=Rio_readlineb(&rio,buf,MAXLINE))  
        Rio_writen(clientfd,buf,n);  
    }  
void proxy(int connfd){  
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];  
    char serv_req[MAXLINE];  
    rio_t rio;  
  
    Rio_readinitb(&rio,connfd);  
    if(!Rio_readlineb(&rio,buf,MAXLINE)) return;  
    fprintf(stdout,"%s",buf);  
    sscanf(buf,"%s %s %s",method,uri,version);  
    if(strcasecmp(method,"GET")){  
        unix_error("Proxy server does not implement this method");  
        return ;  
    }  
    URI *uri_data = (URI *)malloc(sizeof(URI));  
    parse_uri(uri_data,uri);  
    int serverfd = Open_clientfd(uri_data->host,uri_data->port);  
    sprintf(serv_req,"GET %s HTTP/1.0\r\nHost: %s\r\n",uri_data->path,uri_data->host);;  
    printf("%s\n",serv_req);  
    Rio_writen(serverfd,serv_req,strlen(serv_req));  
    readDeal_hdrs(serverfd,&rio);  
    writeback_response(serverfd,connfd);  
    Close(serverfd);  
}  
/* thread */  
void *thread(void *vargp){  
    Pthread_detach(pthread_self());  
    int connfd;  
    while(1){  
    connfd = sbuf_remove(&sbuf);  
    proxy(connfd);  
    Close(connfd);  
    }  
}  
  
int main(int argc, char **argv)  
{  
    int listenfd, connfd;  
    char hostname[MAXLINE], port[MAXLINE];  
    socklen_t clientlen;  
    struct sockaddr_storage clientaddr;  
    pthread_t tid;  
        if(argc!=2){  
        printf("Usage: %s <port>\n",argv[0]);  
        return 0;  
    }  
    listenfd = Open_listenfd(argv[1]);  
    sbuf_init(&sbuf,SBUFSIZE);  
  
    for(int i=0;i<NTHREADS;i++){ // 创建几个并发线程  
        Pthread_create(&tid,NULL,thread,NULL);  
    }  
    while(1){  
    clientlen = sizeof(clientaddr);  
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);  
    printf("Accepted connection from (%s, %s)\n", hostname, port);  
    sbuf_insert(&sbuf, connfd);  
    }  
    return 0;  
}  

static void unix_error(char *msg){  
    fprintf(stderr, "%s\n",msg);  
}
```

## 测试结果
初次测试,报错:
```shell
*** Concurrency ***
Starting tiny on port 30981
Starting proxy on port 15708
Starting the blocking NOP server on port 19519
Timeout waiting for the server to grab the port reserved for it
Terminated
```
> 每次出错,后台总会有遗留的僵尸进程,要一个一个kill掉...😅

查看./driver.sh脚本,发现第298~302行附近:
```sh
# Run a special blocking nop-server that never responds to requests
nop_port=$(free_port)
echo "Starting the blocking NOP server on port ${nop_port}"
./nop-server.py ${nop_port} &> /dev/null &
nop_pid=$!
```
Python脚本哪里能直接当可执行文件?这是Python2时代是这样的吗?我记得之前哪个实验也是这个问题.
改为
```sh
python3 ./nop-server.py ${nop_port} &> /dev/null &
```
果然成功了!
```shell
$ ./driver.sh
*** Basic ***
Starting tiny on 24188
Starting proxy on 13850
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy
   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40

*** Concurrency ***
Starting tiny on port 10579
Starting proxy on port 6922
Starting the blocking NOP server on port 3642
Trying to fetch a file from the blocking nop-server
Fetching ./tiny/home.html into ./.noproxy directly from Tiny
Fetching ./tiny/home.html into ./.proxy using the proxy
Checking whether the proxy fetch succeeded
Success: Was able to fetch tiny/home.html from the proxy.
Killing tiny, proxy, and nop-server
concurrencyScore: 15/15
```


# Part III:Web缓存
给最近使用的页面增加一层缓存.HTTP实际上定义了复杂的缓存模型.但实验将采用简化的方法.我也知道,如果页面缓存超过一定时间,页面可能发生了变化,显然应该把它移除缓存,但本实验没有考虑这一点,仅仅是简单的LRU缓存替换策略.
给缓存采取限制:
- 最大缓存大小:1MB
- 最大缓存页面大小:100KB

说起Cache,应该是让我设计固定大小的缓存块,不是像Malloclab那样的碎片.也就是说,固定有10个页面缓存.
回顾[[Cachelab]]的代码,我设计出相似的数据结构.

如果缓存命中,将命中的页面的timestamp设置为0,其他所有页面加1;如果缓存不命中,获取该页面,寻找空闲页面填入(如果没有空闲页面,寻找timestamp最大的页面替换),将其他所有页面加1.

我通过`man 3 sprintf`了解到,目前`sprintf`已经是线程安全函数.

## 测试结果
```shell
$ make && ./driver.sh
gcc -g -Wall proxy.o csapp.o sbuf.o -o proxy -lpthread
*** Basic ***
Starting tiny on 11304
Starting proxy on 25852
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40

*** Concurrency ***
Starting tiny on port 24301
Starting proxy on port 7349
Starting the blocking NOP server on port 27021
Trying to fetch a file from the blocking nop-server
Fetching ./tiny/home.html into ./.noproxy directly from Tiny
Fetching ./tiny/home.html into ./.proxy using the proxy
Checking whether the proxy fetch succeeded
Success: Was able to fetch tiny/home.html from the proxy.
Killing tiny, proxy, and nop-server
concurrencyScore: 15/15

*** Cache ***
Starting tiny on port 3492
Starting proxy on port 12373
Fetching ./tiny/tiny.c into ./.proxy using the proxy
Fetching ./tiny/home.html into ./.proxy using the proxy
Fetching ./tiny/csapp.c into ./.proxy using the proxy
Killing tiny
Fetching a cached copy of ./tiny/home.html into ./.noproxy
Success: Was able to fetch tiny/home.html from the cache.
Killing proxy
cacheScore: 15/15

totalScore: 70/70
```
通过所有测试!

# 在浏览器中使用
只支持HTTP/1.0的GET请求对于现代网站来说太局限了.不过,我可以访问localhost的tiny服务器:
![[attachments/Pasted image 20250214001902.png|900]]

![[attachments/Pasted image 20250214002220.png|900]]
通过代理服务器,可以翻越GFW,自由之道,蕴于其中.亲自实现这个代理服务器,我还是深有感触的.
本实验比较简单而且我也有计算机网络和网络编程基础,所以我只用了1天时间就完成了.

至此, CMU CS15-213的所有实验已经顺利完成!
完善收尾工作后,我会将所有实验代码,Markdown笔记上传到Github,供有缘人学习参考!