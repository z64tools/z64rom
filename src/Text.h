#include <ExtLib.h>

struct Rom;

typedef struct __attribute__((scalar_storage_order("big-endian"))) {
	u16 textId;
	u8 typePos;
	void32 segment;
} MessageTableEntry;

void Text_Dump(struct Rom* rom);
void Text_Build(struct Rom* rom);

extern s32 gTextFlag;