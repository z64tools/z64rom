#include <ExtLib.h>

extern bool gAutoDownload;

typedef enum {
	mips64_gcc,
	mips64_ld,
	mips64_objdump,
	mips64_objcopy,
	z64audio,
	z64convert,
	seq64,
	seqas,
	nOVL,
	wget
} ToolIndex;

const char* Tools_Get(ToolIndex);
void Tools_Install_mips64(void);
void Tools_Install_z64hdr(s32 isUpdate);
void Tools_CheckUpdates(void);
s32 Tools_Init(void);

#define Tools_Command(dest, tool, args, ...) sprintf(dest, "%s " args, Tools_Get(tool), __VA_ARGS__)