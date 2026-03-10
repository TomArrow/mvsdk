#include "../g_local.h"

void G_TvT_Printf(int clientNum, const char *fmt, ...) {
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (clientNum == TVT_PRINT_CONSOLE) {
		G_Printf("%s", buf);
	} else {
		trap_SendServerCommand(clientNum, va("print \"%s\"", buf));
	}
}
