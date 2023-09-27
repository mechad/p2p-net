#ifndef __READ_CMD_H__
#define __READ_CMD_H__

#define MAXARG 5
//返回参数个数
//-1 输入错误
int read_command(char **command,char **parameters,char *prompt);
void freecahe( void );
#endif
