(gdb) disass /r 0x401994,0x401ab7
Dump of assembler code from 0x401994 to 0x401ab7:
   0x0000000000401994 <start_farm+0>:   b8 01 00 00 00          mov    $0x1,%eax
   0x0000000000401999 <start_farm+5>:   c3                      ret
   0x000000000040199a <getval_142+0>:   b8 fb 78 90 90          mov    $0x909078fb,%eax
   0x000000000040199f <getval_142+5>:   c3                      ret
   0x00000000004019a0 <addval_273+0>:   8d 87 48 89 c7 c3       lea    -0x3c3876b8(%rdi),%eax
# 48 89 c7 c3 :		movq	%rax,%rdi;ret
   0x00000000004019a6 <addval_273+6>:   c3                      ret
   0x00000000004019a7 <addval_219+0>:   8d 87 51 73 58 90       lea    -0x6fa78caf(%rdi),%eax
# 58 90 c3 :	pop		%rax;ret
   0x00000000004019ad <addval_219+6>:   c3                      ret
   0x00000000004019ae <setval_237+0>:   c7 07 48 89 c7 c7       movl   $0xc7c78948,(%rdi)
   0x00000000004019b4 <setval_237+6>:   c3                      ret
   0x00000000004019b5 <setval_424+0>:   c7 07 54 c2 58 92       movl   $0x9258c254,(%rdi)
   0x00000000004019bb <setval_424+6>:   c3                      ret
   0x00000000004019bc <setval_470+0>:   c7 07 63 48 8d c7       movl   $0xc78d4863,(%rdi)
   0x00000000004019c2 <setval_470+6>:   c3                      ret
   0x00000000004019c3 <setval_426+0>:   c7 07 48 89 c7 90       movl   $0x90c78948,(%rdi)
#   	movq	%rax,%rdi;ret
   0x00000000004019c9 <setval_426+6>:   c3                      ret
   0x00000000004019ca <getval_280+0>:   b8 29 58 90 c3          mov    $0xc3905829,%eax
# 58 90 c3 :	pop		%rax;ret
   0x00000000004019cf <getval_280+5>:   c3                      ret
   0x00000000004019d0 <mid_farm+0>:     b8 01 00 00 00          mov    $0x1,%eax
   0x00000000004019d5 <mid_farm+5>:     c3                      ret
   0x00000000004019d6 <add_xy+0>:       48 8d 04 37             lea    (%rdi,%rsi,1),%rax
   0x00000000004019da <add_xy+4>:       c3                      ret
   0x00000000004019db <getval_481+0>:   b8 5c 89 c2 90          mov    $0x90c2895c,%eax
# 89 c2 90 c3 :		movl	%eax,%edx;ret
   0x00000000004019e0 <getval_481+5>:   c3                      ret
   0x00000000004019e1 <setval_296+0>:   c7 07 99 d1 90 90       movl   $0x9090d199,(%rdi)
   0x00000000004019e7 <setval_296+6>:   c3                      ret
   0x00000000004019e8 <addval_113+0>:   8d 87 89 ce 78 c9       lea    -0x36873177(%rdi),%eax
   0x00000000004019ee <addval_113+6>:   c3                      ret
--Type <RET> for more, q to quit, c to continue without paging--c
   0x00000000004019ef <addval_490+0>:   8d 87 8d d1 20 db       lea    -0x24df2e73(%rdi),%eax
# 20 db c3:		andb	%bl,%bl;ret
   0x00000000004019f5 <addval_490+6>:   c3                      ret
   0x00000000004019f6 <getval_226+0>:   b8 89 d1 48 c0          mov    $0xc048d189,%eax
   0x00000000004019fb <getval_226+5>:   c3                      ret
   0x00000000004019fc <setval_384+0>:   c7 07 81 d1 84 c0       movl   $0xc084d181,(%rdi)
# 84 c0 c3:		testb	%al,%al;ret
   0x0000000000401a02 <setval_384+6>:   c3                      ret
   0x0000000000401a03 <addval_190+0>:   8d 87 41 48 89 e0       lea    -0x1f76b7bf(%rdi),%eax
# 48 89 e0 c3:	movq	 %rsp,%rax;ret
   0x0000000000401a09 <addval_190+6>:   c3                      ret
   0x0000000000401a0a <setval_276+0>:   c7 07 88 c2 08 c9       movl   $0xc908c288,(%rdi)
# 08 c9 c3:		orb		%cl,%cl;ret
   0x0000000000401a10 <setval_276+6>:   c3                      ret
   0x0000000000401a11 <addval_436+0>:   8d 87 89 ce 90 90       lea    -0x6f6f3177(%rdi),%eax
# 89 ce c3:		movl	%ecx,%esi
   0x0000000000401a17 <addval_436+6>:   c3                      ret
   0x0000000000401a18 <getval_345+0>:   b8 48 89 e0 c1          mov    $0xc1e08948,%eax
   0x0000000000401a1d <getval_345+5>:   c3                      ret
   0x0000000000401a1e <addval_479+0>:   8d 87 89 c2 00 c9       lea    -0x36ff3d77(%rdi),%eax
   0x0000000000401a24 <addval_479+6>:   c3                      ret
   0x0000000000401a25 <addval_187+0>:   8d 87 89 ce 38 c0       lea    -0x3fc73177(%rdi),%eax
