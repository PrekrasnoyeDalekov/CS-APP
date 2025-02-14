#csapp #csapplab 

这是第一个实验.我还没开始写呢,先在datalab-handout文件夹下一使用make构建,立即报错:
``` shell
gcc -O1 -Wall -m32 -lm -o btest bits.c btest.c decl.c tests.c
In file included from btest.c:16:
/usr/include/stdio.h:27:10: fatal error: bits/libc-header-start.h: No such file or directory
   27 | #include <bits/libc-header-start.h>
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
In file included from decl.c:1:
/usr/include/stdio.h:27:10: fatal error: bits/libc-header-start.h: No such file or directory
   27 | #include <bits/libc-header-start.h>
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
In file included from /usr/lib/gcc/x86_64-linux-gnu/12/include/limits.h:203,
                 from /usr/lib/gcc/x86_64-linux-gnu/12/include/syslimits.h:7,
                 from /usr/lib/gcc/x86_64-linux-gnu/12/include/limits.h:34,
                 from tests.c:3:
/usr/include/limits.h:26:10: fatal error: bits/libc-header-start.h: No such file or directory
   26 | #include <bits/libc-header-start.h>
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
make: *** [Makefile:11: btest] Error 1
```
显然,这里缺少一个头文件.找到的解决办法是:
``` bash
sudo apt update
sudo apt-get install gcc-multilib
```

每次修改后,运行
``` bash
make && ./btest
```
会自动检查结果是否正确.至于其他文件夹,目前不了解是什么用处,可以查看README文件

我自己做了一下,真是疯狂!从没想过这些不用循环,控制语句的算法!也用上了一些逻辑学运算的知识才能勉强解决!

网上的答案中,这份写的不错:
[CSAPP | Lab1-Data Lab 深入解析 - 知乎](https://zhuanlan.zhihu.com/p/472188244)

终于成功写完!非常有成就感!也让我意识到自己的不足.这个实验做了很久,可能有10小时.我感觉我的思维从来没有来到过这一层.看似简单的操作,需要考虑的情况非常繁杂.CMU不愧是名牌大学,教学质量真的狠,不是平庸之人能随意学会的.
第一个实验就如此丰富,下个实验更是大名鼎鼎的bomblab,非常期待!

最后欣赏一下全部pass的结果:

``` bash
kim@MyDell:/mnt/e/Documents/C/CSAPP/datalab-handout$ make && ./btest
gcc -O1 -Wall -m32 -lm -o btest bits.c btest.c decl.c tests.c
btest.c: In function ‘test_function’:
btest.c:334:23: warning: ‘arg_test_range’ may be used uninitialized [-Wmaybe-uninitialize ]
  334 |     if (arg_test_range[2] < 1)
      |         ~~~~~~~~~~~~~~^~~
btest.c:299:9: note: ‘arg_test_range’ declared here
  299 |     int arg_test_range[3]; /* test range for each argument */
      |         ^~~~~~~~~~~~~~
Score   Rating  Errors  Function
 1      1       0       bitXor
 1      1       0       tmin
 2      2       0       isTmax
 2      2       0       allOddBits
 2      2       0       negate
 3      3       0       isAsciiDigit
 3      3       0       conditional
 3      3       0       isLessOrEqual
 4      4       0       logicalNeg
 4      4       0       howManyBits
 4      4       0       float_twice
 4      4       0       float_i2f
 4      4       0       float_f2i
Total points: 37/37
```
