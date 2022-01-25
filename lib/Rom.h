#ifndef __Z64_ROM_H__
#define __Z64_ROM_H__

#include "ExtLib.h"

struct Rom;
struct RomFile;

void Rom_New(struct Rom* rom, char* romName);
void Rom_Free(struct Rom* rom);
void Rom_Dump(struct Rom* rom);
void Rom_Build(struct Rom* rom);

void Rom_ItemList(ItemList* list, bool isPath, bool isNum, bool numericalSort);
s32 Rom_Extract(MemFile* mem, struct RomFile rom, char* name);

#endif