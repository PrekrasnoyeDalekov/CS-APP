#csapp 

课件:[15-213: Introduction to Computer Systems / Schedule Fall 2015](https://www.cs.cmu.edu/afs/cs/academic/class/15213-f15/www/schedule.html)
视频:[2015 CMU 15-213 CSAPP 深入理解计算机系统 课程视频i](https://www.bilibili.com/video/BV1iW411d7hd)
实验:[CS:APP3e, Bryant and O'Hallaron](https://csapp.cs.cmu.edu/3e/labs.html)
# 2015 CMU 15-213: Introduction to Computer Systems
Instructors: Randal E. Bryant and David R.O'Hallaron
(深入理解计算机系统 CSAPP作者)
## 课程主题：
- 大多数 CS 和 CE 课程都强调抽象
	- 抽象数据类型
	- 非典型分析
 - 这些抽象有其局限性
	- 特别是在存在错误的情况下
	- 需要了解底层实现的细节
- 学习本课程的有益成果
	- 成为更有效率的程序员
		- 能够有效地发现和消除bug
		- 能够理解并调整程序性能
	- 为以后学习 CS 和 ECE 的 "系统 "课程做准备
		- 编译器、操作系统、网络、计算机体系结构、
		- 嵌入式系统、存储系统等

## 1:int 不是整数,float 不是实数
$$ x^2\ge0 $$
但是计算机中存在数值溢出的问题，不总是我们期望的行为
`print 50000*50000`
输出-1794967296
而`print 1e20+(-1e20+3.14)`
输出0
`print 1e20+-1e20+3.14`
输出3.1400000000000001

## 计算机算术
- 不会产生随机数
- 不能想当然认为它遵守一般的数学性质
- 需要理解哪些情境下使用哪些抽象
- 对编译器和严谨的应用程序员的重要话题

## 2:了解汇编

## 3:内存很重要
随机存取存储器是一种非物理抽象概念

## 4:提升性能
``` C
void copyij(int src[2048][2048], int dst[2048][2048]) 
{ 
	int i,j; 
	for (i = 0; i < 2048; i++) 
		for (j = 0; j < 2048; j++) 
			dst[i][j] = src[i][j]; 
}
```
4.3ms
``` C
void copyij(int src[2048][2048], int dst[2048][2048]) 
{ 
	int i,j; 
	for (j = 0; j < 2048; j++) 
		for (i = 0; i < 2048; i++) 
			dst[i][j] = src[i][j]; 
}
```
81.8ms
这个分层存取的例子说明,性能取决于访问的方式
为什么性能有差异?
![[attachments/Pasted image 20250113113958.png]]
> 本书封面

## 5:计算机不仅执行程序
- I/O系统
- 网络服务