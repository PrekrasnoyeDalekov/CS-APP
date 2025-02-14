#csapp 

之前讨论的数据类型都是整型,这部分是将多个数据元素聚合在一起.
# 数组
## 数组分配空间
```C
	ElemType A[L];
```
在内存中连续分配 L * sizeof(ElemType) 字节的空间.

## 读取数组
```C
#define ZLEN 5
typedef int zip_dig[ZLEN];

int get_digit(zip_dig z, int digit){
	return z[digit];
}
```
对应的汇编代码
```x86asm
	# %rdi = z
	# %rsi = digit
	movl    (%rdi,%rsi,4), %eax      # z[digit] = *(%rdi + %rsi*4)
```

## 修改数组
```C
void zincr(zip_dig z){
	size_t i;
	for(i = 0;i < ZLEN;i++)
		z[i]++;
} // 对数组的每个元素加1
```
对应的汇编代码
```x86asm
    # %rdi = z
    movl    $0, %eax               # i = 0
    jmp     .L3
.L4:
	addl    $1, (%rdi, %rax, 4)    # z[i]++
	addq    $1, %rax               # i++
.L3:
	cmp    $4, %rax                # 循环条件判断
	jbe    .L4
	rep; ret
```

## 多维数组
```C
	ElemType A[R][C];
```
在数据结构中我已经学过矩阵的相关操作,跟这里是贯通的.数组大小占 `R*C*sizeof(ElemType)` 字节,在内存中按行主序储存. `A[i][j]`的地址是 `A+(i*C+j)*sizeof(ElemType)`
```C
int get_pgh_digit (int index, int dig) { 
	return pgh[index][dig]; 
}
```
对应的汇编代码
```x86asm
get_phg_digit:
	leaq (%rdi,%rdi,4), %rax   # 5*index 
	addl %rax, %rsi            # 5*index+dig
	movl phg(,%%rsi,4), %eax   # M[pgh + 4*(5*index+dig)]
```

另一种声明方式是
```C
#define UCOUNT 3
zip_dig CMU = {1,5,2,1,3};
zip_dig MIT = {0,2,1,3,9};
zip_dig UCB = {9,4,7,2,0};

zip_digt univ[UCOUNT] = {MIT, CMU, UCB};
int get_univ_digit (size_t index, size_t digit) { 
	return univ[index][digit]; 
}
```
也可以这样读取
```x86asm
get_univ_digit:
	salq    $2, %rsi                    # digit*4
	addq    univ(,%rdi,8), %rsi         # p = univ[index] + digit*4  一个指针的大小是8字节
	movl    (%rsi), %eax                # return *p
	ret
```
与第一种相比,这种多维数组的定义方式,存放的元素是指针,元素不一定连续.

![[attachments/Pasted image 20250122114001.png| 800]]

# 结构体

## 结构体分配空间
```C
struct rec{
	int a[4];
	size_t i;
	struct rec *next;
}
```
![[attachments/Pasted image 20250122115115.png|400]]
- 结构体在内存中以块呈现,容纳所有字段.
- 字段按照声明的顺序排列.
- 编译器将追踪每个字段的起始位置,然后生成适当的字节偏移(相对结构体地址)来得到不同字段.
## 产生结构体成员的指针
编译器能记住每个字段起始的位置,所以,对于上面那个结构体
```x86asm
get_ap:
	# r in %rdi, idx in %rsi
	leaq    (%rdi, %rsi, 4), %rax     # &r->a[idx]
	ret

get_i:
	# r in %rdi
	movslq   16(%rdi), %rax     # i = M[r+16]

get_next:
	# r in %rdi
	movq     24(%rdi), %rax     # next = M[r+24]
```

## 内存对齐
为了提升存取性能而使用内存对齐.
编译器实际会在分配空间时在数据结构中插入一些空白的不被使用的字节.
> 我在用结构体设计IPv4报文头部的时候发现了这种现象.

