#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <strings.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "udp_msg.h"
#include "read_cmd.h"
#include "my_debug.h"

#define OFFSET_OF(obj_type,mb)  ((int)&(((obj_type*)0)->mb))
#define MSG_SIZE(msg)	OFFSET_OF(struct Msg,dat)+msg.len
#define MESSAGE_SIZE( message ) OFFSET_OF(struct message,text)+message.len

#define MAX(a, b) ((a) > (b)?(a):(b))
#define MIN(a, b) ((a) > (b)?(b):(a))

/*
 *   模块内局部变量
 */
static int debugModule = MY_SECTION_MAIN;

char *command = NULL;
char **parameters = NULL;
extern int errno;
struct userinfo userlist[5];
int user_num = 0;
int P2P_No = 0;//P2P通信对方的序号
struct sockaddr_in host_addr;//服务器
char myname[20]={'c','o','m','m','a','n','d',0};
int sock;


void rec_cleanup(void *arg) 
{
	printf("cleaning up ressources allocated by rec thread \n");
}

void print_userlist(void)
{
	int i = 0;
	while( i < user_num )
	{
		printf("\n----------------------------\n");
		printf("%d. username: %s\n", i+1, userlist[i].username );
		printf("         ip: %s\n", userlist[i].ip );
		printf("       port: %d\n", userlist[i].port );
		printf("----------------------------\n");	
		i ++;
	}
	
}
int get_user_no( const char* username )
{
	int i = 0;
	while( i < user_num )
	{
		i++;
		if( !strcmp( userlist[i-1].username, username ))
			return i;
	}
	return -1;	
}

int SendMsg(int sock, struct sockaddr *addr, struct Msg msg )
{
	int n = sendto(sock, &msg, MSG_SIZE(msg), 0, addr, sizeof(struct sockaddr));
	if( n < 0 )
	{
		perror("sendto");
	}
	return n;
}

