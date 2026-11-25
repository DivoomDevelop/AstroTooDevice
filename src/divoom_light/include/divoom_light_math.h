#ifndef __DIVOOM_MATH_H
#define	__DIVOOM_MATH_H


// #ifndef PI
// #define PI 3.1415926535897932384626433832795
// #endif

#define DIVOOM_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define DIVOOM_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define DIVOOM_SWAP_INT8(a, b) { int8 t = a; a = b; b = t; }

#define DIVOOM_ARRAY_LEN(array)   (sizeof(array) / sizeof(array[0]))


/**
 * @function:    divoom_math_abs
 * @description: 取得办入数的绝对值
 * @Author:      coronaa
 * @date:        2017-07-06
 * @param        n              [description]
 * @return                      [description]
 */
int8 divoom_math_abs(int8 n);
/**
 * @function:    divoom_math_cos_fixed_point
 * @description: 定点余弦函数，放大了1024倍
 * @Author:      coronaa
 * @date:        2017-07-06
 * @param        angle          [角度从原来的0-360变更为0-1024]
 * @return                      [返回值放大1024倍，范围-1024  ～  1024]
 */
int16 divoom_math_cos_fixed_point(int angle);

/**
 * @function:    divoom_math_sin_fixed_point
 * @description: 定点正弦函数，放大了1024倍
 * @Author:      coronaa
 * @date:        2017-07-06
 * @param        angle          [角度从原来的0-360变更为0-1024]
 * @return                      [返回值放大1024倍，范围-1024  ～  1024]
 */
int16 divoom_math_sin_fixed_point(int angle);


#endif
