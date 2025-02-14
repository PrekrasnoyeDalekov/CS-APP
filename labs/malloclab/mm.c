/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8       

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* my macros */
typedef  void *block_ptr;
typedef  void *payload_ptr;
#define WORD 4
#define DWORD 8
#define void_plus(ptr,add) ((void *)((char *)(ptr) + (add)))
#define void_minus(ptr,sub) ((void *)((char *)(ptr) - (sub)))
#define bp2pp(bp) (void_plus((bp),WORD))
#define pp2bp(pp) (void_minus((pp),WORD))
#define get_next(bp) (void_plus((bp),WORD))
#define get_prev(bp) (void_plus((bp),2*WORD))
#define access(ptr) (*(unsigned int *)(ptr))
#define writeword(ptr,val) (access(ptr) = (unsigned int)(val))
#define writedword(ptr,val)	(*(size_t *)(ptr) = (size_t)(val))
#define next_block(bp) (void_plus((bp),get_size(bp)))
#define prev_block(bp) (void_minus((bp),get_size(void_minus((bp),WORD))))
#define is_next_free(pp) ((access(next_block(pp2bp(pp))) & 0x7) == 0 && access(next_block(pp2bp(pp))))
#define is_prev_free(pp) ((access(prev_block(pp2bp(pp))) & 0x7) == 0 && access(prev_block(pp2bp(pp))))
#define is_free(bp) ((access(bp) & 7) == 0)
#define get_size(bp) (access(bp)>>3<<3)

/* static variables */
static void *seglist[21];
/*	
	
	2-3    我发现一个块最少需要2双字,使得free后能容纳头部脚部,next/prev指针!
	4-7
	8-15
  16-31
  32-63
  64-127
  128-255
  256-511
	...
  20MB<32MB=2^25B=2^22字长
*/
/* check functions */
extern int verbose;

static void print_block(block_ptr bp,const char *func){
	if(!verbose) return;
	fprintf(stderr,"================%5s===================\n",func);
	fprintf(stderr,"===============block=================\n");
	if(!bp){
		fprintf(stderr,"NULL block");return;
	}
	fprintf(stderr,"block address:%p\tsize|allocated: %d\tnext: %p\tprev: %p\n",bp,access(bp),access(get_next(bp)),access(get_prev(bp)));
	return ;
}
static void print_freelist(int index){
	for(block_ptr p=seglist[index];p;p=(void *)access(get_next(p))){
		print_block(p,__func__);
	}
}
static int check_1(void){
	// Is every block in the free list marked as free?
	for(int i=0;i<21;i++){
		for(block_ptr bp = seglist[i];bp;bp=access(get_next(bp)))
			if(!is_free(bp)){
				fprintf(stderr,"Block %p in the free list %d is not marked as free!\n",bp,i);
				print_block(bp,__func__);
//				exit(1);
				print_freelist(i);
				return 0;
			}
	}
	return 1;
}
static int check_2(void){
	// Are there any contiguous free blocks that somehow escaped coalescing?
	void *heap_start = void_plus(mem_heap_lo(),WORD);
	int flag=0;
	for(block_ptr bp = heap_start;access(bp);bp=next_block(bp)){
		if(flag){
			if(is_free(bp)){
				fprintf(stderr,"Contiguous free block at %p to %p.\n",prev_block(bp),bp);
				fprintf(stderr,"prev block:\n");
				print_block(prev_block(bp),__func__);
				fprintf(stderr,"next block:\n");
				print_block(bp,__func__);
				return 0;
			}
			flag=0;
		}
		else if(is_free(bp))
			flag=1;
	}
	return 1;
	
}
static int check_3(void){
	//  Is every free block actually in the free list?
//	void *heap_start = void_plus(mem_heap_lo(),WORD);
//	for(block_ptr bp = heap_start;access(bp);bp=next_block(bp)){
//		if(is_free(bp))
	
//	}
	return 1;
}
static int check_4(void){
	// Do any allocated blocks overlap?
	return 1;
}
static void mm_check(void){
	if(!verbose) return;
	static int valid_flag = 1;
	if(!valid_flag)exit(-1);
	valid_flag &= check_1();
	valid_flag &= check_2();
	valid_flag &= check_3();
	valid_flag &= check_4();
}