void *recthread( void *param )
{
	char buff[512];
	int sock = *(int*)param;
	int len;
	struct Msg msg;
	struct sockaddr_in addr;
	
	bzero( &msg, sizeof(msg) );

	while(1)
	{
		int n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr *)0, NULL);
        if (n>0)
        {
			//my_debug_print_buf(debugModule, DEBUG_LEVEL_INFO)("",(char*)&msg,n);
			if( msg.id == USERLIST )//收到登录信息
			{
				struct userinfo uif;
				bzero(&uif, sizeof(struct userinfo));
				len = msg.len;
				user_num = 0;
				while( len != 0 )
				{
					struct userinfo uif;
					memcpy( &uif, msg.dat+user_num*sizeof(uif), sizeof(uif));
					len -= sizeof(uif);
					printf("\n----------------------------\n");
					printf("%d. username: %s\n", user_num+1, uif.username );
					printf("         ip: %s\n", uif.ip );
					printf("       port: %d\n", uif.port );
					printf("----------------------------\n");	
					memcpy( &userlist[user_num], &uif, sizeof(uif) );
					user_num ++;
				}
			} 
			else if( msg.id == CONNECTUSER )//链接请求
			{
				struct linkinfo connect;
				memcpy( &connect, msg.dat, msg.len );
				
				addr.sin_family = AF_INET;
				addr.sin_port = htons(connect.srcinfo.port);
				addr.sin_addr.s_addr = inet_addr(connect.srcinfo.ip);
				if(addr.sin_addr.s_addr == INADDR_NONE)
				{
					my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!\n");
					continue;
				}
				print_date( debugModule, DEBUG_LEVEL_INFO );
				my_debug( debugModule, DEBUG_LEVEL_INFO)("receive connect from: %s:%d\n",connect.srcinfo.ip,connect.srcinfo.port);
				my_debug( debugModule, DEBUG_LEVEL_INFO)("send connect to: %s:%d\n",connect.srcinfo.ip,connect.srcinfo.port);
				my_debug( debugModule, DEBUG_LEVEL_INFO)("send connect to host: %s:%d\n",inet_ntoa(host_addr.sin_addr),ntohs(host_addr.sin_port));
				msg.id = ACK;
				msg.len = sizeof(connect);
				struct userinfo uif;
				memcpy( &uif, &connect.destinfo, sizeof(uif));
				memcpy( &connect.destinfo, &connect.srcinfo, sizeof(uif));
				memcpy( &connect.srcinfo, &uif, sizeof(uif));// 源与目标信息调换
				memcpy( msg.dat, &connect, msg.len );
				my_debug( debugModule, DEBUG_LEVEL_INFO)("send ack\n");
				SendMsg( sock, (struct sockaddr *)&addr, msg);//向发起者回复确认，表示我已经开好洞
				usleep(100*1000);//100ms
				SendMsg( sock, (struct sockaddr *)&host_addr, msg);//发送链接请求到服务器
			}
			else if( msg.id == ACK )//链接请求被受理，只有发起者会收到该消息
			{
				struct linkinfo connect;
				memcpy( &connect, msg.dat, msg.len );
				
				struct message message;
			
				bzero( &message, sizeof(message) );
				memcpy(message.user.srcinfo.username, myname , strlen(myname));
				int No = get_user_no(myname);
				if( No > user_num || No < 0 )
				{
					print_userlist();
					my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect username\n");
					continue;
				}				
				//源是自己
				memcpy(message.user.srcinfo.ip, userlist[No-1].ip , strlen(userlist[No-1].ip));
				message.user.srcinfo.port = userlist[No-1].port;
				memcpy(&message.user.destinfo, &connect.srcinfo, sizeof(connect.srcinfo));
				
				message.len = 3;
				strcpy( message.text, "ok" );
				
				msg.len = MESSAGE_SIZE( message );	
				msg.id = P2PLINK;
				memcpy( msg.dat, &message, msg.len );
				print_date( debugModule, DEBUG_LEVEL_INFO );
				my_debug( debugModule, DEBUG_LEVEL_INFO)("receive ACK from: %s:%d\n",connect.srcinfo.ip,connect.srcinfo.port);
				my_debug( debugModule, DEBUG_LEVEL_INFO)("send p2plink to: %s:%d\n",connect.srcinfo.ip,connect.srcinfo.port);
				//不再经过服务器，直接发送到目标端点
				addr.sin_family = AF_INET;
				addr.sin_port = htons(connect.srcinfo.port);
				addr.sin_addr.s_addr = inet_addr(connect.srcinfo.ip);
				if(addr.sin_addr.s_addr == INADDR_NONE)
				{
					my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!");
					continue;
				}
				
				SendMsg( sock, (struct sockaddr *)&addr, msg);//向目标机发送第一条消息
			}
			else if(msg.id == P2PLINK)
			{
				struct message message;
			
				memcpy( &message, msg.dat , msg.len );
				if( !strcmp(message.user.srcinfo.username, myname) )//如果源是自己，说明我发起的P2P链接建立成功
				{
					print_date( debugModule, DEBUG_LEVEL_INFO );
					my_debug( debugModule, DEBUG_LEVEL_INFO)("Launched by I create P2P links to establish a success!\n");
					
					message.len = 9;
					strcpy( message.text, "link ok!" );
					
					msg.len = MESSAGE_SIZE( message );	
					msg.id = MSG_TEXT;
					memcpy( msg.dat, &message, msg.len );
					
					//不再经过服务器，直接发送到目标端点
					addr.sin_family = AF_INET;
					addr.sin_port = htons(message.user.destinfo.port);
					addr.sin_addr.s_addr = inet_addr(message.user.destinfo.ip);
					if(addr.sin_addr.s_addr == INADDR_NONE)
					{
						my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!");
						continue;
					}
					
					SendMsg( sock, (struct sockaddr *)&addr, msg);//发送第二条消息
					P2P_No = get_user_no(message.user.destinfo.username);
				} else {//收到发起者发过来的P2P数据，说明链接建立成功
					//不再经过服务器，直接发送到目标端点
					addr.sin_family = AF_INET;
					addr.sin_port = htons(message.user.srcinfo.port);
					addr.sin_addr.s_addr = inet_addr(message.user.srcinfo.ip);
					if(addr.sin_addr.s_addr == INADDR_NONE)
					{
						my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!");
						continue;
					}
					
					SendMsg( sock, (struct sockaddr *)&addr, msg);//发送第一条消息
					print_date( debugModule, DEBUG_LEVEL_INFO );
					my_debug( debugModule, DEBUG_LEVEL_INFO)("Already establish P2P links with the initiator\n");
					P2P_No = get_user_no(message.user.srcinfo.username);
				}
				alarm(1);
			}
			else if(msg.id == MSG_TEXT)
			{
				struct message message;
			
				memcpy( &message, msg.dat , msg.len );
				print_date( debugModule, DEBUG_LEVEL_INFO );
				my_debug( debugModule, DEBUG_LEVEL_INFO)("receice from %s:\n", message.user.srcinfo.username);
				message.text[message.len] = '\0';
				my_debug( debugModule, DEBUG_LEVEL_INFO)("%s\n", message.text);					
			}
			else if(msg.id == HEARTBEAT)
			{
				//收到心跳不做处理
				printf("receive heartbeat\n");
			}
        }
        else if (n==0)
        {
            print_date( debugModule, DEBUG_LEVEL_INFO );
			my_debug( debugModule, DEBUG_LEVEL_INFO)("server closed\n");
            close(sock);
            break;
        }
        else if (n == -1)
        {
            print_date( debugModule, DEBUG_LEVEL_INFO );
			my_debug( debugModule, DEBUG_LEVEL_ERROR)("recvfrom");
            close(sock);
            break;
        }
	}
}

