#ifndef __Z64_ROM_H__
#define __Z64_ROM_H__

#include <ext_type.h>

struct Rom;
struct RomFile;

typedef struct PatchNode {
	struct PatchNode* prev;
	struct PatchNode* next;
	u32  start;
	u32  end;
	char source[128];
	char section[128];
} PatchNode;

void Rom_New(struct Rom* rom, const char* romName);
void Rom_Free(struct Rom* rom);
void Rom_Dump(struct Rom* rom);
void Rom_Build(struct Rom* rom);
void Rom_ItemList(List* list, const char* path, bool isNum, ListFlag flags);
void Rom_Compress(void);
void Rom_DeleteUnusedContent(void);
s32 Rom_Extract(Memfile* mem, struct RomFile rom, const char* name);
void AudioOnly_Dump(struct Rom* rom);
void AudioOnly_Build(struct Rom* rom);

#endif