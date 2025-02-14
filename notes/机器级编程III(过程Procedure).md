#csapp 

# 过程的机制
- 传递控制
当P调用Q时,程序必须以某种方式跳入Q中,执行Q的代码,然后当Q执行到退出时,程序需要以某种方式回到P.
因此为了返回到正确的位置,我们需要记录返回位置的信息.
- 传递参数
Q是一个函数,它能够接收参数并能在函数内部使用这个参数.
当P调用Q的时候,P给Q传递了一个具体值.
当Q返回值时,P也能够使用返回值.
- 内存管理
过程执行时分配内存,返回时释放内存.

# 栈的结构
栈不是单独的硬件,而是内存的某一部分.程序使用栈来管理过程调用与返回的状态.
当调用时,需要一些信息;当从调用中返回时,所有的信息可以被丢弃,符合栈的后进先出的规则.
在x86栈中,栈开始的地址实际上是一个非常高的地址,当栈增长时,通过减小栈指针%rsp来实现.%rsp的值是当前栈顶的地址,每次分配更多栈的空间时,都会递减该指针.

|Stack | Address 
|---|---
|Bottom  | high address
|  |  
| | 
| | 
| |  ↓ stack growing down
| | 
| | 
| | 
| %rsp:Top| low address 

汇编当中有 `push` 和 `pop` 两个指令来对栈进行操作.
## push入栈
```x86asm
	pushq    src 
```
操作数src入栈,此操作数可以来自寄存器,内存或立即数:
首先%rsp减小8,然后把操作数复制到%rsp指向的内存处.
不能把内存中的数据直接放入到栈中,所以src是一个寄存器或者立即数.


## pop出栈
```x86asm
	popq     dst
```
首先把%rsp地址指向的内存的值保存到dst寄存器中,然后%rsp增加8.
dst只能是寄存器.

# 传递控制
> 注意call, ret并不是完成了调用函数的全部工作,只是控制部分
使用栈来支持call和ret指令

## call
```x86asm
	call    label
```
1. 返回地址入栈
2. 跳转到label

## 返回地址
返回地址是call的下一条指令

## ret
1. 返回地址出栈
2. 跳转到返回地址

## 举例
对于
```x86asm
400540 <multstore>:
	...
	400544: callq    400550 <mult2>
	400549: mov      %rax, (%rbx)
	...

400550 <mult2>:
	400550: mov     %rdi, %rax
	...
	400557: ret
```
首先当程序计数器%rip执行到0x400544的时候,%rsp减去8,0x400549入栈,%rip修改为调用的函数的地址;调用的函数返回后,0x400549出栈,%rsp增加8,%rip修改为0x400549,也就是执行call的下一条指令.
![[attachments/Pasted image 20250121180538.png|500]]
![[attachments/Pasted image 20250121180559.png|500]]
![[attachments/Pasted image 20250121180618.png|500]]
![[attachments/Pasted image 20250121180635.png|500]]

> 不像 `push` 和 `pop`指令能够用mov实现,由于没有指令能够之间操作%rip,所以 `call` 和 `ret` 不能用其他指令实现.


# 传递参数
头6个参数(如果有)依次存放在这6个寄存器中:

参数|寄存器
---|---
第1个|%rdi
第2个|%rsi|
第3个|%rdx|
第4个|%rcx|
第5个|%r8|
第6个|%r9|
如果参数的个数多于6个,剩下的参数会存放在栈上.

函数返回值为%rax

# 管理局部数据

## 面向堆栈的语言
- 支持递归的语言:例如C,Java等等大多数语言
	- "可重入"的函数:在任何给定的时间只有一个函数在运行
	- 需要一些空间来存储每个实例的状态:参数,局部变量,返回指针
我们把栈上用于特定调用的每个内存块称为**栈帧**.

## 栈帧
在栈上为每个被调用且未返回的过程保留一个栈帧.
![[attachments/Pasted image 20250121215045.png|500]]
通常一个栈帧由两个指针分割,一个是栈顶指针%rsp,另一个是基底指针
- 栈帧的内容
	- 返回信息
	- 局部存储
	- 临时空间

![[attachments/Pasted image 20250121221021.png|500]]
- 调用者的栈帧
如果需要传递6个以上的参数,调用者实际上将使用自己的栈帧来存储这些参数
返回地址入栈

- 当前栈帧
如果使用%rbp,还需要存储之前的%rbp
参数,局部变量...

## 举例
```C
/* incr */
long incr(long *p,long val){
	long x = *p;
	long y = x + val;
	*p = y;
	return x;
}// 以val增加p指向的整数
```
incr的汇编代码
```x86asm
incr:
	movq    (%rdi), %rax
	addq    %rax, %rsi
	movq    %rsi, (%rdi)
```

```C
/* call_incr */
long call_incr(){
	long v1 = 12345;
	long v2 = incr(&v1, 1)
	return v1 + v2;
}
```
call_incr的汇编代码
```x86asm
call_incr:
	subq    $16, %rsp
	movq    $12345, 8(%rsp)
	movl    $1, %esi           # 第二个参数1
	leaq    8(%rsp), %rdi      # 第一个参数*8(%rsp)
	call    incr
	addq    8(%rsp), %rax
	addq    $16, %rsp
	ret
```
栈的内容|
---|
...|
...|
ret address|
12345          ←%rsp+8|
unuse          ←%rsp|
> *编译器分配栈帧空间时会预留一些不一定会使用的空间*

## 寄存器保存约定
寄存器的数量是有限的,不同函数对寄存器重复进行操作可能会导致冲突,所以有些寄存器的值不得不先保存到内存当中,并遵循一定规则约定.
1. 调用者保存(caller saved)
调用者在调用前把临时数据保存在它的栈帧中.
![[attachments/Pasted image 20250122003159.png|700]]

2. 被调用者保存(callee saved)
![[attachments/Pasted image 20250122003225.png|700]]
被调用者在使用寄存器前把寄存器的数据保存在它的栈帧中.返回时重新存取.

# 阐述递归
使用递归计数unsigned int 的1位的个数
```C
/* Recursive popcount */
long pcount_r(unsigned long x){
	if(x==0)
		return 0;
	else 
		return (x & 1) + pcount_r(x>>1);
}
```
汇编代码
```x86asm
pcount_r:
	movl    $0, %eax
	testq   %rdi, %rdi
	je      .L6
	pushq   %rbx
	movq    %rdi, %rbx       # x先存储到%rbx
	andl    $1, %eax      
	shrq    %rdi             # x>>=1,传给递归函数
	call    pcount_r
	addq    %rbx, %rax       # %rax是递归返回结果
	popq    %rbx             # x恢复到之前的值
.L6:
	rep; ret
```
