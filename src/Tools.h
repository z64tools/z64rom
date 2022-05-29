#include <ExtLib.h>

extern bool gAutoDownload;

typedef enum {
	mips64_gcc,
	mips64_ld,
	mips64_objdump,
	mips64_objcopy,
	z64audio,
	z64convert,
	z64compress,
	seq64,
	seqas,
	nOVL,
	wget
} ToolIndex;

const char* Tools_Get(ToolIndex);
s32 Tools_RegisterBlender(MemFile* mem);
void Tools_Clean(void);
s32 Tools_Validate_ReqrTools(void);
s32 Tools_Validate_AddiTools(void);
void Tools_Update_Header(bool install);
void Tools_Update_Binutils(void);
void Tools_CheckUpdates(void);
s32 Tools_Init(void);

#define Tools_Command(dest, tool, args, ...) sprintf(dest, "%s " args, Tools_Get(tool), __VA_ARGS__)