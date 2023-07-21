#include <ext_type.h>

typedef enum {
	mips64_gcc,
	mips64_ld,
	mips64_objdump,
	mips64_objcopy,
	z64audio,
	z64convert,
	z64playas,
	seq64,
	seqas,
	nOVL,
	nm,
} ToolIndex;

const char* Tools_Get(ToolIndex);
void Tools_CheckUpdates(void);
s32 Tools_Init(void);

void Tools_InstallHeader(bool update);

#define Tools_Command(dest, tool, args, ...) sprintf(dest, "%s " args, Tools_Get(tool), __VA_ARGS__)