#ifndef MY_DEBUG_H
#define MY_DEBUG_H

//#include <stdio.h>
#include <libgen.h>
// ģ�鹦�ܺ�
enum {
    MY_SECTION_ALL  = 0,
    MY_SECTION_MAIN,
    MY_SECTION_END
};

enum {
    DEBUG_LEVEL_ALL= 0,
    DEBUG_LEVEL_DEBUG,      //������Ϣ��������������Ϣ
    DEBUG_LEVEL_INFO,   //��Ҫ����������Ҫ���̡���֧���������Ϣ
    DEBUG_LEVEL_WARNING,
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_NONE,
};

/*    
 * global variants
 */
extern int __my_allow_debug_levels[MY_SECTION_END];extern void printDateTime(void);void printBuf(char * disp, char * buf, int len);

#define OFFSET_OF(obj_type,mb)  ((int)&(((obj_type*)0)->mb))


    // (�ڲ�ʹ��) �ж�"SECTION"ģ�鹦�ܺ��Ƿ�����"DEBUG_LEVEL"�ȼ��ĵ�����Ϣ���
#define __my_unallow_debug(SECTION, DEBUG_LEVEL) \
     ( DEBUG_LEVEL < __my_allow_debug_levels[SECTION] )
    // (�ڲ�ʹ��) ������Ϣ�������
#define __my_debug(FORMAT, ARG...) \
     printf("%s:%d %s():" FORMAT, basename(__FILE__), __LINE__, __FUNCTION__, ##ARG)
    
    // ��ʼ��"SECTION"ģ�鹦�ܺŵĵ��Եȼ�
#define my_init_debug_levels(SECTION, ALLOW_DEBUG_LEVEL) \
     ( __my_allow_debug_levels[SECTION<MY_SECTION_END?SECTION:MY_SECTION_END] = ALLOW_DEBUG_LEVEL )
    
    // ������Ϣ�������������ϢΪ"SECTION"ģ�鹦�ܺ�"DEBUG_LEVEL"�ȼ��ĵ�����Ϣ
#define my_debug(SECTION, DEBUG_LEVEL) \
            ( __my_unallow_debug(SECTION, DEBUG_LEVEL) ) ? (void) 0 : __my_debug
    
    // ������Ϣ�������������ϢΪ"SECTION"ģ�鹦�ܺ�"DEBUG_LEVEL"�ȼ��ĵ�����Ϣ
#define my_debug_print_buf(SECTION, DEBUG_LEVEL) \
            ( __my_unallow_debug(SECTION, DEBUG_LEVEL) ) ? (void) 0 : printBuf
void _print_debug_array( unsigned char section, const unsigned char s[],int cnt, char *fmt);
#define print_debug_array( cection,tip,s,cnt,fmt ) \
    do{ \
        my_debug(cection,DEBUG_LEVEL_DEBUG)(tip);\
        _print_debug_array(cection,s,cnt,fmt);\
    }while(0)
/////////////////////////////////////////////////////
//BCD��ת10����
#define BCDtoDec( var ) (var)>0x99?0:((var)-((var) >> 4)*6)
//10����תBCD��
#define DectoBCD( var ) (var)>99?0:((var)+((var)/10)*6)
//HEX����תascll����,len ָHEX����ֽ���
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

//ascii תHEX��
//len ָASCII�ַ����ĳ���
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
//��ʱn��
int my_sleep( int sec );
//��ʱuseconds ΢��
int my_usleep(int usec );

//�õ�����ʱ��
void get_time( SYSTEM_TIME *tm );
int mystrtime( char *timebuf ,char* formt );
/*
 * ��׼����ض���
 * �����⵽/tiandao/stdout.txt�ļ����ڣ��򽫱�׼����ض���ø��ļ�
 * �����⵽���ļ������������ض���ر�׼���
 * �ú���Ϊ���̰߳�ȫ����
 */
int out_redirect( const char *path_name );
//�������һ�ε��øú�����ָ���ļ��Ƿ��޸Ĺ�
int check_file_modify_time( char *filename );
//��ӡ��ǰʱ�䣬����ʱʹ�ã��淶��־��ʽ
void print_date( int module, int level );

//��ȡϵͳ��������ʱ��
int getstarttime( void );
void setshutdowntime(void);
void getshutdowntime( unsigned char *bcdtime );

/*
extern int debuglevels;

void printBuf(char * disp, char * buf, int len);

// (�ڲ�ʹ��) �ж�"SECTION"ģ�鹦�ܺ��Ƿ�����"DEBUG_LEVEL"�ȼ��ĵ�����Ϣ���
#define __my_unallow_debug( DEBUG_LEVEL) \
 ( DEBUG_LEVEL < debuglevels)

// (�ڲ�ʹ��) ������Ϣ�������
#define __my_debug(FORMAT, ARG...) \
 printf("%s:%d %s(): " FORMAT, basename(__FILE__), __LINE__, __FUNCTION__, ##ARG)

// ��ʼ�����Եȼ�
#define my_init_debug_levels( ALLOW_DEBUG_LEVEL) \
 ( debuglevels = ALLOW_DEBUG_LEVEL )

// ������Ϣ�������
#define my_debug( DEBUG_LEVEL) \
        ( __my_unallow_debug( DEBUG_LEVEL) ) ? (void) 0 : __my_debug

// ������Ϣ�������
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


