
#include <string.h>
#include <stdio.h>
//#define _XOPEN_SOURCE /* glibc2 needs this */
//��makefile�ļ����-D_GNU_SOURCE -D__USE_XOPEN����
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include "my_debug.h"

/*    
 *   global variants
 */
 
//int debuglevels = DEBUG_LEVEL_NONE;
int __my_allow_debug_levels[MY_SECTION_END];
extern int errno;

//��ʱuseconds ΢��
int my_usleep(int usec )
{
    if( usec < 1 || usec > 999999 ) 
        return -1;

    struct timespec delay;
    struct timespec rem;

    memset( &delay, 0, sizeof(delay));
    memset( &rem, 0, sizeof(rem));

    delay.tv_nsec = usec*1000;
    do{
        if( !nanosleep( &delay, &rem ))
        {
            break; 
        }
        if( errno == EINTR ) {
            printf("my_usleep:rem.tv_sec=%ld\n",(int long)rem.tv_sec);
            printf("my_usleep:rem.tv_nsec=%ld\n",rem.tv_nsec);
            delay.tv_sec = rem.tv_sec;
            delay.tv_nsec = rem.tv_nsec;
        } else {
            break;
        }
    }while(1);
    
    return 0;
}

//��ʱn��
int my_sleep( int sec )
{
    if( sec < 1 || sec > 999999999 ) 
        return -1;

    struct timespec delay;
    struct timespec rem;

    memset( &delay, 0, sizeof(delay));
    memset( &rem, 0, sizeof(rem));
    delay.tv_sec = sec;
    do{
        if( !nanosleep( &delay, &rem ) )
        {
            break; 
        }
        if( errno == EINTR ) {
            printf("my_sleep:rem.tv_sec=%ld\n",(int long)rem.tv_sec);
            printf("my_sleep:rem.tv_nsec=%ld\n",rem.tv_nsec);
            delay.tv_sec = rem.tv_sec;
            delay.tv_nsec = rem.tv_nsec;
        } else {
            break;
        }
    }while(1);
    
    return 0;
}
//�õ�����ʱ��
void get_time( SYSTEM_TIME *tm )
{
    struct tm tm_t;
    time_t tt;

    tt = time( NULL );
    localtime_r( &tt, &tm_t );

    tm->year = tm_t.tm_year + 1900 - 2000;
    tm->month = tm_t.tm_mon + 1;
    tm->week = tm_t.tm_wday;//day of week
    tm->day = tm_t.tm_mday;
    tm->hour = tm_t.tm_hour;
    tm->minute = tm_t.tm_min;
    tm->second = tm_t.tm_sec;
}

int mystrtime( char *timebuf ,char* formt )
{
    struct tm tm_t;
    time_t t_time;

    t_time = time( NULL );
    localtime_r(&t_time, &tm_t);
    strftime(timebuf, 100, formt , &tm_t );
    
    return 0;
}
/*
 *  ��׼����ض���
 *  �����⵽/tiandao/stdout.txt�ļ����ڣ��򽫱�׼����ض���ø��ļ�
 *  �����⵽���ļ������������ض���ر�׼���
 *  �ú���Ϊ���̰߳�ȫ����
 */
int out_redirect( const char *path_name )
{
    static int filefg =0;
    static int oldstdout;
    FILE *fpout;

    if( !path_name || !path_name[0] )
        return -1;
    //�����ļ��Ƿ����
    if( access( path_name , F_OK|W_OK ) < 0 ) {
        if( filefg ) {//����ļ��Ѿ���
            filefg = 0;
            dup2( oldstdout,1);
            close(oldstdout);//��׼����ض��� 1
        }else{
            filefg = 0;
        }
    } else {
        if( !filefg )
        {
            fpout=fopen( path_name, "a" );
            if( fpout==NULL ) {
                return errno;
            } else {
                oldstdout = dup(1);
                //close(1);
                if(dup2( fileno(fpout),1) < 0 )
                {
                    return errno;
                }
                fclose(fpout);
                filefg = 1;
            }
        }       
    }
    return 0;
}
/*
 * �������һ�ε��øú�����ָ���ļ��Ƿ��޸Ĺ�
 * �޸Ĺ�����0,���򷵻�-1
 * �ļ������ڷ���1
 */
