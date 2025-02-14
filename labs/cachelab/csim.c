#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int hit_count=0,miss_count=0,eviction_count=0;
unsigned char verbose = 0;
int s,E,b;
char t[30];

void deal_commandline(int argc, char **argv){
	char opt;
	while(-1!=(opt = getopt(argc,argv,"hvs:E:b:t:"))){
		switch(opt){
		case 'h': // prints usage info
			printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
			printf("\nThis is my cache simulator, which simulates the behavior of a cache with arbitrary size and associativity on a valgrind trace file. \
It uses the	LRU (least-recently used) replacement policy when choosing which cache line to evict.\n");
			printf("\noptions:\n");
			printf("\t-h: Optional help flag that prints usage info \n\
\t-v: Optional verbose flag that displays trace info \n\
\t-s <s>: Number of set index bits (S = 2^s is the number of sets) \n\
\t-E <E>: Associativity (number of lines per set) \n\
\t-b <b>: Number of block bits (B = 2^b is the block size) \n\
\t-t <tracefile>: Name of the valgrind trace to replay\n");
			exit(0);
			break;
		case 'v':  // verbose flag to display trace info
			verbose = 1;
			break;
		case 's':
			s = atoi(optarg);
			break;
		case 'E':
			E = atoi(optarg);
			break;
		case 'b':
			b = atoi(optarg);
			break;
		case 't':
			strcpy(t,optarg);
		}
	}
	
}

/* 定义缓存结构 */ 

typedef struct {
	unsigned char v; // 有效位
	unsigned tag;  // 标志位
	unsigned timestamp;  // 使用情况
	// 本来还需要block的,但没要求抽取字节偏移,不用设置block块
}line;// 行

typedef struct {
	int S,E,B;
	line **block;
}cache;// 缓存是行的二维数组

cache *Cache;
int init_cache(){
	Cache = (cache *)malloc(sizeof(cache));
	if(!Cache)return 0;
	int S=1<<s,B=1<<b;
	Cache->E = E;
	Cache->S = S;
	Cache->B = B;
	Cache->block = (line **)malloc(sizeof(line*)*S);
	if(!Cache->block)return 0;
	for(int i=0;i<S;i++){
		Cache->block[i] = (line *)malloc(sizeof(line)*E);
		if(!Cache->block[i])return 0;
		for(int j=0;j<E;j++){
			Cache->block[i][j].v=0;  // 初始化,invalid.
			Cache->block[i][j].timestamp=0;
		}
	}
	return 1;
}



int find_line(unsigned long tag,unsigned long set){
	for(int i=0;i<E;i++){
		if(Cache->block[set][i].v && Cache->block[set][i].tag==tag)
			return i;
	}
	// 没有返回,说明找不到,返回-1.
	return -1;
}
int is_full(unsigned long set){
	// 判断该组是否有空行,是否已满
	for(int i=0;i<E;i++){
		if(Cache->block[set][i].v==0)return i;//有空行,返回i
	}
	return -1;// 没有空行,返回-1;
}

int find_LRU(unsigned long set){
	// 找到组内timestamp最大的行数
	int result=0;
	unsigned timestamp=Cache->block[set][result].timestamp;
	for(int i=1;i<E;i++){
		if(Cache->block[set][i].timestamp>timestamp)
		{result=i;timestamp=Cache->block[set][i].timestamp;}
	}
	return result;
}

void update_timestamp(int set,int index){
	for(int i=0;i<E;i++){
		if(i==index)
			Cache->block[set][i].timestamp=0;  // 一旦使用,设置为0;
		
		else
			Cache->block[set][i].timestamp++;  // 不使用的加1;
	}
}
// load 和 store并没有必然的先后关系,所以操作是一样的.
void LoadorStore(unsigned long tag, unsigned long set){
	// 加载操作:
	// 如果缓存命中,hit_count++
	int index;
	if((index = find_line(tag,set))!=-1){
		hit_count++;
		if(verbose)printf("hit ");
	}
	
	else{
		miss_count++;
		if(verbose)printf("miss ");
		if((index = is_full(set))==-1){
			eviction_count++;

			// LRU 替换
			index = find_LRU(set);
			Cache->block[set][index].tag=tag;
			if(verbose)printf("eviction");			
		}
		else{

			Cache->block[set][index].v=1;
			Cache->block[set][index].tag=tag;
		}
		
	}
	update_timestamp(set,index);
	// 如果缓存不命中,要看有没有空行.如果有空行,miss_count++;如果没有空行,eviction_count++
	// 干脆模块化设计一个查找函数
}



void modify(unsigned long tag, unsigned long set){
	// a data load followed by a data store
	LoadorStore(tag,set);
	LoadorStore(tag,set);
}

void deal_traceflie(){
	FILE *f = fopen(t,"r");
	if(!f)exit(EXIT_FAILURE);
	char operation;
	unsigned long addr,tag,set;
	size_t size;
	
	while(fscanf(f," %c %lx,%lu",&operation,&addr,&size)==3){
		tag = addr>>(s+b);
		set = (addr>>b)%(1<<s);
		// writeup 提示我不会跨块,可忽略size字段.
		// 只需定位set和(v==1)和tag相同
		if(verbose&&(operation!='I'))printf("%c %lx,%lu ",operation,addr,size);
		switch(operation){
		case 'L':case 'S':
			LoadorStore(tag,set);break;
			case 'M':modify(tag,set);break;
		}
		if(verbose&&(operation!='I'))printf("\n");
	}
	
	fclose(f);
	
}


int main(int argc, char **argv)
{
	deal_commandline(argc, argv);
	// 现在,命令行参数都已经保存到全局变量
	if(!init_cache())exit(EXIT_FAILURE);
	// 经过初始化,我得到了缓存指针Cache
	deal_traceflie();
    printSummary(hit_count, miss_count, eviction_count);
	free(Cache);
    return 0;
}
