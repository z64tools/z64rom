#include <ExtLib.h>

const char* Tools_Get(const char* app);
void Tools_Update(void);

#define Tools_Command(dest, tool, args, ...) sprintf(dest, "%s " args, Tools_Get(tool), __VA_ARGS__)