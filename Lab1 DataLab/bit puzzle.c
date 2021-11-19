/* CSED 211 Fall '2021.  Lab L1 */

#if 0
LAB L1 INSTRUCTIONS:

#endif

/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 */
int bitAnd(int x, int y)
{
    return ~(~x | ~y);
}

/* 
 * addOK - Determine if can compute x+y without overflow
 *   Example: addOK(0x80000000,0x80000000) = 0,
 *            addOK(0x80000000,0x70000000) = 1, 
 *   Legal ops: ! ~ & ^ | + << >>
 */
int addOK(int x, int y)
{
    return !!(x >> 31 ^ y >> 31) | (!(x >> 31 ^ y >> 31) & !(y >> 31 ^ (x + y) >> 31));
}

/* 
 * isNegative - return 1 if x < 0, return 0 otherwise 
 *   Example: isNegative(-1) = 1.   
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNegative(int x)
{
    int y = x >> 31;
    return !(~y);
}

/* 
 * logicalShift - logical right shift of x by y bits, 1 <= y <= 31
 *   Example: logicalShift(-1, 1) = TMax.
 *   Legal ops: ! ~ & ^ | + << >>
 */
int logicalShift(int x, int y)
{
    int z = x >> y;
    x = x >> y;
    z = z >> 31;
    z = z << 32 - y;
    return x - z;
}

/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max operation: 40
 */
int bitCount(int x)
{
    int check = (1 << 24) + (1<< 16) + (1 << 8) + 1;
    int cnt = (x&check) + (x>>1 & check) + (x>>2 & check) + (x>>3 & check) + (x>>4 & check) + (x>>5 & check) + (x>>6 & check) + (x>>7 & check);
    return (cnt>>24) + ((cnt<<8)>>24) + ((cnt<<16)>>24) + ((cnt<<24)>>24);
}
int main(){
    printf("%d\n", bitCount(0xffffffff));

    return 0;
}