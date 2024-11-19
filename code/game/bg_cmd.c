// vm version of the engine side command tokenization
// kinda lame but i wanna examine chat for possible accidental login commands
#include "../game/bg_cmd.h"

static	int			cmd_argc;
static	char* cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
static	char		cmd_tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS];	// will have 0 bytes inserted
// for auto-complete (copied from OpenJK)
static	char		cmd_cmd[BIG_INFO_STRING]; // the original command we received (no token processing)


/*
============
Cmd_Argc
============
*/
int		BG_Cmd_Argc(void) {
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char* BG_Cmd_Argv(int arg) {
	if ((unsigned)arg >= (unsigned)cmd_argc) {
		return "";
	}
	return cmd_argv[arg];
}


/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char* BG_Cmd_Args(void) {
	static	char		cmd_args[MAX_STRING_CHARS];
	int i;

	cmd_args[0] = 0;
	for (i = 1; i < cmd_argc; i++) {
		Q_strcat(cmd_args, sizeof(cmd_args), cmd_argv[i]);
		if (i + 1 != cmd_argc) {
			Q_strcat(cmd_args, sizeof(cmd_args), " ");
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
char* BG_Cmd_ArgsFrom(int arg) {
	static	char		cmd_args[BIG_INFO_STRING];
	int i;

	cmd_args[0] = 0;
	if (arg < 0)
		arg = 0;
	for (i = arg; i < cmd_argc; i++) {
		Q_strcat(cmd_args, sizeof(cmd_args), cmd_argv[i]);
		if (i + 1 != cmd_argc) {
			Q_strcat(cmd_args, sizeof(cmd_args), " ");
		}
	}

	return cmd_args;
}

const char* BG_Cmd_Cmd(void) {
	return cmd_cmd;
}

/*
============
Cmd_ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	BG_Cmd_ArgsBuffer(char* buffer, int bufferLength) {
	Q_strncpyz(buffer, BG_Cmd_Args(), bufferLength);
}

/*
============
Cmd_DropArg

Drop argument from tokenized command
Doesn't update cmd_cmd
============
*/
void	BG_Cmd_DropArg(int arg) {
	if (0 <= arg && arg < cmd_argc) {
		for (; arg < cmd_argc - 1; arg++) {
			cmd_argv[arg] = cmd_argv[arg + 1];
		}

		cmd_argc--;
	}
}




/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the appropriate place, The argv array
will point into this temporary buffer.
============
*/
// NOTE TTimo define that to track tokenization issues
//#define TKN_DBG
static void BG_Cmd_TokenizeString2(const char* text_in, qboolean ignoreQuotes) {
	const char* text;
	char* textOut;

#ifdef TKN_DBG
	// FIXME TTimo blunt hook to try to find the tokenization of userinfo
	Com_DPrintf("Cmd_TokenizeString: %s\n", text_in);
#endif

	// clear previous args
	cmd_argc = 0;

	if (!text_in) {
		return;
	}

	Q_strncpyz(cmd_cmd, text_in, sizeof(cmd_cmd));

	text = text_in;
	textOut = cmd_tokenized;

	while (1) {
		if (cmd_argc == MAX_STRING_TOKENS) {
			return;			// this is usually something malicious
		}

		while (1) {
			// skip whitespace
			while (*text && *(const unsigned char* /*eurofix*/)text <= ' ') {
				text++;
			}
			if (!*text) {
				return;			// all tokens parsed
			}

			// skip // comments
			if (text[0] == '/' && text[1] == '/') {
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if (text[0] == '/' && text[1] == '*') {
				while (*text && (text[0] != '*' || text[1] != '/')) {
					text++;
				}
				if (!*text) {
					return;		// all tokens parsed
				}
				text += 2;
			}
			else {
				break;			// we are ready to parse a token
			}
		}

		// handle quoted strings
	// NOTE TTimo this doesn't handle \" escaping
		if (!ignoreQuotes && *text == '"') {
			cmd_argv[cmd_argc] = textOut;
			cmd_argc++;
			text++;
			while (*text && *text != '"') {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if (!*text) {
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = textOut;
		cmd_argc++;

		// skip until whitespace, quote, or command
		while (*(const unsigned char* /*eurofix*/)text > ' ') {
			if (!ignoreQuotes && text[0] == '"') {
				break;
			}

			if (text[0] == '/' && text[1] == '/') {
				break;
			}

			// skip /* */ comments
			if (text[0] == '/' && text[1] == '*') {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if (!*text) {
			return;		// all tokens parsed
		}
	}

}
/*
============
Cmd_TokenizeString
============
*/
void BG_Cmd_TokenizeString(const char* text_in) {
	BG_Cmd_TokenizeString2(text_in, qfalse);
}

/*
============
Cmd_TokenizeStringIgnoreQuotes
============
*/
void Cmd_TokenizeStringIgnoreQuotes(const char* text_in) {
	BG_Cmd_TokenizeString2(text_in, qtrue);
}