static void sig_timer(int sig)
{
	switch( sig )
	{
		case SIGALRM:
		{
			struct Msg msg;
			struct sockaddr_in addr;

			msg.id = HEARTBEAT;
			msg.len = 1;
			msg.dat[0] = 'H';

			if( P2P_No > 5 || P2P_No < 1 )
				break;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(userlist[P2P_No-1].port);
			addr.sin_addr.s_addr = inet_addr(userlist[P2P_No-1].ip);
			if(addr.sin_addr.s_addr == INADDR_NONE)
			{
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!");
				break;
			}
			SendMsg( sock, (struct sockaddr *)&addr, msg);	
		}
		break;
		case SIGINT:
		case SIGQUIT:
		{
			free(parameters);
			freecahe();
			close(sock);
			exit(0);
		}
		break;		
	}
	alarm(1);
}

void usage(void)
{
	printf("\n*****************************************************\n");
    printf("--------------This is a UDP client.-------------\n");
	printf("> Author: chad\n");
	printf(">   Mail: linczone@163.com\n");
	printf(">version: v1.0\n");
	printf(">  Usage: command [param...] [text]\n");
	printf("          login   username hostip hostport\n");
	printf("          userlist\n");
	printf("          connect username\n");
	printf("          sendto  username text\n");
	printf("          exit\n");
	printf("          logout\n");
	printf("          clear\n");
	printf(">Created Time: %s %s\n",__DATE__,__TIME__);
	printf("*****************************************************\n");
}

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
	usage();
	struct sockaddr_in addr;
	int n;
	struct Msg msg;
	pthread_t thread_id=-1;
	int flag = 0;
	
	setAllDebugLevel( DEBUG_LEVEL_ALL );
	
	parameters = malloc(sizeof(char *)*(MAXARG+2));
    if( parameters == NULL )
    {
        my_debug( debugModule, DEBUG_LEVEL_ERROR)("ERR:malloc failed.\n");
        exit(1);
    }
	
    if ( (sock=socket(AF_INET, SOCK_DGRAM, 0)) <0)
    {
        perror("socket");
        exit(1);
    }
	addr.sin_family = AF_INET;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
	{
		close(sock);
		perror("bind");
		exit(1);
	}
	host_addr.sin_family = AF_INET;
	bzero( &msg, sizeof(msg) );
	
	char prompt[20] = {0};
	//注册线程取消点
	//pthread_cleanup_push( rec_cleanup, NULL );
	signal(SIGALRM, sig_timer);
	signal(SIGINT, sig_timer); 
	signal(SIGQUIT, sig_timer); 
	strcpy( prompt, "command>" );
	
	while( 1 )
	{
		usleep(100000);//等待服务器回复后再推出输入提示
		int params = read_command( &command, parameters, prompt );
        if( -1 == params )
            continue;
		if( !strcmp( command, "login" ) )
		{
			if( flag )
			{
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Can't repeat the login.\n");
				continue;
			}
			if( params != 4 ) 
			{
				my_debug( debugModule, DEBUG_LEVEL_INFO)("Incorrect format!\nUsage: login username hostip hostport\n");
				continue;
			}
			host_addr.sin_port = htons(atoi(parameters[2]));
			host_addr.sin_addr.s_addr = inet_addr(parameters[1]);
			if(host_addr.sin_addr.s_addr == INADDR_NONE)
			{
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!");
				continue;
			}
			bzero( myname, strlen(myname) );
			
			struct userinfo uif;
			
			bzero( &uif, sizeof(uif));
			memcpy(uif.username, parameters[0], MIN(sizeof(uif.username)-1, strlen(parameters[0])));
			memcpy(myname, uif.username, strlen(uif.username));
			msg.id = LOGIN;
			msg.len = sizeof(uif);
			memcpy( msg.dat, &uif, msg.len );
			
			SendMsg( sock, (struct sockaddr *)&host_addr, msg);
			
			if( pthread_create(&thread_id, NULL, recthread, &sock )!=0 ) 
			{
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Create thread error!\n");
				exit(1);
			}
			pthread_detach(thread_id);
			flag = 1;
			strcpy( prompt, myname );
			strcat( prompt, ">" );
		} 
		else if( !strcmp( command, "exit" ) ) 
		{
			free(parameters);
			freecahe();
			close(sock);
			exit(0);
		} 
		else if( !strcmp( command, "clear" ) ) 
		{
			system("clear");
		}
		
		if( !flag ) //没有登录之前不允许执行后面的命令
		{
			continue;			
		}
		
		if( !strcmp( command, "userlist" ) ) 
		{
			msg.id = USERLIST;
			msg.len = 0;

			SendMsg( sock, (struct sockaddr *)&host_addr, msg);//发送链接请求到服务器
		} 		
		else if( !strcmp( command, "connect" ) ) 
		{
			if( params != 2 ) 
			{
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect format!\nUsage: connect username\n");
				continue;
			}
			int No = get_user_no(parameters[0]);
			if( No > user_num || No < 0 )
			{
				print_userlist();
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect username\n");
				continue;
			}
			struct linkinfo connect;
			//bzero( myname, sizeof(myname) );
			bzero( &connect, sizeof(struct linkinfo) );
			
			//memcpy(myname, parameters[0], MAX(sizeof(myname)-1, strlen(parameters[0])));
			msg.id = CONNECTUSER;
			msg.len = sizeof(struct linkinfo);
			memcpy(connect.srcinfo.username, myname , strlen(myname));
			memcpy(connect.destinfo.username, userlist[No-1].username , strlen(userlist[No-1].username));
			memcpy(connect.destinfo.ip, userlist[No-1].ip , strlen(userlist[No-1].ip));
			connect.destinfo.port = userlist[No-1].port;
			memcpy( msg.dat, &connect, msg.len );
			/******************debug**************************/
			print_date( debugModule, DEBUG_LEVEL_INFO );
			my_debug( debugModule, DEBUG_LEVEL_INFO)("create connect to: %s:%d\n",userlist[No-1].ip,userlist[No-1].port);
			my_debug( debugModule, DEBUG_LEVEL_INFO)("host: %s:%d\n",inet_ntoa(host_addr.sin_addr), ntohs(host_addr.sin_port));
			/********************************************/
			SendMsg( sock, (struct sockaddr *)&host_addr, msg);//发送链接请求到服务器
		} 
		else if( !strcmp( command, "sendto" ) ) 
		{
			if( params != 3 ) 
			{
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect format!\nUsage: sendto  username text\n");
				continue;
			}
			if( !strcmp( "host", parameters[0] ) )
			{
				struct message message;
			
				bzero( &message, sizeof(message) );
				memcpy(message.user.srcinfo.username, myname , strlen(myname));
				message.len = strlen(parameters[1]);
				memcpy( message.text, parameters[1], message.len );
				msg.len = MESSAGE_SIZE( message );
				msg.id = MSG_TEXT;
				memcpy( msg.dat, &message, msg.len );
				
				SendMsg( sock, (struct sockaddr *)&host_addr, msg);//发送链接请求到服务器
				continue;
			}
			int No = get_user_no(parameters[0]);
			if( No > user_num || No < 0 )
			{
				print_userlist();
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect username\n");
				continue;
			}
			struct message message;
			
			bzero( &message, sizeof(message) );
			memcpy(message.user.srcinfo.username, myname , strlen(myname));
			memcpy(message.user.destinfo.username, userlist[No-1].username , strlen(userlist[No-1].username));
			memcpy(message.user.destinfo.ip, userlist[No-1].ip , strlen(userlist[No-1].ip));
			message.user.destinfo.port = userlist[No-1].port;
			message.len = strlen(parameters[1]);
			memcpy( message.text, parameters[1], message.len );
			
			msg.len = MESSAGE_SIZE( message );	
			msg.id = MSG_TEXT;
			memcpy( msg.dat, &message, msg.len );
			
			addr.sin_port = htons(userlist[No-1].port);
			addr.sin_addr.s_addr = inet_addr(userlist[No-1].ip);
			if(addr.sin_addr.s_addr == INADDR_NONE)
			{
				my_debug( debugModule, DEBUG_LEVEL_ERROR)("Incorrect ip address!");
				continue;
			}
			
			SendMsg( sock, (struct sockaddr *)&addr, msg);//发送链接请求到服务器	
		}
		else if( !strcmp( command, "logout" ) )
		{
			struct userinfo uif;
			flag = 0;
			strcpy( prompt, "command>" );
			
			bzero( &uif, sizeof(uif));
			memcpy(uif.username, myname, strlen(myname));
		
			msg.id = LOGOUT;
			msg.len = sizeof(uif);
			memcpy( msg.dat, &uif, msg.len );
			alarm(0);	
			SendMsg( sock, (struct sockaddr *)&host_addr, msg);
			alarm(0);
		}
		else {
		//	print_userlist();
		}
		
	}
    
    return 0;
}

