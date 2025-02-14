#csapp 

# 英特尔/AMD处理器的历史和架构构


# C语言,汇编语言,机器代码
## 定义
- 指令集架构(ISA:instruction set architecture)
	处理器设计用于理解和编写汇编/机器代码的部分
- 微架构:架构的实现
-  代码形式:
	- 机器代码:处理器执行的字节级别的程序
	- 汇编代码:代表机器代码的文本
- ISA有intel x86-64,ARM等

## 汇编/机器代码概览
![[attachments/Pasted image 20250117182544.png | 700]]
- 程序计数器(PC:Program counter):存放下一个指令的地址
- 寄存器文件(Register):程序中重复使用的数据
- 条件码(Condition code):存储指令的运行结果,逻辑运算的结果,用于实现条件分支
- 内存:地址数组,存放数据和代码,栈支持进程
## C语言编译原理

![[attachments/Pasted image 20250117184342.png|700]]
编译其实是一系列步骤,gcc其实也是一系列程序
1. 编译成汇编语言

C语言源代码:
```C
/* sum.c */
long plus(long x, long y);

void sumstore(long x, long y, long *dest){
	long t = plus(x, y);
	*dest = t;
}
```

使用命令生成汇编代码:
``` bash
gcc -Og -S sum.c
```

对应产生的汇编语言代码(主要部分)
``` x86asm
sumstore: 
	pushq %rbx 
	movq %rdx, %rbx 
	call plus 
	movq %rax, (%rbx)
	popq %rbx
	ret
```
> 汇编语言中,整数数据占有1,2,4,8字节
> 浮点数据占有4,8,或10字节
> 注意数组,结构体这些数据类型不存在于机器级别,是由编译器产生的
- 反汇编器:从目标代码转回到汇编代码,变量的名称会丢失
首先,只需要由一个可执行文件,这是一个二进制文件
```shell
gcc -Og sum.c -o sum
```
可以使用objdump程序
``` shell
objdump -d sum
```
或者使用gdb
```shell
gdb sum
(gdb) disassemble sumstore
Dump of assembler code funtion sumstore:
	0x0000000000400595 <+0>:    push %rbx
	0x0000000000400596 <+1>:    mov %rdx,%rbx
	0x0000000000400599 <+4>:    callq 0x400590 <plus>
	0x000000000040059e <+9>:    mov %rax,(%rbx)
	0x00000000004005a1 <+12>:    pop %rbx      
	0x00000000004005a2 <+13>:    retq         
End of assembler dump.
```

2. 运行汇编程序
把文本表示的指令变成实际的字节指令

3. 链接器
将不同的文件融合在一起,包括我编写的程序和库文件
当运行程序时,可能还有一些库在程序首次开始时动态导入
# 汇编基础:寄存器,操作数,move指令
## x86-64的整数寄存器
![[attachments/Pasted image 20250118182908.png|700]]
有16个寄存器可以用来保存整数和指针,其中一些以字母为名,一些以数字为名
使用%r的是64位,使用%e的是32位,%e版本只是%r实体的低32位.


IA32的8个寄存器就是上面的所有%e
旧机器可以使用这些寄存器的低16位(2字节)和低8位(1字节),已经是"历史遗产"了.
![[attachments/Pasted image 20250118184424.png|700]]

## mov指令
`movq src, dst` : 复制src到dest


## 操作数的类型
- 立即数:常整数
	- 类似于C语言的常量,但是带有'$',例如 `$0x400`,` $-533`
	- 编码为1,2或4字节
- 寄存器:16个整数寄存器之一
	- 例如`%rax`,`%r13`
	- `%rsp`是保留为特殊用途
- 内存:寄存器保存的地址对应的内存
	- 在寄存器的名称外加上括号,例如:`(%rax)`