# 38 c0 c3:		cmpb	%al,%al;ret
# 89 ce c3:		movl	%ecx,%esi;ret
   0x0000000000401a2b <addval_187+6>:   c3                      ret
   0x0000000000401a2c <setval_248+0>:   c7 07 81 ce 08 db       movl   $0xdb08ce81,(%rdi)
# 08 db c3:		orb		%bl,%bl;ret
   0x0000000000401a32 <setval_248+6>:   c3                      ret
   0x0000000000401a33 <getval_159+0>:   b8 89 d1 38 c9          mov    $0xc938d189,%eax
# 38 c9 c3:		cmpb	%cl,cl;ret
# 89 d1 c3:		movl	%eDx,%ecx
   0x0000000000401a38 <getval_159+5>:   c3                      ret
   0x0000000000401a39 <addval_110+0>:   8d 87 c8 89 e0 c3       lea    -0x3c1f7638(%rdi),%eax
# 89 e0 c3:		movl	%esp,%eax;ret
   0x0000000000401a3f <addval_110+6>:   c3                      ret
   0x0000000000401a40 <addval_487+0>:   8d 87 89 c2 84 c0       lea    -0x3f7b3d77(%rdi),%eax
# 84 c0 c3:		testb	%al,%al;ret
# 89 c2 c3:		movl
   0x0000000000401a46 <addval_487+6>:   c3                      ret
   0x0000000000401a47 <addval_201+0>:   8d 87 48 89 e0 c7       lea    -0x381f76b8(%rdi),%eax
   0x0000000000401a4d <addval_201+6>:   c3                      ret
   0x0000000000401a4e <getval_272+0>:   b8 99 d1 08 d2          mov    $0xd208d199,%eax
# 08 d2 c3:		orb		%dl,%dl;ret
   0x0000000000401a53 <getval_272+5>:   c3                      ret
   0x0000000000401a54 <getval_155+0>:   b8 89 c2 c4 c9          mov    $0xc9c4c289,%eax
   0x0000000000401a59 <getval_155+5>:   c3                      ret
   0x0000000000401a5a <setval_299+0>:   c7 07 48 89 e0 91       movl   $0x91e08948,(%rdi)
   0x0000000000401a60 <setval_299+6>:   c3                      ret
   0x0000000000401a61 <addval_404+0>:   8d 87 89 ce 92 c3       lea    -0x3c6d3177(%rdi),%eax
   0x0000000000401a67 <addval_404+6>:   c3                      ret
   0x0000000000401a68 <getval_311+0>:   b8 89 d1 08 db          mov    $0xdb08d189,%eax
# 08 db c3:		orb		%bl,%bl;ret
# 89 d1 c3:		movl	%edx,%ecx
   0x0000000000401a6d <getval_311+5>:   c3                      ret
   0x0000000000401a6e <setval_167+0>:   c7 07 89 d1 91 c3       movl   $0xc391d189,(%rdi)
   0x0000000000401a74 <setval_167+6>:   c3                      ret
   0x0000000000401a75 <setval_328+0>:   c7 07 81 c2 38 d2       movl   $0xd238c281,(%rdi)
# 38 d2 c3:		cmpb	%dl,%dl;ret
   0x0000000000401a7b <setval_328+6>:   c3                      ret
   0x0000000000401a7c <setval_450+0>:   c7 07 09 ce 08 c9       movl   $0xc908ce09,(%rdi)
# 08 c9 c3:		orb		%cl,%cl;ret
   0x0000000000401a82 <setval_450+6>:   c3                      ret
   0x0000000000401a83 <addval_358+0>:   8d 87 08 89 e0 90       lea    -0x6f1f76f8(%rdi),%eax
# 89 e0 c3:		movl	%esp,%eax;ret
   0x0000000000401a89 <addval_358+6>:   c3                      ret
   0x0000000000401a8a <addval_124+0>:   8d 87 89 c2 c7 3c       lea    0x3cc7c289(%rdi),%eax
   0x0000000000401a90 <addval_124+6>:   c3                      ret
   0x0000000000401a91 <getval_169+0>:   b8 88 ce 20 c0          mov    $0xc020ce88,%eax
# 20 c0 c3:		andl	%al,%al;ret
   0x0000000000401a96 <getval_169+5>:   c3                      ret
   0x0000000000401a97 <setval_181+0>:   c7 07 48 89 e0 c2       movl   $0xc2e08948,(%rdi)
   0x0000000000401a9d <setval_181+6>:   c3                      ret
   0x0000000000401a9e <addval_184+0>:   8d 87 89 c2 60 d2       lea    -0x2d9f3d77(%rdi),%eax
   0x0000000000401aa4 <addval_184+6>:   c3                      ret
   0x0000000000401aa5 <getval_472+0>:   b8 8d ce 20 d2          mov    $0xd220ce8d,%eax
# 20 d2 c3:		andl	%dl,%dl;ret
   0x0000000000401aaa <getval_472+5>:   c3                      ret
   0x0000000000401aab <setval_350+0>:   c7 07 48 89 e0 90       movl   $0x90e08948,(%rdi)
# 48 89 e0 c3:		movl	%rsp,%rax;ret
   0x0000000000401ab1 <setval_350+6>:   c3                      ret
   0x0000000000401ab2 <end_farm+0>:     b8 01 00 00 00          mov    $0x1,%eax
End of assembler dump.
