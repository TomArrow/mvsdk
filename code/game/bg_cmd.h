
#ifndef BG_CMD_H
#define BG_CMD_H
#include "../game/q_shared.h"

int		BG_Cmd_Argc(void);
char* BG_Cmd_Argv(int arg);
char* BG_Cmd_Args(void);
char* BG_Cmd_ArgsFrom(int arg);
const char* BG_Cmd_Cmd(void);
void	BG_Cmd_ArgsBuffer(char* buffer, int bufferLength);
void	BG_Cmd_DropArg(int arg);
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are allways safe.

void	BG_Cmd_TokenizeString(const char* text);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.
#endif
