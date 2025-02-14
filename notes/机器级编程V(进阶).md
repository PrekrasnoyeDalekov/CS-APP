#csapp 

# x86-64 Linux内存排列
虽然64位机器可以访问2^ 64的地址,但是这远远超过了目前的硬件水平.实际上现在64位机器会限制只是用47位的地址.
![[attachments/Pasted image 20250123162831.png|600]]

- 栈(Stack):最大8MB,用 `ulimit -a`命令可以查看
- 堆(Heap):调用alloc()函数时动态分配
- 数据(Data):固定分配的数据,全局变量
- 共享库(Share Libraries)/文本(Text):可执行的机器指令,只读.程序执行时会**动态加载**库函数

```C
char big_array[1L<<24]; /* 16 MB */ 
char huge_array[1L<<31]; /* 2 GB */ 
int global = 0; 

int useless() { return 0; } 
int main () { 
	void *p1, *p2, *p3, *p4; 
	int local = 0; 
	p1 = malloc(1L << 28); /* 256 MB */ 
	p2 = malloc(1L << 8); /* 256 B */ 
	p3 = malloc(1L << 32); /* 4 GB */ 
	p4 = malloc(1L << 8); /* 256 B */ 
	/* Some print statements ... */ 
}
```
当运行这个程序时,可以观察到
![[attachments/Pasted image 20250123163815.png|600]]
小的数组地址低,大的数组地址高.中间的区域即使还未被分配,依然视为堆的一部分,引用它会产生 Segmentation Fault.
如果上下两块相遇,alloc函数再也无法分配内存,就会分配失败返回空指针.
所以我们能够对哪些内存对应程序的哪一部分有一个大概的印象,在做bomblab实验时,我也注意到,0x7fff...是栈空间,0x40...是函数体,0x60...是一些变量.


# 缓冲区溢出
在学习C语言的过程中我已经了解到有一些处理字符串的函数是不安全的,没有边界检查.比如 `gets()`  `*scanf()` `strcat()` 函数等等.

这里有一个不安全的函数:
```C
void echo() { 
	char buf[4]; /* Way too small! */ 
	gets(buf); 
	puts(buf); 
}

void call_echo(){
	echo();
}
```
这个函数智能存储3个字符的字符串.不过实际上输入大于3个时也不会立即崩溃,要看编译器生成的汇编代码
```x86asm
echo:
	sub    $0x18, %rsp
	mov    %rsp, %rdi
	callq  <gets>
	mov    %rsp, %rdi
	callq  <puts>
	add    $0x18, %rsp
	retq
```
所以编译器实际上给栈分配了24字节的空间,能够容纳长度23的字符串.一旦字符串多于23个字符,就会把`call_echo`函数栈帧的返回地址覆盖掉.当 `echo()`函数执行完毕后,跳转到返回地址就到别的地方去了,从而引发无法预料的行为,程序崩溃.
![[attachments/Pasted image 20250123170310.png|300]] =>![[attachments/Pasted image 20250123170512.png|300]]

## 代码注入攻击
利用这个缓冲区溢出的漏洞,可以进行代码注入攻击
- 输入字符串包括转换成字节形式的指令
- 覆盖返回地址,使其指向注入的代码
![[attachments/Pasted image 20250123171356.png|500]]

## 避免缓冲区溢出攻击
1. 在代码层面增强健壮性
避免使用不安全的函数:
- 用 `fgets()` 替代 `gets()`
- 用 `strncpy()` 替代 `strcpy()`
- 在 `*scanf()`的`%s`前加上数字限制最大读取长度

2. 系统级保护
- 栈随机化(ASLR:Address Space Layout Randomization)
	- 程序开始时,在栈上分配一个随机大小的空间
	- 整个程序的栈地址随之偏移
	- 攻击者更难构造返回地址
	![[attachments/Pasted image 20250123172651.png|400]]

- 不可执行代码标记
	通过将堆栈标记为**不可执行的**,能够避免这种攻击方式

3. 金丝雀(Canary)保护机制
Bomblab的[[Bomblab#phase_5|phase_5]]中出现过
在gcc中添加编译选项
`gcc`
` -fstack-protector`: 启用保护,只为局部变量中含有char数组的函数插入保护(默认开启)
` -fstack-protector-all`: 启用保护,为所有函数插入保护代码
` -fno-stack-protector`: 禁用保护
加入了栈保护后
```x86-64asm
echo:
	sub    $0x18, %rsp

	mov    %fs:0x28, %rax
	mov    %rax, 0x8(%rsp)

	mov    %rsp, %rdi
	callq  <gets>
	mov    %rsp, %rdi
	callq  <puts>

	mov    0x8(%rsp), %rax
	xor    %fs:0x28, %rax
	je     .notoverflow
	callq  <__stack_chk_fail@plt>
	
.notoverflow:
	add    $0x18, %rsp
	retq
```
这样,程序将一个随机的cookies放在了%rsp+8的地址上,如果输入的字符串长度大于等于8,覆盖到了这个随机数,程序返回之前的检查就不能通过.
![[attachments/Pasted image 20250123175437.png|400]]

## 面向返回的编程攻击(Return-Oriented Programming Attack)
系统级保护(栈随机化/不可知性标记)给攻击提升了难度,但是现有代码/库函数是可执行的.
ROP利用现有代码,需要栈溢出漏洞,没有克服canary保护

> 这一部分有些难理解


我目前暂时的理解(不一定正确,需要去做Attacklab进一步学习)是,通过栈溢出覆盖了多个返回地址,每个返回地址指向了攻击者抽取的gadget.一系列gadget都以ret结尾,使得gadget能够连成指令序列.
![[attachments/Pasted image 20250123185428.png|600]]
- 抽取gadget构建程序:指令序列以 `ret` 终止(0xc3)
	- 现有函数的结尾
```x86asm
4004d0 <ab_plus_c>:
	4004d0:    48 0f af fe
	4004d4:    48 8d 04 17
	4004d8:    c3
```
如果gadget地址取0x4004d4,会执行
```x86asm
	lea    (%rdi,%rdx,1), %rax
	retq
```
这就是含有一定功能和ret的gadget.
	- 任何二进制片段
```x86asm
<setval>:
	4004d9:    c7 07 d4 48 89 c7
	4004df:    c3
```
如果gadget地址取0x4004dc,刚好截取
```x86asm
	movl    %rax, %rdi
	retq
```
这就是含有一定功能和ret的gadget.

# 联合体(Union)
C语言中,结构体会为每个字段分配单独的存储空间;而联合体使用占空间最大的字段的大小来分配内存,智能存储一个字段的值.
```C
union U1 { 
	char c; 
	int i[2]; 
	double v; 
} *up;
```
![[attachments/Pasted image 20250124174011.png|400]]
对一个存有数据的联合体读取不同字段的结构取决于小端序和大端序.

