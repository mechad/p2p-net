#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

#include "read_cmd.h"
/* A static variable for holding the line. */
static char *line_read = (char *)NULL;

void freecahe( void )
{
	if(line_read)
	{
		free(line_read);
		line_read = NULL;
	}	
}
char *rl_gets(const char *prompt)
{
	/* If the buffer has already been allocated, return the memory
	to the free pool. */
	if(line_read)
	{
		free(line_read);
		line_read = NULL;
	}

	/* Get a line from the user. */
	line_read = readline( prompt );

	/* If the line has any text in it, save it on the history. */
	if(line_read && *line_read)
		add_history(line_read);

	return (line_read);
}
//返回参数个数
//-1 输入错误
int read_command(char **command,char **parameters,char *prompt)
{
	char *buffer = rl_gets(prompt);
	if( buffer == NULL ) 
		return -1;
    if( buffer[0] == '\0' || buffer[0] == '\n' )
        return -1;
	
    char *pStart,*pEnd;
    int count = 0;
    int isFinished = 0;
    pStart = pEnd = buffer;
    while(isFinished == 0)
    {
		//被双引号'' or "“ 单引号包裹的属于一个字符串
		while((*pEnd == '\'' && *pStart == '\'') || (*pEnd == '\"' && *pStart == '\"'))
        {
            pEnd++;
			while(*pEnd != *pStart && *pEnd != '\0' && *pEnd != '\n')
				pEnd++;
			if( *pEnd == *pStart )//如果闭合
			{
				parameters[count-1] = ++pStart;
				count++;
				*pEnd = '\0';
				pEnd++;
				pStart = pEnd;
			}
        }
        while((*pEnd == ' ' && *pStart == ' ') || (*pEnd == '\t' && *pStart == '\t'))
        {
            pStart++;
            pEnd++;
        }

        if(*pEnd == '\0' || *pEnd == '\n')
        {
            if(count == 0)
                return -1;
            break;
        }

        while(*pEnd != ' ' && *pEnd != '\0' && *pEnd != '\n')
            pEnd++;


        if(count == 0)
        {
            *command = pStart;
			count++;
        }
        else if(count <= MAXARG)
        {
            parameters[count-1] = pStart;
            count++;
        }
        else
        {
            break;
        }

        if(*pEnd == '\0' || *pEnd == '\n')
        {
            *pEnd = '\0';
            isFinished = 1;
        }
        else
        {
            *pEnd = '\0';
            pEnd++;
			pStart = pEnd;
        }
    }

    parameters[count-1] = NULL;
    return count;
}
