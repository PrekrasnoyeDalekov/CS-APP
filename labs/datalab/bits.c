/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
	// 提出x,y中的0和1位
	// x1 = x & 0xffffffff
	// x0 = ~x1
	// y1 = y & 0xffffffff
	// y0 = ~y1
	// not (a and b) = not a or not b,so not(not a and not b) = a or b
	// result = ~(~(x & ~y) & ~(~x & y));
	return ~(~(x & ~y) & ~(~x & y));
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1<<(sizeof(int)*8-1);
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 2
 */
int isTmax(int x) {
  return !((x+x+2)|!(x+1));
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
	int odd_numbered = 0xAA+(0xAA<<8)+(0xAA<<16)+(0xAA<<24);
	// 如果a=b,那么a^b=0
  return !((x&odd_numbered)^odd_numbered);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  return !(((x+(~0x30+1))>>31) | ((0x39+(~x+1))>>31));
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
	int boo = !(!x);
	boo = ~boo+1;
  return (y&boo)+(z&(~boo));
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  return !((y+(~x+1))>>31);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
	int boo = ~((x+(~0x1)+1)>>31) | ~((~x)>>31);  // 这里不能使用^.逻辑上应使用^,但TMin过不了
  return (0&boo)+(1&~boo);
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
	int flag = x >> 31;
	x = ((~flag) & x) | (flag & (~x));  // 如果x是负数,返回~x;如果x是正数,返回x.巧妙之处在于对于负数,除去符号位需要的最高位其实是0,所以取反后与正数最高位1一致.(11101)_2=(101)_2=(-3)_10
	int bit_16 = (!!(x >> 16)) << 4;   //  // 如果高16位非0,那么结果是0x10;如果高16位都是0,那么结果是0x0.
	x = x >> bit_16;
	int bit_8 = !!(x>>8)<<3;
	x = x >> bit_8;
	int bit_4 = !!(x >> 4) << 2;
	x = x >> bit_4;
	int bit_2 = !!(x >> 2) << 1;
	x = x >> bit_2;
	int bit_1 = !!(x >> 1);
	x = x >> bit_1;
	int bit_0 = x;
	return bit_16+bit_8+bit_4+bit_2+bit_1+bit_0+1;  // 1 是符号位
	
}
//float
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
	if(!((uf<<1>>24)^(0xff))) return uf;// NaN和inf
	else if(!((uf<<1>>24)^0x0)) return (uf>>23<<23)+(uf<<9>>8);  // 非规格化的值
	else {
		if(uf<<1>>24 == 0xfe) return (uf+(0x1<<23))>>23<<23; // 溢出为无穷
		else return (uf+(0x1<<23));
	}  // 规格化的值

}



/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
	if(!x) return 0;
	if(x==-2147483648) return 0xcf000000;
	unsigned s = x>>31;// x的符号
	int x_abs=s?(~x+1):x;
	unsigned char i=0;
	double power = 1;
	int x1=x_abs;
	while(x1/2){
		i++;
		x1/=2;
		power*=2;
	}// 循环结束后,i是满足2^i<=x<2^{i+1},power=2^i.
	unsigned char exp = i+127;
	unsigned frac = 0;
	unsigned char cnt= 23;// 23bits计数
	int x2=x_abs;
	x2-=power;
	while(cnt>0){
		cnt--;
		power/=2;
		if(x2>=power) {
			frac |= 1<<cnt;
			x2-=power;
		}
		if(!x2) break;  // 整除情况下直接退出
		// 如果23位不能整除要考虑舍入!
		// 再计算两位,与最低位结合起来考虑
		// >*10进位,<*10舍去不用处理,=*1000进位,010舍去不用处理
	}
	if(!cnt){
		power/=2;
		if((x2==power && (frac&0x1))||x2>power)
			frac+=1;
	}
	return 	(s<<31 | exp<<23 )+ frac;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
	unsigned s = uf>>31;
	unsigned exp = uf<<1>>24;
	int E = exp-127;
	unsigned frac = uf<<9>>9 | 1<<23;
	if(E<0) return 0;  // 绝对值小于1的数
	if(!((uf<<1>>24)^0xff) || (!s&&E>30) || (s&&frac&&(E==31)) || (s&&E>31)) return 1<<31;//NaN,inf和溢出的情况
	else {
		if(E<23)
			return s? (s<<31 | (~(frac>>(23-E))+1)) : (s<<31 | frac>>(23-E));
		else
			return s? (s<<31 | (~(frac>>(E-23))+1)) : (s<<31 | frac<<(E-23));
	}
	
}