## mov和操作数的合并使用
$$
\text{movq}
\left \{ \begin{matrix} 
\text{Imm} \left \{ \begin{matrix} \text{Reg} && \text{movq \;\$0x4,\%rax} && \text{把\%rax的值设置为0x4}\\ \text{Mem} && \text{movq\;\$-147,(\%rax)} && \text{把\%rax指向的内存设置为-147} \end{matrix}\right. \\ 
\text{Reg}   \left \{ \begin{matrix} \text{Reg} && \text{movq \%rax,\%rdx} && \text{把\%rax的值复制到\%rdx}\\ \text{Mem} &&  \text{movq\;\%rax,(\%rdx)}&&\text{把\%rax的值复制到\%rdx指向的内存}\\ \end{matrix}\right. \\  
\text{Mem}\;\;\;\;\;\;\;\; \text{Reg} \;\;\;\;\;\;\;\text{movq\;(\%rax),\%rdx}\;\;\;\;\;\;\text{把\%rax指向的内存的内容复制到\%rdx} \\
 \end{matrix}\right.
$$
不允许从一个内存直接复制到另一个内存,立即数是常量不能当"右值",所以一共有5中组合

- 一个简单的例子
```C
void swap(long *xp,long *yp){
	long t0 = *xp;
	long t1 = *yp;
	*xp = t1;
	*yp = t0;
}
```
这是一个简单的交换内存中两个数的函数,(long类型刚好能用%r的6位模式),对应生成的汇编代码的核心部分为:
```x86asm
swap:
	movq (%rdi), %rax
	movq (%rsi), %rdx
	movq %rdx, (%rdi)
	movq %rax, (%rsi)
	ret
```
这里解释一下,在x86中,函数的参数总是复制在特定的寄存器中:
%rdi是存放第一个参数寄存器
%rsi是存放第二个参数寄存器
%rax是存放返回值的寄存器
最多可以有6个.

## 内存地址的引用方式
上面我们已经看到通过在寄存器名称外加括号能够表示寄存器的值指向的内存.也有更高级,普遍的方法:
### 一般格式
`D(Rb,Ri,S)`表示Mem[Reg[Rb]+S\*Reg[Ri]+D]
- D(displacement)是固定偏移量(字节)
- Rb(base register)是基址:任意16个寄存器之一
- Ri(index register)是索引:除了%rsp的任意寄存器
- S(scale)是比例因子:(1 for char,2 for short,4 for int或8for long).
### 特殊格式
格式|表示的内存
---|---
`(Rb,Ri) `| Mem[Reg[Rb]+Reg[Ri]]
`D(Rb,Ri)` |  Mem[Reg[Rb]+Reg[Ri]+D]
`(Rb,Ri,S)` |  Mem[Reg[Rb]+S\*Reg[Ri]]
`D(,Ri,S)` | Mem[S\*Reg[Ri]+D]

## leaq指令
`leaq src,dst`:src是一块内存,将其对应的地址保存到dst.类似于C语言中的&取值运算符.dst必须是寄存器.
看起来像是mov指令,但这是一种非常方便的算术运算方式,无需内存引用,所以C编译器喜欢使用它
- 一个简单的例子
```C
long m12(long x){
	return x*12;
}
```
对应的汇编代码的核心部分:
```x86asm
leaq (%rdi,%rdi,2), %ras    # t = x+x*2
salq $2, %rax               # 右移2位,相当于乘4
```


## 其他指令
指令 | 对应代码
---|:---
`addq src,dst` | `dst += src; ` 
`subq src,dst` | `dst -= src; ` 
`imulq src,dst` | `dst *= src; ` 
`salq src,dst` | `dst <<= src; ` 
`sarq src,dst` | `dst >>= src; ` 
`shlq src,dst` | `dst <<<= src; ` 
`shrq src,dst` | `dst >>>= src; ` 
`xorq src,dst` | `dst ^= src; ` 
`andq src,dst` | `dst &= src; ` 
`orq src,dst` | `dst &#124= src; `
`notq dst` | `dst = ~dst;`
`incq dst` | `dst += 1;`
`decq dst` | `dst -= 1;`
`incq dst` | `dst = -dst;`

# 总结
``` C
long arith (long x, long y, long z) {
	long t1 = x+y; 
	long t2 = z+t1; 
	long t3 = x+4; 
	long t4 = y * 48; 
	long t5 = t3 + t4; 
	long rval = t2 * t5; 
	return rval; 
}
```

```x86asm
arith: 
	leaq (%rdi,%rsi), %rax           # t1 
	addq %rdx, %rax                  # t2 
	leaq (%rsi,%rsi,2), %rdx 
	salq $4, %rdx                    # t4 
	leaq 4(%rdi,%rdx), %rcx          # t5 
	imulq %rcx, %rax                 # rval 
	ret
```

我从数学物理世界跨进计算机世界,先是接触Python语言,后来学习C语言,到如今与汇编语言交手,回想起来真是心潮澎湃!一开始写Python代码,连简单的循环都要Debug好久,后来学习了C语言,相比起来觉得Python非常简单啊,如今接触汇编语言这样奇怪曲折的表达方式,突然觉得C语言根本就像英语一样自然了!