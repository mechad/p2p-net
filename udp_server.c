#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "udp_msg.h"
#include "my_debug.h"

#define OFFSET_OF(obj_type,mb)  ((int)&(((obj_type*)0)->mb))
#define MSG_SIZE(msg)	OFFSET_OF(struct Msg,dat)+msg.len
#define MAX(a, b) ((a) > (b)?(a):(b))


/*
    模块内局部变量
*/
static int debugModule = MY_SECTION_MAIN;


void setAllDebugLevel( int level )
{   
    int i;  
    for(i = 0; i < MY_SECTION_END; i++) 
    {       
        my_init_debug_levels( i, level ); 
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s port\n", argv[0]);
        exit(1);
    }
	
	setAllDebugLevel( DEBUG_LEVEL_ALL );
	
    print_date( debugModule, DEBUG_LEVEL_INFO );
	my_debug( debugModule, DEBUG_LEVEL_INFO)("Welcome! This is a UDP server.\n");
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock;
    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }
    char buff[512];
    struct sockaddr_in clientAddr;
    int n;
    int len = sizeof(clientAddr);
	
	struct Msg msg;
	struct userinfo user[5];//最多五个用户登录
	int user_num=0;//登录用户个数
	char *ip;
	u16 port;
	int i=0;
	bzero( user, sizeof(user) );
    while (1)
    {
		bzero( &msg, sizeof(msg) );
        n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&clientAddr, &len);
        if (n>0)
        {
			ip = inet_ntoa(clientAddr.sin_addr);
			port = ntohs(clientAddr.sin_port);
			printf("receive data from [%s:%d]\n",ip,port);
			my_debug_print_buf(debugModule, DEBUG_LEVEL_INFO)("msg:",(char*)&msg,n);
			if( msg.id == LOGIN )//收到登录信息
			{
				struct userinfo uif;
				bzero(&uif, sizeof(struct userinfo));
				memcpy( &uif, msg.dat, sizeof(struct userinfo) );
				
				memcpy(uif.ip, ip, strlen(ip));
				uif.port = port;
				uif.time = 10000;
				for( user_num=0,i=0; i< 5; i++ )
				{
					user_num++;
					if( user[i].username[0] == '\0' )
					{
						memcpy( &user[i], &uif, sizeof(uif) );
						break;
					}					
				}
				for( i=0; i< 5; i++ )
				{
					if( user[i].username[0] != '\0' )
					{
						clientAddr.sin_family = AF_INET;
						clientAddr.sin_port = htons(user[i].port);
						clientAddr.sin_addr.s_addr = inet_addr(user[i].ip);
						if(clientAddr.sin_addr.s_addr == INADDR_NONE)
						{
							my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!\n");
							continue;
						}
						msg.id = USERLIST;//直接将登录用户信息更新到客户端
						msg.len = sizeof(uif)*user_num;
						memcpy( msg.dat, user, sizeof(uif)*user_num );
						n = sendto(sock, &msg, MSG_SIZE(msg), 0, (struct sockaddr *)&clientAddr, len);
						if (n < 0)
						{
							perror("sendto");
							break;
						}
					}					
				}
			} 
			else if( msg.id == USERLIST )
			{
				msg.id = USERLIST;//直接将登录用户信息更新到客户端
				msg.len = sizeof(struct userinfo)*user_num;
				memcpy( msg.dat, user, sizeof(struct userinfo)*user_num );
				n = sendto(sock, &msg, MSG_SIZE(msg), 0, (struct sockaddr *)&clientAddr, len);
				if (n < 0)
				{
					perror("sendto");
					break;
				}				
			}
			else if( msg.id == CONNECTUSER )//链接请求转发给目标主机
			{				
				struct linkinfo connect;
				
				bzero( &connect, sizeof(connect) );
				memcpy( &connect, msg.dat, msg.len );
				strcpy(connect.srcinfo.ip, inet_ntoa(clientAddr.sin_addr));
				connect.srcinfo.port = ntohs(clientAddr.sin_port);
				memcpy( msg.dat, &connect, msg.len );
				print_date( debugModule, DEBUG_LEVEL_INFO );
				my_debug( debugModule, DEBUG_LEVEL_INFO)("receive connect from: %s:%d\n",connect.srcinfo.ip,connect.srcinfo.port);
				my_debug( debugModule, DEBUG_LEVEL_INFO)("send connect to: %s:%d\n",connect.destinfo.ip,connect.destinfo.port);
				
				addr.sin_family = AF_INET;
				addr.sin_port = htons(connect.destinfo.port);
				addr.sin_addr.s_addr = inet_addr(connect.destinfo.ip);
				if(addr.sin_addr.s_addr == INADDR_NONE)
				{
					my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!\n");
					continue;
				}
				n = sendto(sock, &msg, MSG_SIZE(msg), 0, (struct sockaddr *)&addr, len);
				if (n < 0)
				{
					perror("sendto");
					break;
				}			
			}
			else if( msg.id == ACK )//目标确认链接时，将确认信息转发给发起者
			{				
				struct linkinfo connect;
				
				bzero( &connect, sizeof(connect) );
				memcpy( &connect, msg.dat, msg.len );
				print_date( debugModule, DEBUG_LEVEL_INFO );
				my_debug( debugModule, DEBUG_LEVEL_INFO)("receive ACK from: %s:%d\n",connect.srcinfo.ip,connect.srcinfo.port);
				my_debug( debugModule, DEBUG_LEVEL_INFO)("send ACK to: %s:%d\n",connect.destinfo.ip,connect.destinfo.port);
				addr.sin_family = AF_INET;
				addr.sin_port = htons(connect.destinfo.port);
				addr.sin_addr.s_addr = inet_addr(connect.destinfo.ip);
				if(addr.sin_addr.s_addr == INADDR_NONE)
				{
					my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!");
					continue;
				}
				n = sendto(sock, &msg, MSG_SIZE(msg), 0, (struct sockaddr *)&addr, len);
				if (n < 0)
				{
					perror("sendto");
					break;
				}	
			}
			else if(msg.id == MSG_TEXT)
			{
				struct message message;
			
				bzero( &message, sizeof(message) );
				memcpy( &message, msg.dat , msg.len );
				print_date( debugModule, DEBUG_LEVEL_INFO );
				printBuf("", message.user.srcinfo.username, 10 );
				printf("receice from %s[%d]:\n", message.user.srcinfo.username, message.len);
				message.text[message.len] = '\0';
				printf("message:%s\n", message.text );			
			}
			else if( msg.id == LOGOUT )//收到登录信息
			{
				struct userinfo uif;
				bzero(&uif, sizeof(struct userinfo));
				memcpy( &uif, msg.dat, sizeof(struct userinfo) );
				
				for( i=0; i< 5; i++ )
				{
					if( !strcmp(user[i].username, uif.username) )
					{
						memset( &user[i], 0, sizeof(uif) );
						user_num --;
						break;
					}					
				}
			} 
        }
        else
        {
            perror("recv");
            break;
        }
    }
    return 0;
}
