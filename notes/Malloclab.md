#csapp #csapplab 

# 准备工作
为方便GDB调试,修改Makefile:
```cmake
CC = gcc -g
CFLAGS = -Wall -O0 -m32
```
值得注意的是,这里的 `-m32`标签强制使用32位编译,也就是说,相当于32位系统,指针大小是4字节!一开始没注意,导致了很多困惑.

下载12个traces文件,修改config.h:
```C
//#define TRACEDIR "/afs/cs/project/ics2/im/labs/malloclab/traces/"  
#define TRACEDIR "./traces/"
```

本次实验可以2个人合作(话说两个人怎么写一份代码啊?设计不同怎么兼容,一起找bug还可行点).
需要我写4个函数,实现堆管理:
- `mm_init`:堆初始化
- `mm_malloc`:自己实现`malloc`函数的功能
- `mm_free`:自己实现`free`函数的功能
- `mm_realloc`:自己实现`realloc`函数的功能
除此之外,还有5分落在了堆一致性检查器上:
- free列表里的块都标记为free
- 没有连续的free块
- free块都在free里
等等.我主要用的第一个功能.找bug的时候自己检查,这确实是个好建议,稍微有点帮助.因为./mdrive给的错误信息实在太少了...


# 代码解释
我使用的策略是隔离空闲块列表 + first fit + 头插法.
在config.h文件中指明了最大堆空间:
```C
#define MAX_HEAP (20*(1<<20))  /* 20 MB */
```
所以根据2的幂可以确定seglist的元素个数.

写代码的过程依旧难以言说,与bug进行各种博弈...翻来覆去能改好几遍.具体见代码的注释,都是写代码过程中所思所想,很仔细了.

mm_realloc函数主体很长,模块化较差,因为情况实在太多了,不是很满意...
有一个很坑的点,当 `memcpy`的内存区域重叠时就不要使用了会有奇怪的bug(填充别的字符),改用功能一样的`memmove`.

realloc改进前:
```shell
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   98%    5694  0.000421 13512
 1       yes   98%    5848  0.000426 13721
 2       yes   97%    6648  0.000456 14576
 3       yes   99%    5380  0.000370 14525
 4       yes  100%   14400  0.000661 21795
 5       yes   92%    4800  0.000455 10561
 6       yes   90%    4800  0.000379 12672
 7       yes   55%   12000  0.000590 20353
 8       yes   51%   24000  0.001004 23892
 9       yes   58%   14401  0.001003 14358
10       yes   45%   14401  0.000662 21754
Total          80%  112372  0.006427 17484

Perf index = 48 (util) + 40 (thru) = 88/100
```
realloc的利用率不理想,我想了一下,结合别人的方法的点拨[^1],我一下子就发现了原因.
原始的split函数总是分割前部,后部空闲.这会导致后部被别的占用,realloc扩大就只能另寻
如果大块的使用后部,小块的使用前部,就可以大大提升利用率!
这种想法当然是对的,要应用得好还需要考虑很多细节.
这里我仅是将realloc里面的split全部改用split_post,没有区别对待大小,利用率已经有了很大提升!

[^1]:[高性能 Malloc Lab —— 不上树 97/100](https://zhuanlan.zhihu.com/p/374478609)

# 结果
采用split_post改进后:
```shell
$ make && ./mdriver -a -v
make: 'mdriver' is up to date.
Using default tracefiles in ./traces/
Measuring performance with gettimeofday().

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   98%    5694  0.000314 18151
 1       yes   98%    5848  0.000405 14429
 2       yes   97%    6648  0.000449 14823
 3       yes   99%    5380  0.000358 15028
 4       yes  100%   14400  0.000595 24218
 5       yes   92%    4800  0.000427 11254
 6       yes   90%    4800  0.000391 12279
 7       yes   55%   12000  0.000567 21160
 8       yes   51%   24000  0.000929 25826
 9       yes   94%   14401  0.025577   563
10       yes  100%   14401  0.000833 17284
Total          89%  112372  0.030844  3643

Perf index = 53 (util) + 40 (thru) = 93/100
```

# 总结/优化方向
隔离空闲块列表 + first fit + 头插法达到了93分,还算可以吧,比网上一些八十几分的强多了.模块之间嵌合得太紧了,再优化需要动很多结构,暂时就不折腾了.
吞吐量满分就是40,很容易得满分,提高利用率才是关键.优化方向可以是:头插法->按地址排序;first fit -> best fit.甚至一些课上不讲的算法,什么二叉搜索树可能也能用上...

本次的代码量比较大,约500行,比上一次的Cachelab还大,我自己写的那个网络应用更是小卡拉米hhh.
总共写了两天半的时间,也花了不少时间用GDB调试,希望对自己的代码能力有比较好的提升吧!