/* my static functions */
static int get_seglist_index(int dwords){
	if(dwords<4)return 0;
	int res = -2;
	while(dwords){
		res++;
		dwords = dwords>>1;
	}
	return res;
}
static block_ptr link_search_firstfit(block_ptr bp, int block_size){
	while(bp){
		int curr_size = access(bp);
		if(curr_size>=block_size)
			return bp;
		else
			bp = (block_ptr)access(get_next(bp));
	}
	return (block_ptr) NULL;
}
static void insert_node(block_ptr bp, int index){
	writedword(get_next(bp), seglist[index]);
	writedword(get_prev(bp), NULL);
	if(seglist[index])
		writedword(get_prev(seglist[index]), bp);
	seglist[index] = bp;
}
static void delet_node(block_ptr bp){
	block_ptr prev_ptr = access(get_prev(bp)),next_ptr = access(get_next(bp));
	if(!prev_ptr && !next_ptr){
		// 删除的是唯一一个节点
		seglist[get_seglist_index(access(bp)>>3)] = (void *)NULL;  // seglist[index] = next_ptr;
		return;
	}
	if(!prev_ptr){
		seglist[get_seglist_index(access(bp)>>3)] = next_ptr;
		writeword(get_prev(next_ptr),NULL);
		return;
	}
	else if(!next_ptr){
		// 删除的是最后一个节点
		writeword(get_next(prev_ptr),NULL);
		return;
	}

	writedword(get_next(prev_ptr), next_ptr);
	writedword(get_prev(next_ptr), prev_ptr);
	return;
}
static void split(block_ptr bp, int dwords){
	//  bp具有old_size,将其分割成
	//  -------------------------
	//  | new_size | split_size |
	//  -------------------------
	int old_size = get_size(bp);
	int new_size = dwords*8;
	int split_size = old_size - new_size;

	if(split_size>=16){
		// 足以产生新的空闲块
		// 新的空闲块头插法加到seglist中
		block_ptr new_bp = void_plus(bp,new_size);
		writeword(new_bp,split_size);  // 头部
		writeword(void_plus(new_bp,split_size-WORD),split_size);  // 脚部
		int index = get_seglist_index(split_size/8);
		insert_node(new_bp,index);
		// 更新分配的块,设置头部和脚部,设置标志位
		writeword(bp,new_size|1);writeword(void_minus(new_bp,WORD),new_size|1);
	}
	else{
		writeword(bp,old_size|1);writeword(void_minus(next_block(bp),WORD),old_size|1);
	}// 设置标志位

}
static block_ptr split_post(block_ptr bp, int dwords){
	//  原来的块的数据在前半部分,要先复制到后部分:
	//  -------------------------
	//  | ****** |              |
	//  -------------------------
	//             |
	//             V
	//  -------------------------
	//  |              | ****** |
	//  -------------------------
	//  bp具有old_size,将其分割成
	//  -------------------------
	//  | split_size | new_size |
	//  -------------------------
	int old_size = get_size(bp);
	int new_size = dwords*8;
	int split_size = old_size - new_size;
	memmove(bp2pp(void_plus(bp,split_size)),bp2pp(bp),new_size-DWORD);	
	if(split_size>=16){
		// 足以产生新的空闲块
		// 新的空闲块头插法加到seglist中
		writeword(bp,split_size);  // 头部
		writeword(void_plus(bp,split_size-WORD),split_size);  // 脚部
		int index = get_seglist_index(split_size>>3);
		insert_node(bp,index);
		// 更新分配的块,设置头部和脚部,设置标志位
		block_ptr new_bp = void_plus(bp,split_size);
		writeword(new_bp,new_size|1);  // 头部
		writeword(void_plus(new_bp,new_size-WORD),new_size|1);// 脚部
		return new_bp;
	}
	else{
		writeword(bp,old_size|1);writeword(void_minus(next_block(bp),WORD),old_size|1);
		return bp;
	}// 设置标志位
	
}
static payload_ptr extend_heap(int size){
	payload_ptr new_space = mem_sbrk(size);
	if(new_space == (void *)-1)
		return (void *)-1;
	writeword(void_minus(new_space,WORD), size|1);
	writeword(void_minus(mem_heap_hi(),3),NULL);
	writeword(void_minus(mem_heap_hi(),7), size|1);
	// ---------------------
	// |foot|ending padding|
	// ---------------------
	return new_space;
}
static void case1(payload_ptr ptr){
	block_ptr bp = pp2bp(ptr);
	int size = get_size(bp);
	// 修改标志位
	writeword(bp,size);
	writeword(void_plus(bp,size-WORD),size);
	
	insert_node(bp,get_seglist_index(size>>3));

}
static void case2(payload_ptr ptr){
	block_ptr bp = pp2bp(ptr);
	block_ptr next_bp = next_block(bp);
	int size1 = get_size(bp);
	int size2 = get_size(next_bp);
	int size = size1 + size2;
	delet_node(next_bp);
	
	// 修改标志位
	writeword(bp,size);
	writeword(void_plus(bp,size-WORD),size);
	
	insert_node(bp,get_seglist_index(size>>3));
}
static void case3(payload_ptr ptr){
	block_ptr bp = pp2bp(ptr);
	block_ptr prev_bp = prev_block(bp);
	int size1 = get_size(bp);
	int size2 = get_size(prev_bp);
	int size = size1 + size2;
	delet_node(prev_bp);
	
	// 修改标志位
	writeword(prev_bp,size);
	writeword(void_plus(bp,size1-WORD),size);
	
	insert_node(prev_bp,get_seglist_index(size>>3));

}
static void case4(payload_ptr ptr){
	block_ptr bp = pp2bp(ptr);
	block_ptr next_bp = next_block(bp);
	block_ptr prev_bp = prev_block(bp);
	int size1 = get_size(bp);
	int size2 = get_size(next_bp);
	int size3 = get_size(prev_bp);
	int size = size1 + size2 + size3;
	delet_node(next_bp);
	delet_node(prev_bp);
	
	// 修改标志位
	writeword(prev_bp,size);
	writeword(void_plus(next_bp,size2-WORD),size);
	
	insert_node(prev_bp,get_seglist_index(size>>3));
}



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 初始化seglist
	for(int i=0;i<21;i++){
		seglist[i] = (void *)(NULL);
	}
	// 初始化堆:一个对齐字,一个结尾块
	//  ---------------------------
	//  | unused padding | ending |
	//  ---------------------------
	// 一共是8字节
	void *heap_head = mem_sbrk(DWORD);
	if(heap_head == (void *)-1)
		return -1;
	writeword(heap_head,NULL);
	writeword(void_plus(heap_head,WORD),NULL);
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	if(!size) return (payload_ptr)NULL;
    int newsize = ALIGN(size + SIZE_T_SIZE);  // 带头部,脚部的新块大小(字节)
	int index=0;
	int dwords = newsize>>3;
	switch(dwords){
	case 1:case 2:case 3: index = 0;break;
	default: index = get_seglist_index(dwords);break;
	}
	block_ptr new_ptr=NULL;
	for(int i=index;i<21;i++){
		if(new_ptr = link_search_firstfit(seglist[i],newsize)){
			// 将新分配的块抽离原来的空闲列表
			delet_node(new_ptr);
			split(new_ptr,dwords);
			return bp2pp(new_ptr);
		}
	}
	// seglist没有合适的空闲块,分配新的
	switch(dwords){
	case 1:case 2:case 3: 
		return extend_heap(24);
	default:
		return extend_heap(newsize);
	}
	
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	// 下一块是否释放
	int next_free = is_next_free(ptr);
	// 前一块是否释放
	int prev_free = is_prev_free(ptr);
	
	int case_ = (prev_free<<1) | next_free;
	switch(case_){
	case 0:case1(ptr);break;
	case 1:case2(ptr);break;
	case 2:case3(ptr);break;
	case 3:case4(ptr);break;
	}
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{	
	if(!ptr)
		return mm_malloc(size);
	if(!size){
		mm_free(ptr);
		return NULL;
	}
	block_ptr old_bp = pp2bp(ptr);
	size_t old_block_size = get_size(old_bp);
	size_t new_block_size = ALIGN(size + DWORD);
	if(new_block_size<=old_block_size){
		// 对原块进行操作,直接截断
		writeword(old_bp,new_block_size);
		writeword(void_plus(old_bp,new_block_size-WORD),new_block_size);
		split(old_bp,new_block_size>>3);
		return ptr;
	}
	else{
		
		// 如果块很大,尽量分配在靠近堆顶的地方,以便随时拓展堆.
		// 旧方法的split函数一直分割前部,后面的碎片就被小块占用
		// 如果新块大于512字节,采用split_post
		
		
		// 新块比旧块大
		payload_ptr new_ptr=NULL;
		block_ptr tmp1=NULL,tmp2=NULL;
		size_t all_size;
		// 如果ptr是堆中最后一块,
		// 前一块空闲则合并成新的块,再判断大小是否足够,
		// 大小足够:修改头部脚部,split;大小不足:拓展堆空间,修改头部脚部
		// 复制.
		// 前一块不空闲,拓展堆空间,修改头部脚部.
		if(!access(next_block(old_bp))){
			if(is_prev_free(ptr)){ // 前一块空闲
				tmp1 = prev_block(old_bp);
				size_t con_size = old_block_size+get_size(tmp1);
				int extend_size = new_block_size - con_size;
				delet_node(tmp1);
				memmove(bp2pp(tmp1),ptr,old_block_size-DWORD);
				if(extend_size > 0){
					// 大小不足
					writeword(void_plus(pp2bp(extend_heap(extend_size)),extend_size-WORD),new_block_size|1);
					writeword(tmp1,new_block_size|1);
					return bp2pp(tmp1);
				}
				else{// 大小充足
					writeword(tmp1,con_size);
					writeword(void_plus(old_bp,old_block_size-WORD),con_size);
//					split(tmp1,new_block_size>>3);
//					return bp2pp(tmp1);
					return bp2pp(split_post(tmp1,new_block_size>>3));
				}
			}
			else{ // 前一块不空闲
				size_t extend_size = new_block_size - old_block_size;
				writeword(void_plus(pp2bp(extend_heap(extend_size)),extend_size-WORD),new_block_size|1);
				writeword(old_bp,new_block_size|1);
				return ptr;
			}
		}
		
		// 如果ptr不是堆中的最后一块
		// 寻找前后能否合并
		// 下一块空闲且加上后大小足够
		int next_enough=-1;
		int next_free = is_next_free(ptr) && ((next_enough = old_block_size+get_size(next_block(old_bp)) - new_block_size) >= 0);
		// 前一块空闲且加上后大小足够
		int prev_enough=-1;
		int prev_free = is_prev_free(ptr) && ((prev_enough = old_block_size+get_size(prev_block(old_bp)) - new_block_size) >= 0);
		
		
		int realloc_case = (prev_free<<1) | next_free;
		switch(realloc_case){
		case 0:
			if(is_next_free(ptr) && !next_block(next_block(old_bp))){
				// 下一块空闲且是最后一块,但大小不足,可以合并后拓展堆
				delet_node(next_block(old_bp));
				size_t next_block_size = get_size(next_block(old_bp));
				size_t extend_size = new_block_size - old_block_size - next_block_size;
				writeword(void_plus(pp2bp(extend_heap(extend_size)),extend_size-WORD),new_block_size|1);
				writeword(old_bp,new_block_size|1);
				return ptr;
			}//  emm...加了这种情况没有提分...
			
			new_ptr = mm_malloc(size);
			memcpy(new_ptr,ptr,old_block_size-DWORD);
			mm_free(ptr);
			return new_ptr;
			break;
		case 1:
			// 与case2函数相似
			delet_node(next_block(old_bp));
			all_size = next_enough + new_block_size;
			// 修改标志位
			writeword(old_bp,all_size);
			writeword(void_plus(old_bp,all_size-WORD),all_size);
			
//			split(old_bp,new_block_size>>3);
//			return ptr;
			return bp2pp(split_post(old_bp,new_block_size>>3));
			break;
		case 2:
			// 与case3函数相似
			tmp1 = prev_block(old_bp);
			delet_node(tmp1);
			all_size = prev_enough + new_block_size;
			memmove(bp2pp(tmp1),ptr,old_block_size-DWORD);
			// 修改标志位
			writeword(tmp1,all_size);
			writeword(void_plus(tmp1,all_size-WORD),all_size);
			
//			split(tmp1,new_block_size>>3);
//			return bp2pp(tmp1);
			return bp2pp(split_post(tmp1,new_block_size>>3));
			break;
		case 3:
			tmp1 = prev_block(old_bp);
			tmp2 = next_block(old_bp);
			delet_node(tmp1);
			delet_node(tmp2);
			all_size = old_block_size + get_size(tmp1) + get_size(tmp2);
			memmove(bp2pp(tmp1),ptr,old_block_size-DWORD);
			// 修改标志位
			writeword(tmp1,all_size);
			writeword(void_plus(tmp1,all_size-WORD),all_size);
			
//			split(tmp1,new_block_size>>3);
//			return bp2pp(tmp1);
			return bp2pp(split_post(tmp1,new_block_size>>3));
			break;
		}
	}
	
}
