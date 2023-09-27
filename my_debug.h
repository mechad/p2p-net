#ifndef MY_DEBUG_H
#define MY_DEBUG_H

//#include <stdio.h>
#include <libgen.h>
// 模块功能号
enum {
    MY_SECTION_ALL  = 0,
    MY_SECTION_MAIN,
    MY_SECTION_END
};

enum {
    DEBUG_LEVEL_ALL= 0,
    DEBUG_LEVEL_DEBUG,      //调试信息基本包含所有信息
    DEBUG_LEVEL_INFO,   //主要包含程序主要流程、分支运行情况信息
    DEBUG_LEVEL_WARNING,
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_NONE,
};

/*    
 * global variants
 */
extern int __my_allow_debug_levels[MY_SECTION_END];extern void printDateTime(void);void printBuf(char * disp, char * buf, int len);

#define OFFSET_OF(obj_type,mb)  ((int)&(((obj_type*)0)->mb))


    // (内部使用) 判断"SECTION"模块功能号是否允许"DEBUG_LEVEL"等级的调试信息输出
#define __my_unallow_debug(SECTION, DEBUG_LEVEL) \
     ( DEBUG_LEVEL < __my_allow_debug_levels[SECTION] )
    // (内部使用) 调试信息输出函数
#define __my_debug(FORMAT, ARG...) \
     printf("%s:%d %s():" FORMAT, basename(__FILE__), __LINE__, __FUNCTION__, ##ARG)
    
    // 初始化"SECTION"模块功能号的调试等级
#define my_init_debug_levels(SECTION, ALLOW_DEBUG_LEVEL) \
     ( __my_allow_debug_levels[SECTION<MY_SECTION_END?SECTION:MY_SECTION_END] = ALLOW_DEBUG_LEVEL )
    
    // 调试信息输出函数，该信息为"SECTION"模块功能号"DEBUG_LEVEL"等级的调试信息
#define my_debug(SECTION, DEBUG_LEVEL) \
            ( __my_unallow_debug(SECTION, DEBUG_LEVEL) ) ? (void) 0 : __my_debug
    
    // 调试信息输出函数，该信息为"SECTION"模块功能号"DEBUG_LEVEL"等级的调试信息
#define my_debug_print_buf(SECTION, DEBUG_LEVEL) \
            ( __my_unallow_debug(SECTION, DEBUG_LEVEL) ) ? (void) 0 : printBuf
void _print_debug_array( unsigned char section, const unsigned char s[],int cnt, char *fmt);
#define print_debug_array( cection,tip,s,cnt,fmt ) \
    do{ \
        my_debug(cection,DEBUG_LEVEL_DEBUG)(tip);\
        _print_debug_array(cection,s,cnt,fmt);\
    }while(0)
/////////////////////////////////////////////////////
//BCD码转10进制
#define BCDtoDec( var ) (var)>0x99?0:((var)-((var) >> 4)*6)
//10进制转BCD码
#define DectoBCD( var ) (var)>99?0:((var)+((var)/10)*6)
//HEX码流转ascll码流,len 指HEX码的字节数
#define HEXtoAscii( HEX,ASCII,len )\
        {\
            unsigned char *p = (unsigned char*)HEX;\
            unsigned char *out = (unsigned char*)ASCII;\
            int i=0;\
            do{\
                if((p[i]>>4) > 9) {(out[i*2])=(p[i]>>4) + '0' + 7;}\
                else {out[i*2]=(p[i]>>4)+'0';}\
                if((p[i]&0x0f) >9) {(out[i*2+1])=(p[i]&0xf) + '0' + 7;}\
                else {out[i*2+1]=(p[i]&0x0f)+'0';}\
            }while( ++i<len);\
        }

//ascii 转HEX码
//len 指ASCII字符串的长度
#define AsciitoHEX( ASCII, HEX, Len )\
    {\
        int i;\
        unsigned char *hex = (unsigned char *)HEX;\
        unsigned char *asc = (unsigned char *)ASCII;\
        unsigned char c1, c2;\
        for(i=0; i<Len; i+=2)\
        {\
            c1 = asc[i]-'0';\
            if( c1 > 9 )    {c1=c1-7;}\
                c2 = asc[i+1]-'0';\
            if( c2 > 9 )    {c2=c2-7;}\
                hex[i/2] = (( c1<<4 ) | ( c2&0x0f ));\
        }\
    }
#define SmalltoBig( src ,dst ,len )\
        {\
            unsigned char *psrc = (unsigned char*)src;\
            unsigned char *pdst = (unsigned char*)dst;\
            int i=0;\
            for(;i<len;i++)\
            {\
                pdst[i] = psrc[len - 1 - i];\
            }\
        }


typedef struct SYSTEM_TIME
{
    int year;
    unsigned char month;
    unsigned char day;
    unsigned char week;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
}__attribute__( (packed, aligned(1)) )SYSTEM_TIME;

char code2index( char c );
//延时n秒
int my_sleep( int sec );
//延时useconds 微秒
int my_usleep(int usec );

//得到本地时间
void get_time( SYSTEM_TIME *tm );
int mystrtime( char *timebuf ,char* formt );
/*
 * 标准输出重定向
 * 如果检测到/tiandao/stdout.txt文件存在，则将标准输出重定向该该文件
 * 如果检测到该文件不存在则再重定向回标准输出
 * 该函数为非线程安全函数
 */
int out_redirect( const char *path_name );
//检查自上一次调用该函数后指定文件是否被修改过
int check_file_modify_time( char *filename );
//打印当前时间，调试时使用，规范日志格式
void print_date( int module, int level );

//获取系统启动运行时间
int getstarttime( void );
void setshutdowntime(void);
void getshutdowntime( unsigned char *bcdtime );

/*
extern int debuglevels;

void printBuf(char * disp, char * buf, int len);

// (内部使用) 判断"SECTION"模块功能号是否允许"DEBUG_LEVEL"等级的调试信息输出
#define __my_unallow_debug( DEBUG_LEVEL) \
 ( DEBUG_LEVEL < debuglevels)

// (内部使用) 调试信息输出函数
#define __my_debug(FORMAT, ARG...) \
 printf("%s:%d %s(): " FORMAT, basename(__FILE__), __LINE__, __FUNCTION__, ##ARG)

// 初始化调试等级
#define my_init_debug_levels( ALLOW_DEBUG_LEVEL) \
 ( debuglevels = ALLOW_DEBUG_LEVEL )

// 调试信息输出函数
#define my_debug( DEBUG_LEVEL) \
        ( __my_unallow_debug( DEBUG_LEVEL) ) ? (void) 0 : __my_debug

// 调试信息输出函数
#define my_debug_print_buf( DEBUG_LEVEL) \
        ( __my_unallow_debug( DEBUG_LEVEL) ) ? (void) 0 : printBuf

//void _print_debug_array( char *tip, unsigned char s[],int cnt, char *fmt);
void _print_debug_array( unsigned char s[],int cnt, char *fmt);
#define print_debug_array( tip,s,cnt,fmt ) \
do{ \
    my_debug(DEBUG_LEVEL_DEBUG)(tip);\
    _print_debug_array(s,cnt,fmt);\
}while(0)

*/
#endif //MY_DEBUG_H


