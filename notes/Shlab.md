#csapp #csapplab
# 着手
Writeup告诉我,本实验可以两个人组队完成,而我当然只能自己完成咯^_^

本实验提供了一个shell框架(包括数据结构`job_t`),要让我自己编写几个关键函数:
- `eval`:执行命令行
- `builtin_cmd`:遇到内置函数(`quit`,`fg`,`bg`,`quit`)直接执行
- `do_bgfg`: 实施后台进程调度
- `waitfg`: 阻塞等待前台任务完成.
- `sigchld_handler`:捕获SIGCHILD信号
- `sigint_handler`: 捕获SIGINT(Ctrl-C)信号
- `sigstp_handler`: 捕获SIGTSP(Ctrl-Z)信号
实现由前台后台调度的tiny shell.
我没有着急开始写,而是先浏览了tsh.c的所有代码,熟悉了数据结构和help函数,惊叹于分割处理命令行参数的`pearseline()`函数,然后自顶向下开始思考(实际上是还没思路hhh),参考了别人的代码量[^1],想清楚了每个函数负责什么功能再动手.


[^1]:[CSAPP Lab5实验记录 ---- Shell Lab（实验分析 + 完整代码）](https://blog.csdn.net/qq_37500516/article/details/120836083)

测试方法:
```shell
make
make test*
make rtest*
```
将我的tsh与参考答案tshref对比,要求输出一致(处理PID).

# 程序设计
在我的tsh中运行的程序都是tsh程序的子进程,Linux默认fork子进程与父进程同属一个process group,这会导致发送信号`kill(-pid,sig)`会发送给整个process group,也就是说,这种情况下,如果我在前台Ctrl-Z/Ctrl-C,我的tsh进程也会暂停/终止,这不是我们期待的结果.解决方法也简单,writeup提示我在子进程中设置`setgpid(0, 0)`就可.
这样,tsh,fg,bg1,bg2,bg3各有不同的pid和gpid:
![[attachments/Pasted image 20250205152333.png|700]]
tsh的子进程,停止或终止都会给tsh发送SIGCHLD信号,触发`sigchld_handler`函数.

代码中穿插了大量系统调用判断,信号阻塞机制增强原子性,最坑的是这个`waitpid`函数的返回状态.
当我代码大体完成,逐个测试时,test16死活过不去,我真的无语...无奈看看别人的解法[^2][^3].谁能想到, `WIFSTOPPED(status)`与`WTERMSIG(status)==SIGTSTP`行为似乎不一样,一开始用后者,改为前者及其风格的`WIFEXITED(status)`这些就好了...


[^2]:[CSAPP Shell Lab 实验代码+16个test](https://blog.csdn.net/weixin_45739365/article/details/113527531)
[^3]:[CSAPP 之 Shell Lab](https://blog.liuly.moe/posts/csapp-shell)


Writeup提示我用`waitpid`的`WNOHANG`,`WUNTRACED`选项.我在
```shell
man 2 waitpid
```
找到了详细的解释:
```
 WNOHANG
              return immediately if no child has exited.

WUNTRACED
              also return if a child has stopped (but not traced via ptrace(2)).   Sta‐
              tus  for  traced children which have stopped is provided even if this op‐
              tion is not specified.

WCONTINUED (since Linux 2.6.10)
              also return if a stopped child has been resumed by delivery of SIGCONT.
```
还有`kill`函数的第一个参数,不同值具有不同意义,还算挺有意思的:
```
DESCRIPTION
       The kill() system call can be used to send any signal to any process group or process.

       If pid is positive, then signal sig is sent to the process with the ID specified by pid.

       If pid equals 0, then sig is sent to every process in the process group of the calling process.

       If  pid equals -1, then sig is sent to every process for which the calling process has permission to send
       signals, except for process 1 (init), but see below.

       If pid is less than -1, then sig is sent to every process in the process group whose ID is -pid.

       If sig is 0, then no signal is sent, but existence and permission checks are still performed; this can be
       used  to check for the existence of a process ID or process group ID that the caller is permitted to sig‐
       nal.
```

在`waitfg`函数中,Writeup居然推荐我循环内用`sleep`函数,课堂上不是说这样很浪费时间吗,我一度以为自己理解错了.我果断使用了课堂上提到的`sigsuspend`函数,提升了效率,算是一个改进吧.


写代码这种事情难以言说,去我的注释中体会吧...😋

# 结果
除了打印的字符串格式不同(这样才显得是我自己写的嘛)外,与`tshref`输出结果
**完全一致**(激动):
![[attachments/Pasted image 20250205143726.png]]
![[attachments/Pasted image 20250205143737.png]]
![[attachments/Pasted image 20250205144605.png]]
![[attachments/Pasted image 20250205144644.png]]
成功实现了一个*tiny shell*!

# 感受
本次实验的感受又与之前的实验有不同的感受.这次的感受就是:**混乱**!
虽然逻辑不难,但是代码中要注意的细节实在太多了!再过一段时间估计就看不懂为什么那样处理了...
信号机制错综复杂,gdb调试更没辙,看起来功能一致的不同的宏定义行为甚至不一样!只能一点点试错,参考别人使用的宏定义才改掉了一个离奇的bug.我游离在各种离奇的bug中,勉强修复了我发现的所有bug...
连一个半成品shell的代码都如此混乱,真的很难想象Linux内核居然是用C语言写成的!😨