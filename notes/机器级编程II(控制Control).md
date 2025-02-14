#csapp 

# 处理器状态(部分)
当前执行程序的信息
- 临时数据:%rax,%rbx...
- 当前栈顶:%rsp
- 指令指针:%rip当前正在执行的指令的地址
- 条件码(共8个)
# 条件码
- 1bit的寄存器
	- CF(Carry Flag):进位标志(for unsigned)
	- ZF(Zero Flag):零标志,运算的结果是0就会被设置
	- SF(Sign Flag):符号标志(for signed),如果运算的结果最高位是1(说明结果是负数)就会被设置为1
	- OF(Overflow Flag):溢出标志(for signed),如运算结果溢出就会被设置为1
	- lea指令不会设置这些条件码
## 隐式设置
`addq src,dst` <-> `t = a + b;`
- 当unsigned溢出时设置CF
- 当`t == 0`时设置ZF
- 当`t < 0`时设置SF
- 当signed溢出时设置OF    `(a>0 && b>0 && t<0) || (a<0 && b<0 && t>=0)`
## 显式设置
1. `cmpq b, a`:计算a-b
- 当unsigned溢出时设置CF
- 当`a == b`时设置ZF
- 当`(a-b) < 0`时设置SF
- 当signed溢出时设置OF     `(a>0 && b<0 && (a-b)<0) || (a<0 && b>0 && (a-b)>0)`

2. `test b, a` : 计算a&b
- 当`a & b == 0`时设置ZF
- 当`a & b < 0`时设置SF
有一个操作数是掩码的时候就可以直接得出另一个操作数是整数还是负数.

## 读取条件码

`set* dst`指令:基于条件码,设置dst的最低一位为0或1

set* | 条件 | 含义
:---|:---|:---
`sete` | ZF | equal/zero
`setne` | ~ZF | not equal/not zero
`sets` | SF | negative
`setns` | ~SF | nonnegative
`setg` | ~(SF^OF)&~ZF | greater(signed)
`setge` | ~(SF^OF) | greater or equal(signed)
`setl` | (SF^OF) | less(signed)
`setle` | (SF^OF)&#124ZF | less or equal(signed)
`seta` | ~CF&~ZF | above(unsigned)
`setb` | CF | below(unsigned)

在x86-64整数寄存器中,能够使用最低一字节来存放以上结果
![[attachments/Pasted image 20250118232534.png|700]]
- 举例
```C
int gt(long x,long y){
	return x > y;
}
```

对应的汇编代码:
```x86asm
gt:
	cmpq   %rsi, %rdi    # 比较%rdi-%rsi
	setg   %al           # 当 > 时设置
	movzbl %al, %eax     # mov with zero byte to longword, 因为返回int类型是4字节长字,所以使用%eax
	ret
```
> 需要说明一点,x86-64中当结果是32位时会把剩余的32位设置为0.

# 条件分支
## 跳转j\*指令

j* | 条件 | 含义
---|---|---
`jmp` | 1 | 条件为真时执行
`je` | ZF | equal/zero
`jne`| ~ZF | not equal/not zero
`js` | SF | negative
`jns` | ~SF | nonnegative
`jg` | ~(SF^OF)&~ZF | greater(signed)
`jge` | ~(SF^OF) | greater or equal(signed)
`jl` | (SF^OF) | less(signed)
`jle` | (SF^OF)&#124ZF | less or equal(signed)
`ja` | ~CF&~ZF | above(unsigned)
`jb` | CF | below(unsigned)

## 使用条件分支的例子(旧式)
```C
/* control.c */
long absdiff(long x,long y){
	long result;
	if(x>y)
		result = x-y;
	else
		result = y-x;
	return result;
}
```

生成汇编代码(不使用条件移动)
```shell
gcc -Og -S -fno-if-conversion control.c
```
```x86asm
absdiff:
	cmpq    %rsi, %rdi    # x, y
	jle     .L4
	movq    %rdi, %rax
	subq    %rsi, %rax
	ret
.L4:                     # x <= y
	movq    %rsi, %rax
	subq    %rdi, %rax
	ret
```
可以看到,jump语句就像C语言中的goto语句
```C
long absdiff_j (long x, long y) {
	long result; 
	int ntest = x <= y; 
	if (ntest) goto Else; 
	result = x-y; 
	goto Done; 
Else: 
	result = y-x; 
Done: 
	return result; }
```

## 使用条件移动(优化)
将if-then-else逻辑中then和else都先执行得到两个结果,然后再决定使用哪一个结果
事实证明在非常简单的运算中这样更有效率!
原因:分支会对管道中的指令流造成极大的干扰;条件移动不需要控制权转移
```C
/* 三元运算符 */
	val = test ? then_expr : else_expr;
/* goto */
	result = then_expr;
	eval = else_expr;
	nt = !test;
	if(nt) result = eval;
	return result;
```
gcc会在安全的前提下使用.
对应的汇编代码
```x86asm
absdiff:
	movq    %rdi, %rax    # x
	subq    %rsi, %rax    # result = x - y
	movq    %rsi, %rdx    # y
	subq    %rdi, %rdx    # eval = y - x
	cmqp    %rsi, %rdi    # 比较x, y
	cmovle  %rdx, %rax    # 如果x<=y,result = eval
	ret
```
- 不宜使用条件移动的情况:
	1. 两种结果计算难度大:只有计算简单的情况下才有明显效果
	2. 有不良副作用:例如 `val = p ? *p : 0;`
	3. 污染数据甚至影响判断本身:例如 `val = x>0 ? x*=y : x+=3;`

