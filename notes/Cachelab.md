#csapp #csapplab 

说实话,初次查看这个实验,我感到非常困惑,不知道要做什么.Writeup也只是介绍了测试方法,我根本不知道要写什么!感觉没有说清楚啊...
只好参考了别人的解答,才明白要写什么代码.
[csapp实验5-cachelab实验详解](https://blog.csdn.net/m0_65591847/article/details/132323877)
[CSAPP: Cachelab全注释+思路和建议](https://zhuanlan.zhihu.com/p/456858668)
# Part A:编写缓存模拟器
我并不需要真正去内存读取什么数据,只是模拟缓存的行为.
可以看到traces/文件夹下的.trace文件:
![[attachments/Pasted image 20250130213310.png|500]]
根据writeup的说明,每一行的格式是:
```
[space]operation address,size
```

operation|说明
---|---
I|加载指令(前面没有空格)
L|加载数据
S|存储数据
M|修改数据
这就是模拟CPU给缓存的指令吧

csim.c文件的要求:
以 `printSummary(hit_count, miss_count, eviction_count);`结尾来返回缓存模拟器的使用情况,最后要达到与
```shell
./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
```
输出的结果一致.
我回顾高速缓存的知识: [[高速缓存#缓存组织方式]]
可知(s,E,b)正是缓存的基本参数
我的可执行文件需要接收命令行参数,Writeup推荐我使用 `getopt`库函数.
替换策略是什么?Writeup指明采用LRU策略(least-recently used),替换最近没有被访问的行.

先分析到这里,具体思路见代码的注释,体现了模块化的编程思想.

测试结果:
```shell
$ make && ./test-csim
gcc -g -Wall -Werror -std=c99 -m64 -o csim csim.c cachelab.c -lm
# Generate a handin tar file each time you compile
tar -cvf kim-handin.tar  csim.c trans.c
csim.c
trans.c
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27

TEST_CSIM_RESULTS=27
```

# Part B:优化矩阵转置
测试时使用valgrind工具抽取缓存使用情况,所以要先安装valgrind工具,否则无法开展.
```shell
sudo apt update && sudo apt-get install valgrind
```
> 哇,又用一百多MB硬盘空间...

一共有三个测试参数:32\*32,64\*64和61\*67.Writeup提示我,完全可以根据不同的输入参数运行不同的优化函数.

1. 32\*32
先试试不做任何优化:
```shell
$ make && ./test-trans -M 32 -N 32
gcc -g -Wall -Werror -std=c99 -m64 -O0 -c trans.c
gcc -g -Wall -Werror -std=c99 -m64 -o test-trans test-trans.c cachelab.c trans.o
gcc -g -Wall -Werror -std=c99 -m64 -O0 -o tracegen tracegen.c trans.o cachelab.c
# Generate a handin tar file each time you compile
tar -cvf kim-handin.tar  csim.c trans.c
csim.c
trans.c

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:869, misses:1184, evictions:1152

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:869, misses:1184, evictions:1152

Summary for official submission (func 0): correctness=1 misses=1184

TEST_TRANS_RESULTS=1:1184
```
对于32\*32矩阵,miss达到了1184.
如何优化呢?
根据Writeup,缓存的参数是(s=5, E=1, b=5),也就是有32组,每组1行([[高速缓存#直接映射高速缓存]]),每行32字节,可容纳8个int整数.
如果逐个元素转置,B数组第0行第0个元素在第0组,第1行第0个元素在第4组,依次进行下去,到第9行时,32个组中的0,4,8,12,16,20,24,28组写有缓存,第9行会覆盖第0组,虽然缓存未满,但是发生了[[存储器层次结构#缓存|conflict miss]].
回顾矩阵乘法的分块思想([[高速缓存#矩阵分块,提高时间局部性]]),对于32\*32矩阵可以分块成8\*8.
> 达到misses<300即为满分


2. 64\*64
但是对于64\*64矩阵,应该分成4\*4,为了充分利用每行8个元素的缓存,也应在8\*8块下整体考虑.具体见代码的注释

说一个很坑的点:(也是看了别人的解答[^1]才知道),查看
```shell
./csim-ref -v -s 5 -E 1 -b 5 -t trace.f0
```
发现A和B的地址相差0x40000,是我cache size的整数倍,这导致在处理A和B的对角线元素时如果 `L A的元素`之后马上 `S B的元素` 总是会发生eviction!解决办法是创建临时变量.
> 达到misses<1300即为满分

[^1]: [CSAPP: Cachelab全注释+思路和建议](https://zhuanlan.zhihu.com/p/456858668)

3. 61\*67
61和67是质数,没有那么容易发生[[存储器层次结构#缓存|conflict miss]]了,寻找合适的block_size达到要求即可
> 达到misses<2000即为满分

尝试`block_size = 14`:
```shell
$ make && ./test-trans -M 61 -N 67
gcc -g -Wall -Werror -std=c99 -m64 -O0 -c trans.c
gcc -g -Wall -Werror -std=c99 -m64 -o test-trans test-trans.c cachelab.c trans.o
gcc -g -Wall -Werror -std=c99 -m64 -O0 -o tracegen tracegen.c trans.o cachelab.c
# Generate a handin tar file each time you compile
tar -cvf kim-handin.tar  csim.c trans.c
csim.c
trans.c

Function 0 (1 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:6182, misses:1997, evictions:1965

Summary for official submission (func 0): correctness=1 misses=1997

TEST_TRANS_RESULTS=1:1997
```
刚好满足要求!有没有更少的呢?
尝试 `block_size = 15`:
```shell
TEST_TRANS_RESULTS=1:2022
```
反而变多了?
尝试 `block_size = 16`:
```shell
TEST_TRANS_RESULTS=1:1993
```
尝试 `block_size = 17`
```shell
TEST_TRANS_RESULTS=1:1951
```
尝试 `block_size = 18`
```shell
TEST_TRANS_RESULTS=1:1962
```
尝试 `block_size = 19`
```shell
TEST_TRANS_RESULTS=1:1980
```
尝试 `block_size = 20`
```shell
TEST_TRANS_RESULTS=1:2003
```
尝试 `block_size = 21`
```shell
TEST_TRANS_RESULTS=1:1958
```
尝试 `block_size = 22`
```shell
TEST_TRANS_RESULTS=1:1960
```
尝试 `block_size = 23`:
```shell
TEST_TRANS_RESULTS=1:1929
```
尝试 `block_size = 24`:
```shell
TEST_TRANS_RESULTS=1:2016
```
所以,block_size取14,16,17,18,19,20,21,22,23都是满分,其中属`block_size = 23`的misses最少.
> *不得不说,测试速度好慢啊...*

# 成果总览
文件夹目录下有一个 `drive.py`文件供测试所有结果,是python2时代编写的.我找到了python3版本,替换后运行:
```shell
$ python3 drive.py
Part A: Testing cache simulator
Running ./test-csim
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27


Part B: Testing transpose function
Running ./test-trans -M 32 -N 32
Running ./test-trans -M 64 -N 64
Running ./test-trans -M 61 -N 67

Cache Lab summary:
                        Points   Max pts      Misses
Csim correctness          27.0        27
Trans perf 32x32           8.0         8         288
Trans perf 64x64           8.0         8        1220
Trans perf 61x67          10.0        10        1929
          Total points    53.0        53
```

我发现本实验的Part B在我这WSL上运行的misses总是比网上的解答的misses大一点点,即使我的思路,算法与他们是一致的.这究竟是隐含了WSL与真实Linux环境的区别,还是代码风格细节的差异呢?