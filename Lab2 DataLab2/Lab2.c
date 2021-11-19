/* CSED 211 Fall '2021.  Lab L2 */

#if 0
FLOATING POINT CODING RULES

For the problems that require you to implent floating - point operations,
the coding rules are less strict compared to the previous homework.
You are allowed to use loopingand conditional control.
You are allowed to use both intsand unsigneds.
You can use arbitrary integerand unsigned constants.

You are expressly forbidden to :
1. Define or use any macros.
2. Define any additional functions in this file.
3. Call any functions.
4. Use any form of casting.
5. Use any data type other than int or unsigned.This means that you
cannot use arrays, structs, or unions.
6. Use any floating point data types, operations, or constants.

#endif

/*
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
    unsigned exp = (uf & 0x7f800000) >> 23;
    unsigned frac = uf & 0x007fffff;
    if (exp == 0x000000ff && frac != 0) {
        return uf;
    }
    return uf + 0x80000000;
}
/*
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 4
 */
 
unsigned float_i2f(int x) {
    unsigned x_f;
    unsigned frac;
    unsigned round;
    int exp = 0;
    unsigned result;

    if (x >= 0) result = 0;
    else result = 1 << 31;
    if(x<0){
        x = -x;
    }
    x_f = x;

    while (x > 0) {
        x /= 2;
        exp = exp + 1;
    }
    exp -=  1;
    frac = x_f - (1 << exp);
    if(exp > 23){ //부동소수점에서 rounding 계산.
        frac = frac >> (exp -23);
        round = x_f & 0xFF;
        if((round > 128) || ((round == 128) && (frac & 1))){
            frac++;
            if(frac >> 23){
                exp++;
                frac = 0;
            }
        }
    }
    exp = 127 + exp;
    exp = exp << 23;

    result = result | exp | frac;
    return result;
}
/*
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 4
 */
 
unsigned float_twice(unsigned uf) {
    unsigned exp = (uf & 0x7f800000) >> 23;
    unsigned frac = uf & 0x007fffff;
    unsigned sign = uf & 0x10000000;
    if (exp == 0x000000ff) {//case NaN and infinity
        return uf;
    }
    exp = (exp + 1) << 23;
    return sign | exp | frac;
}

/*
 * float_abs - Return bit-level equivalent of absolute value of f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 2
 */
 
unsigned float_abs(unsigned uf) {
    unsigned exp = (uf & 0x7f800000) >> 23;
    unsigned frac = uf & 0x007fffff;
    unsigned sign = 0;
    if (exp == 0x000000ff && frac != 0) {
        return uf;
    }
    return sign | (exp << 23) | frac;
}

/*
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Rating: 4
 */
 
unsigned float_half(unsigned uf) {
    unsigned exp = (uf & 0x7f800000) >> 23;
    unsigned frac = uf & 0x007fffff;
    unsigned sign = uf & 0x10000000;
    if (exp == 0x000000ff) {
        return uf;
    }
    exp -= 1;
    return sign | (exp << 23) | frac;
}


