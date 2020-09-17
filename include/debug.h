# ifndef __DEBUG_H__
#define __DEBUG_H__
void panic_spin(char *filename,int line,const char*func,const char*condition);

/**
*__VA_ARGS__是预处理器支持的专用标志符号
*代表所有省略号相对应的参数
*"..."表示定义的宏参数可变
*/
#define PANIC(...) panic_spin (__FILE__,__LINE__,__func__,__VA_ARGS__) //__FILE__ __LINE__ __func__是编译器预定义的宏
#ifdef NDBUG                            /*gcc 编译参数指定-D NDBUG不是debug模式，替换成空*/
    #define ASSERT(CONDITION)((void)0)  /*让ASSERT成为空0，相当于删除ASSERT*/
#else
#define ASSERT(CONDITION) \
if(CONDITION){} else{   \
    /*符号#让编译器将参数转化为字符串字面量*/ \
    PANIC(#CONDITION); \
} 
#endif  /*NDBUG*/

#endif  /*__DEBUG_H__*/
