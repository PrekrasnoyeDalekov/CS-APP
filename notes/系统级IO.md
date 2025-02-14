#csapp 
# Linux I/O总览
Linux文件是字节的序列:$B_{0},B_{1},\dots,B_{k},\dots,B_{m-1}$
- 一切都以文件形式展现:
	- 硬盘: /dev/sda2
	- 终端: /dev/tty2
	- 内核镜像: /boot/vmlinuz-3.13.0-55-generic
	- 内核数据结构: /proc
- Unix I/O函数:
	- `open()`和`close()`
	- `read()`和`write()`
	- `lseek()`:当前文件的相对偏移量
		![[attachments/Pasted image 20250203224007.png|500]]
- 文件类型
	- 常规文件:包括任意数据
	- 目录:相关文件组的索引,描述其他文件的位置和索引
	- 套接字:网络编程
	- 管道
	- 链接
	- 设备

本课程讨论前三种.

## 常规文件
常规文件包含任意数据.
操作系统不管文件内部的内容;有些应用程序区分二进制文件和文本文件.
文本文件只有ASCII或Unicode字符;其他属二进制文件.
不同系统中的换行符:
- Linux和Mac OS: '\n'(0xa)   LF
- Windows和网络协议: '\r\n'(0xd 0xa)   CRLF
## 目录
目录包括一个链接的数组.
每个目录至少包括两个元素: .(自身的链接) 和 ..(父目录的链接)
操作目录的命令:
- `mkdir`:创建空目录
- `ls`:浏览目录内容
- `rmdir`:删除空目录

目录层次结构:
![[attachments/Pasted image 20250203232107.png|700]]

## I/O函数
我在网络编程的实践中已经接触过Linux的I/O函数,以及Linux一切皆文件的特点.对以下C函数已经熟悉.
1. `open`函数
2. `close`函数
3. `read`函数
4. `write`函数
Linux shell创建的进程都有三个初始的**文件描述符**:
- 0: stdin
- 1: stdout
- 2: stderr
之后创建的文件描述符从3开始增加,有上限.

# 文件元数据(Metadata)
stat数据结构:
```C
/* Metadata returned by the stat and fstat functions */ 
struct stat { 
	dev_t st_dev; /* Device */ 
	ino_t st_ino; /* inode */ 
	mode_t st_mode; /* Protection and file type */ 
	nlink_t st_nlink; /* Number of hard links */ 
	uid_t st_uid; /* User ID of owner */ 
	gid_t st_gid; /* Group ID of owner */ 
	dev_t st_rdev; /* Device type (if inode device) */ 
	off_t st_size; /* Total size, in bytes */ 
	unsigned long st_blksize; /* Blocksize for filesystem I/O */ 
	unsigned long st_blocks; /* Number of blocks allocated */ 
	time_t st_atime; /* Time of last access */ 
	time_t st_mtime; /* Time of last modification */ 
	time_t st_ctime; /* Time of last change */ 
};
```
> [!note]
>我发现Linux中一个好用的命令工具`man`:
> ```shell
> sudo apt upgrade
> sudo apt-get install man-db
> ```
> 然后就可以通过`man`查看各种命令,函数,十分详细!
> 例如,
> ```
> man 2 stat
> ````
> 会出现帮助文档:
> ![[attachments/Pasted image 20250204004534.png|500]]

# 打开文件的表示
每一个进程都有一个描述符表(Descriptor table),打开某个文件后,该表相应元素的指针指向该文件的数据结构.该数据结构是打开文件表(Open file table)的一个元素,由操作系统进行全局维护.
打开文件表的一个元素包含了该文件的信息:File pos是最后一次读写操作的位置;打开文件可以被所有进程共享,所以引用计数(refcnt)表明它当前的引用次数.v-node是文件的stat.
- 两个文件描述符引用两个不同的文件:
	![[attachments/Pasted image 20250204110451.png|700]]
- 两个文件描述符打开打开同一个文件:
	![[attachments/Pasted image 20250204111218.png|700]]
- `fork`创建子进程,共享open file table
	每个refcnt加1
	![[attachments/Pasted image 20250204111606.png|700]]

# I/O重定向
```C
#include <unistd.h>
int dup2(int oldfd, int newfd);
```
通过调用`dup2`函数,复制文件描述符来实现重定向.
![[attachments/Pasted image 20250204112244.png|500]]
![[attachments/Pasted image 20250204113313.png|500]]
在shell中,` < ` 是输入重定向; ` > ` 是输出重定向.
> 在Attacklab中,我已经用过
> ```shell
> ./hex2raw < exploit.txt > exploit-raw.txt
> ```
> 这就是将 `hex2raw`程序的输入重定向为 `exploit.txt`,输出重定向为 `exploit-raw.txt`

>[!think]
>我记得在网络编程的实践中,父进程`close`文件描述符,子进程的该文件还在,必须子进程也`close`才是彻底关闭.而`dup2`减少了oldfd的refcnt,却似乎不会关闭文件.它们的具体实现是如何操作descriptor table 和 open file table的呢?
## 引发复杂的分析
假设fname文件的内容是"abcde",以下程序的输出结果分别是什么?
```C
/* ffiles1.c */
#include "csapp.h" 
int main(int argc, char *argv[]) { 
	int fd1, fd2, fd3; 
	char c1, c2, c3; 
	char *fname = argv[1]; 
	fd1 = Open(fname, O_RDONLY, 0); 
	fd2 = Open(fname, O_RDONLY, 0); 
	fd3 = Open(fname, O_RDONLY, 0); 
	Dup2(fd2, fd3); 
	Read(fd1, &c1, 1); 
	Read(fd2, &c2, 1); 
	Read(fd3, &c3, 1); 
	printf("c1 = %c, c2 = %c, c3 = %c\n", c1, c2, c3); 
	return 0; 
}

