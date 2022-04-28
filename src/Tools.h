#include <ExtLib.h>

const char* Tools_Get(const char* app);
s32 Tools_Validate(void);
void Tools_Update_Header(void);
void Tools_Update_Binutils(void);
void Tools_Init(void);

#define Tools_Command(dest, tool, args, ...) sprintf(dest, "%s " args, Tools_Get(tool), __VA_ARGS__)