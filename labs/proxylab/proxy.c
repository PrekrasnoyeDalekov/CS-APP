#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define PAGES 10
typedef struct {
	char host[MAXLINE];
	char port[MAXLINE];
	char path[MAXLINE];
}URI;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


/* Reload unix_error function to avoid exit */
static void unix_error(char *msg);

/* share buf */
sbuf_t sbuf;
#define SBUFSIZE 20	// sbuf/线程池的大小
#define NTHREADS 4  // 4个线程


/* cache relative functions */
typedef struct {
	unsigned char available;
	char uri[MAXLINE];
	char content[MAX_OBJECT_SIZE];   // 读优先
	unsigned int timestamp;          // 写优先
	sem_t mutex;
	sem_t writelock;
	sem_t readlock;
	unsigned int readcnt;
	unsigned int writecnt;
}page;


/* the proxy server */
int parse_uri(URI *rebuild_uri,char raw_uri[MAXLINE]){
	char uri[MAXLINE];
	strcpy(uri,raw_uri);
	char *hostptr = strstr(uri,"://");
	char *pathptr = NULL;
	char *portptr = NULL;
	if(!hostptr){// 不以 http:// 形式请求
		rebuild_uri->host[0] = 0;
		strcpy(rebuild_uri->port,"80");
		if(pathptr = strstr(uri,"/"))
			strcpy(rebuild_uri->path,pathptr);
		return 1;
	}
	else{
		portptr = strstr(hostptr+3,":");
		if(portptr){
			int port;
			sscanf(portptr+1,"%d%s",&port,rebuild_uri->path);
			sprintf(rebuild_uri->port,"%d",port);
			*portptr = 0;
		}
		else{
			pathptr = strstr(hostptr+3,"/");
			strcpy(rebuild_uri->path,pathptr);
			strcpy(rebuild_uri->port,"80");
			*pathptr = 0;
		}
		strcpy(rebuild_uri->host,hostptr+3);
	}
	return 2;
}
void readDeal_hdrs(int sendfd,rio_t *rp){ // 发送请求头
	char buf[MAXLINE];
	sprintf(buf,"%sConnection: close\r\nProxy-Connection: close\r\n",user_agent_hdr);
	Rio_writen(sendfd,buf,strlen(buf));
	/* 保留其他头部信息 */
	for(Rio_readlineb(rp,buf,MAXLINE);strcmp(buf,"\r\n");Rio_readlineb(rp,buf,MAXLINE)){
		if(!strncmp(buf,"Host:",5) || !strncmp(buf,"Connection:",11) ||
			!strncmp(buf,"User-Agent:",11) || !strncmp(buf,"Proxy-Connection:",17))
			continue;
		else
			Rio_writen(sendfd,buf,strlen(buf));
		printf("%s\n",buf);
	}
	Rio_writen(sendfd,buf,strlen(buf));

}
void writeback_response(int servfd,int clientfd,char *buf){
	rio_t rio;
	int n;
	char line[MAXLINE];
	Rio_readinitb(&rio,servfd);
	while(n=Rio_readlineb(&rio,line,MAXLINE)){
		Rio_writen(clientfd,line,n);
		strcat(buf,line);
	}
	
}
page cache[PAGES];
void cache_init(page cache[PAGES]){
	for(int i=0;i<PAGES;i++){
		cache[i].readcnt = 0;
		cache[i].writecnt = 0;
		cache[i].available = 0;
		cache[i].timestamp = 0;
		Sem_init(&cache[i].mutex,0,1);
		Sem_init(&cache[i].writelock,0,1);
		Sem_init(&cache[i].readlock,0,1);
	}
}
int is_hit(char *uri){
	// printf("%s",uri);
	for(int i=0;i<PAGES;i++){
		// printf("uri:%s\ncontent:%s\n",cache[i].uri,cache[i].content);
		P(&cache[i].mutex);
		cache[i].readcnt++;
		if(cache[i].readcnt == 1)
			P(&cache[i].writelock);
		V(&cache[i].mutex);

		/* 读优先的读操作 区域开始 */
		if(!strcmp(cache[i].uri,uri)){
			// uri相同,缓存命中
			return i;
		}
		/* 读优先 区域结束 */

		P(&cache[i].mutex);
		cache[i].readcnt--;
		if(cache[i].readcnt == 0)
			V(&cache[i].writelock);
		V(&cache[i].mutex);
	}
	return -1;
}
void modify_avail_LUR(char *content,char *uri){
	unsigned int max_time = -1;
	int index = -1;
	for(int i=0;i<PAGES;i++){
		if(cache[i].available == 0){
		/* 写优先的读操作 区域开始 */
		P(&cache[i].readlock);
			cache[i].available = 1;
			strcpy(cache[i].content,content);
			strcpy(cache[i].uri,uri);
		V(&cache[i].readlock);
		/* 写优先 区域结束 */
			return ;
		}

		if(cache[i].timestamp > max_time){
			max_time = cache[i].timestamp;
			index = i;
		}
	}
		/* 写优先的读操作 区域开始 */
		P(&cache[index].readlock);
	strcpy(cache[index].content,content);
	strcpy(cache[index].uri,uri);
		V(&cache[index].readlock);
		/* 写优先 区域结束 */
	return ;
}
void get_page_cache(char *buf,int index){
		P(&cache[index].mutex);
		cache[index].readcnt++;
		if(cache[index].readcnt == 1)
			P(&cache[index].writelock);
		V(&cache[index].mutex);
	/* 读优先的读操作 区域开始*/
	strcpy(buf,cache[index].content);
	/* 读优先 区域结束 */
		P(&cache[index].mutex);
		cache[index].readcnt--;
		if(cache[index].readcnt == 0)
			V(&cache[index].writelock);
		V(&cache[index].mutex);
}
void get_page_online(char *buf,char *uri,rio_t *rp,int connfd){
	char serv_req[MAXLINE];
	URI *uri_data = (URI *)Malloc(sizeof(URI));
	parse_uri(uri_data,uri);
	int serverfd = Open_clientfd(uri_data->host,uri_data->port);
	sprintf(serv_req,"GET %s HTTP/1.0\r\nHost: %s\r\n",uri_data->path,uri_data->host);
	printf("%s\n",serv_req);
	Rio_writen(serverfd,serv_req,strlen(serv_req));
	readDeal_hdrs(serverfd,rp);
	writeback_response(serverfd,connfd,buf);
	Free(uri_data);
	Close(serverfd);
}
void update(int index){
	// 将index以外的cache的timestamp增加1
	for(int i=0;i<PAGES;i++){
			// P(&cache[i].mutex);
			// cache[i].writecnt++;
			// if(cache[i].writecnt == 1)
			// 	P(&cache[i].readlock);
			// V(&cache[i].mutex);
			/* 写优先的写操作 区域开始*/
		if(i!=index){
			cache[i].timestamp++;
		}
		else{
			cache[i].timestamp = 0;
		}
			/* 写优先 区域结束*/
			// P(&cache[i].mutex);
			// cache[i].writecnt--;
			// if(cache[i].writecnt == 0)
			// 	V(&cache[i].writelock);
			// V(&cache[i].mutex);
	}
}
void get_page(char *uri,rio_t *rp,int connfd){
	int index = is_hit(uri);
	char content_buf[MAX_OBJECT_SIZE];
	if(index<0){
		// 缓存不命中
		get_page_online(content_buf,uri,rp,connfd);
		if(strlen(content_buf)<MAX_OBJECT_SIZE){
			modify_avail_LUR(content_buf,uri);
		}
		update(index);
	}
	else{
		get_page_cache(content_buf, index);
		printf("%s\n",content_buf);
		update(index);
		Rio_writen(connfd,content_buf,strlen(content_buf));
	}
}