/* ffiles2.c */
#include "csapp.h" 
int main(int argc, char *argv[]) { 
	int fd1; 
	int s = getpid() & 0x1; 
	char c1, c2; 
	char *fname = argv[1]; 
	fd1 = Open(fname, O_RDONLY, 0); 
	Read(fd1, &c1, 1); 
	if (fork()) { /* Parent */ 
		sleep(s); Read(fd1, &c2, 1); 
		printf("Parent: c1 = %c, c2 = %c\n", c1, c2); 
	} else { /* Child */ 
		sleep(1-s); 
		Read(fd1, &c2, 1); 
		printf("Child: c1 = %c, c2 = %c\n", c1, c2); 
	} 
	return 0; 
}

```
fname的内容是什么?
```C
#include "csapp.h" 
int main(int argc, char *argv[]) { 
	int fd1, fd2, fd3; 
	char *fname = argv[1]; 
	fd1 = Open(fname, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR); 
	Write(fd1, "pqrs", 4); 
	fd3 = Open(fname, O_APPEND|O_WRONLY, 0); 
	Write(fd3, "jklmn", 5); 
	fd2 = dup(fd1); /* Allocates descriptor */ 
	Write(fd2, "wxyz", 4); 
	Write(fd3, "ef", 2); 
	return 0; 
}
```
遍历获取目录下的文件名:
```C
#include <sys/types.h>
#include <dirent.h>
{ 
	DIR *directory; 
	struct dirent *de; 
	... 
	if (!(directory = opendir(dir_name))) 
		error("Failed to open directory"); 
	... 
	while (0 != (de = readdir(directory)))
		printf("Found file: %s\n", de->d_name);
	... 
	closedir(directory); 
}
```



# 标准I/O函数
- 打开/关闭文件: `fopen`&`fclose`
- 读/写字节流: `fread`&`fwrite`
- 读/写行文本: `fgets`&`fputs`
- 格式化输入/输出: `fscanf`&`fprintf`
这些标准I/O函数都有缓冲区

## Unix I/O,标准I/O和自定义I/O函数
![[attachments/Pasted image 20250204114508.png|600]]
通过自定义I/O函数,满足个性化需求.
根据不同场景,选择不同的I/O函数.




---
> 推荐阅读:《Unix环境高级编程》