int check_file_modify_time( char *filename )
{
    struct stat sat;
    static time_t oldtime = 0;

    if( access( filename, F_OK ) < 0 )
    {
        return 1;
    }
    if( stat( filename, &sat ) < 0 )
    {
        perror("stat");
        return 2;
    }
    if( !oldtime ) {
        oldtime = sat.st_ctime;
    }
    if( sat.st_ctime != oldtime )
    {
        oldtime = sat.st_ctime;
        
        return 0;
    }

    return -1;
}

//��ӡ��ǰʱ�䣬����ʱʹ�ã��淶��־��ʽ
void print_date( int module, int level )
{
    SYSTEM_TIME fre_date;
    
    get_time( &fre_date );

    my_debug(module,level)("[%02d-%02d %02d:%02d:%02d]\n",fre_date.month,fre_date.day,\
        fre_date.hour,fre_date.minute,fre_date.second);
}

// ��ȡϵͳ��������ʱ��
int getstarttime( void )
{
    FILE *fp;
    float ff=0;

    //�����ļ��Ƿ����
    if( access( "/proc/uptime" , F_OK ) < 0 ) {
        return 0;
    }
    fp = fopen("/proc/uptime","r");
    if( fp ){
        fscanf( fp,"%f",&ff);
        fclose( fp );
    }

    return (int)ff;
}
//ÿ��һ��ʱ�佫ϵͳʱ��д�����м�¼�ļ�
void setshutdowntime(void)
{
    system("echo `date +%Y-%m-%d%H:%M:%S` > /tiandao/downtime");
}

void getshutdowntime( unsigned char *bcdtime )
{
    FILE *stream;

    //�����ļ��Ƿ����
    if( !access( "/tiandao/downtime" , F_OK ) ) {
        stream = fopen("/tiandao/downtime","r");
        if( stream ){   
            char time[20]={0};
            struct tm tm_t;
            
            fgets( time,19, stream );
            if( time[0] == '2' && time[4] == '-' && time[12] == ':' )
            {
                strptime( time,"%Y-%m-%d%H:%M:%S", &tm_t );
                bcdtime[0] = DectoBCD(tm_t.tm_min);
                bcdtime[1] = DectoBCD(tm_t.tm_hour);
                bcdtime[2] = DectoBCD(tm_t.tm_mday);
                bcdtime[3] = DectoBCD(tm_t.tm_mon + 1);
                bcdtime[4] = DectoBCD(tm_t.tm_year + 1900 - 2000);
            } else {
                memset( bcdtime, 0, 5 );
            }
            fclose( stream );
        }
    } else {
        memset( bcdtime, 0, 5 );
    }
}

void _print_debug_array(unsigned char section, const unsigned char s[],int cnt, char *fmt)
{
    int j,i,k,n=0x00;
    char buffer[0x100];
    //�ú���Ĭ��ΪDEBUG_LEVEL_DEBUG�ȼ�
    if( DEBUG_LEVEL_DEBUG < __my_allow_debug_levels[section] ) {
        return;
    }
    //printf("%s", tip);fflush(stdout);

    while (  cnt > 0x00 )
    {
        k = cnt;
        if ( k > 0x20 ) k = 0x20;//�п��
        for ( j = 0x00,i =0x00 ; j < k ; j++ )
        {
            i += sprintf(&buffer[i],fmt,s[n+j]);
        }
        printf("%s\n",buffer);
        fflush(stdout);

        cnt -= k;
        n += k;
    }
}
void prints(char *s,int len)
{
	int i;

	for(i=0;i<len;i++)
	{
		putchar((isprint(s[i])?s[i]:'.'));
	}
}
void printBuf(char * disp, char * buf, int len)
{
    int i;

    if(buf == NULL || len == 0)
    {
        puts("******************");
        puts("invalid param");
        puts("******************");
        return;
    }

    puts("******************");
    if(disp)
        puts(disp);
    if(strlen(buf) < 500)
        prints(buf,len);
    else
        printf("strlen of buf:%d\n", strlen(buf));

    printf("len:%d\n", len);

    for (i = 0; i < len; i++)
        printf("%02x ", (unsigned char)*buf++);

    putchar('\n');
    puts("******************");
}