void proxy(int connfd){
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio,connfd);
	if(!Rio_readlineb(&rio,buf,MAXLINE)) return;
	fprintf(stdout,"%s",buf);
	sscanf(buf,"%s %s %s",method,uri,version);
	if(strcasecmp(method,"GET")){
		unix_error("Proxy server does not implement this method");
		return ;
	}
	get_page(uri,&rio,connfd);
}
/* thread */
void *thread(void *vargp){
	Pthread_detach(pthread_self());
	int connfd;
	while(1){
	connfd = sbuf_remove(&sbuf);
	proxy(connfd);
	Close(connfd);
	}
}



int main(int argc, char **argv)
{
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;
	
	if(argc!=2){
		printf("Usage: %s <port>\n",argv[0]);
		return 0;
	}
	listenfd = Open_listenfd(argv[1]);
	sbuf_init(&sbuf,SBUFSIZE);
	cache_init(cache);
	for(int i=0;i<NTHREADS;i++){ // 创建几个并发线程
		Pthread_create(&tid,NULL,thread,NULL);
	}
	while(1){
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
				port, MAXLINE, 0);
	printf("Accepted connection from (%s, %s)\n", hostname, port);
	sbuf_insert(&sbuf, connfd);
	}

    return 0;
}

static void unix_error(char *msg){
	fprintf(stderr, "%s\n",msg);
}