# 循环

在C语言中,循环结构的实现语句有while语句,for语句,do-while语句,还可以使用goto语句来跳转.而使用goto更接近于汇编语言的实现方式.
```C
/* 使用goto */
long pcount_goto (unsigned long x) { 
	long result = 0; 
	loop: 
		result += x & 0x1; 
		x >>= 1; 
		if(x) goto loop; 
		return result; 
}// 计数x中1的个数
```
对应的汇编代码:
```x86asm
	movl    $0, %eax    # result = 0;
.L2:
	movq    %rdi, %rdx
	addl    %$1, %edx   # t = x&0x1;
	addq    %rdx, %rax  # result += 1;
	shrq    $1, %rax    # x >>=1;
	jne     .L2         # if(x) goto loop;
	rep; ret
```
将while语句转换成goto循环:
```C
/* whlie */
while(test)
	{BODY}


/* goto */
	goto TEST;
LOOP:
	{BODY}
TEST:
	if(test)
		goto loop;
DONE:
	...
```
将do-while语句转换成goto循环:
```C
/* do-whlie */
do
	{BODY}
while(test);

/* goto */
LOOP:
	{BODY}
TEST:
	if(test)
		goto LOOP;
DONE:
	...
```
值得一提的是,gcc使用不同的优化级别会对while语句做出不同的翻译
1. 使用-Og
```C
/* whlie */
while(test)
	{BODY}


/* goto */
	goto TEST;
LOOP:
	{BODY}
TEST:
	if(test)
		goto loop;
DONE:
	...
```

2. 使用-O1
```C
/* whlie */
while(test)
	{BODY}

/* do-while */
if(!test)
	goto DONE;
do
	{BODY}
while(test);
DONE:
	...

/* goto */
if(!test)
	goto DONE;
LOOP:
	{BODY}
	if(test)
		goto loop;
DONE:
	...
```
仔细看,-Og完全顺着C代码的逻辑;而-O1不使用TEST标志,更加简洁.
对于for循环,可以转换成while语句
```C
/* for */
for(Init;Test;Update)
	Body

/* while */
Init;
while(Test)
	Body
	Update
```
如果使用-O1优化,编译器会略去Init后第一次Test,因为通常是满足的.

# switch语句跳转
我们知道,switch语句比if-else语句判断更快.相应地,它对应的机器代码不同.
switch的每一种case在编译器编译时必须是常量.

## 跳转表(jump table)的结构
![[attachments/Pasted image 20250119172324.png|700]]

把所有代码块编译成一块总代码,并将它们存储在内存,加载内存就能得到这些代码块
跳转表就像一个数组,如果知道索引就能随机存取,不必遍历寻找.这就是switch语句的汇编代码的基本思想.
- 举例说明
```C
long switch_eg(long x){
	long w = 1;
	switch(x){
		// case 0:
		case 1: ... break;
		case 2: ... /* Fall Through */
		case 3: ... break;
		// case 4:
		case 5:
		case 6: ... break;
		default: ...
	}
}
```
对应的汇编代码:
```x86asm
switch_eg:
	...
	cmpq    $6, %rdi
	ja      .L8             # 负数和大于6的情况对于解析成unsigned类型来说都是above,这是default情况
	jmp     *.L4(,%rdi,8)   # goto *JTab[x]
```
建立跳转表:
```x86asm
.section    .rodata
	.align 8
.L4:
	.quad    .L8  # x = 0
	.quad    .L3  # x = 1
	.quad    .L5  # x = 2
	.quad    .L9  # x = 3
	.quad    .L8  # x = 4
	.quad    .L7  # x = 5
	.quad    .L7  # x = 6
```
quad只是一个声明,表示这里需要一个8字节的值

现在,分别来看各个代码块
- `x == 1`
```C
switch(x){
	case 1:
		w = y*z;
		break;
}
```
```x86asm
.L3:
	movq    %rsi, %rax    # y
	imulq   %rdx, %rax   # y*z
	ret
```
 - Fall Through的情况
```C
switch(x){
	...
	case 2:
		w = y/z;
		/* Fall Through */
	case 3:
		w += z;
		break;
	...
}
```
分割为
```C
case 2:
	w = y/z;
	goto merge;
case 3:
	w = 1;
merge:
	w += z;
```
![[attachments/Pasted image 20250119180655.png|700]]
实际的汇编代码:
```x86asm
.L5:                    # case 2
	movq    %rsi, %rax
	cqto
	idivq   %rcx        # y/z
	jmp     .L6
.L9:                    # case 3
	movl    $1, %eax    # w = 1
.L6:
	addq    %rcx, %rax  # w += z
	ret
```
跳转表是编译器自动生成的.
如果case有负数,会进行偏置处理,使得最小值是0;
如果case情况少且数值相差大(稀疏),建立跳转表有大量冗余,就会转成if-else结构.
从中我们可以看出编译器十分聪明!
需要在程序执行之前建立好平衡二叉树跳转表,这就是C语言要求case 是常量的原因!