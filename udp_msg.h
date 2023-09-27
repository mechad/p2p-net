#ifndef __MSG_TYPE_H__
#define __MSG_TYPE_H__

#define u8	unsigned char 
#define i8  signed char 
#define u16	unsigned short 
#define i16 signed short
#define u32	unsigned int 
#define i32 signed int

enum msg_id
{
	MSG_TEXT=0x30,	//文本消息
	USERLIST,		//获取用户列表
	CONNECTUSER,	//链接
	LOGIN,			//登录
	HEARTBEAT, 		//心跳
	ACK,			//应答确认
	P2PLINK,		//P2P链接
	LOGOUT
}__attribute__( (packed, aligned(1)) );

struct userinfo
{
	u8 username[20];
	u8 ip[16]; 		//xxx.xxx.xxx.xxx
	u16  port;		//端口号
	u32	 time;		//在线时间
}__attribute__( (packed, aligned(1)) );

struct linkinfo
{
	struct userinfo srcinfo;	
	struct userinfo destinfo;
}__attribute__( (packed, aligned(1)) );

struct message
{
	struct linkinfo user;	
	u32	len;		//消息长度
	u8 text[512];   //消息体
}__attribute__( (packed, aligned(1)) );

struct Msg
{
	enum msg_id id;
	u32	len;
	u8 dat[1024];//消息体
}__attribute__( (packed, aligned(1)) );

#endif