一个这样的结构体:
```C
struct s1{
	char c;
	int i[2];
	double v;
}
```
在对齐之前
![[attachments/Pasted image 20250122122544.png|500]]
对齐之后
![[attachments/Pasted image 20250122122616.png|500]]
关于内存对齐的详细介绍,我参考了[C/C++内存对齐详解 - 知乎](https://zhuanlan.zhihu.com/p/30007037)
在x86-64机器上,gcc默认 `#pragma pack(8)`,所以"对齐系数"是8字节.
**有效对齐值**:"对齐系数"与结构体中最长数据类型中较小的那个
结构体第一个成员的偏移量是0,以后每个成员的偏移量是该成员大小与有效对齐值中较小的那个的整数倍;结构体总大小为有效对齐值的整数倍.
对图片进行解释:有效对齐值是8字节,c放在第一个位置没有疑议,然后i[0]的地址需要是4的倍数,所以在c和i[0]之间填充3个字节.v的地址需要是8的倍数,所以在i[1]和v之间填充4个字节.
这给了我取消内存对齐的方法:设置 `#pragma pack(1)` 或者在定义结构体的最后加上 `__attribute__((packed))`会让这个结构体内部不对齐.
通过调整结构体中成员的顺序,可能节省空间.

# 浮点数
CPU有16个浮点数寄存器,都是调用者保存.
AVX浮点体系允许数据存储在YMM寄存器中,命名为%ymm0 ~ %ymm15
SSE则使用%xmm0 ~ %xmm15.
每个YMM寄存器保存32字节,低16字节可以作为XMM寄存器使用.
CS:APP书中讲解的是AVX体系,视频讲解的是SSE体系.我亲自实验了两者,它们的指令是不一样的:
Ctrl+F12生成汇编代码
![[attachments/Pasted image 20250122183441.png|400]]
```C
double sum(double a,double b){  
    return a+b;  
}  
  
int main(){  
    double d = 7.8;  
    double a = 3.14;  
    return sum(a,d);  
}
```
 - AVX版本
![[attachments/Pasted image 20250122182833.png|600]]
(剔除了大量注释和.开头的变量)
```x86asm
 # options passed: -mavx -mtune=core2 -march=nocona -Og -std=c99  
sum:  
    vaddsd    %xmm1, %xmm0, %xmm0     # tmp87, tmp86, tmp85  
    ret         
main:  
    subq    $40, %rsp     #,  
    call    __main     #  
    vmovsd    .LC0(%rip), %xmm1     #,  
    vmovsd    .LC1(%rip), %xmm0     #,  
    call    sum     #  
    vcvttsd2sil    %xmm0, %eax     
    addq    $40, %rsp     #,  
    ret       
.LC0:  
    .long    858993459  
    .long    1075786547  
    .align 8  
.LC1:  
    .long    1374389535  
    .long    1074339512  
    .ident    "GCC: (x86_64-posix-seh, Built by MinGW-Builds project) 11.4.0"
```
 - SSE版本
![[attachments/Pasted image 20250122183302.png|600]]
(提出了大量注释和.开头的变量)
```x86asm
 # options passed: -msse -mtune=core2 -march=nocona -Og -std=c99  
sum:  
    addsd    %xmm1, %xmm0     # tmp87, tmp85  
    ret       
main:  
    subq    $40, %rsp     #,  
    call    __main     #  
    movsd    .LC0(%rip), %xmm1     #,  
    movsd    .LC1(%rip), %xmm0     #,  
    call    sum     #  
    cvttsd2sil    %xmm0, %eax     # tmp87, <retval>  
    addq    $40, %rsp     #,  
    ret    
.LC0:  
    .long    858993459  
    .long    1075786547  
    .align 8  
.LC1:  
    .long    1374389535  
    .long    1074339512  
    .ident    "GCC: (x86_64-posix-seh, Built by MinGW-Builds project) 11.4.0"
```

CS:APP 3.11中像整数那样详细介绍了浮点代码,暂时略过不深入研究了.