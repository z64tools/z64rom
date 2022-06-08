#include "z64rom.h"

s32 gTextFlag;

#define TextSegment(addr) SegmentedToVirtual(6, addr - 0x07000000)

typedef enum {
	CTRL_NEWLINE            = 0x01,
	CTRL_END                = 0x02,
	CTRL_BOX_BREAK          = 0x04,
	CTRL_COLOR              = 0x05,
	CTRL_SHIFT              = 0x06,
	CTRL_TEXTID             = 0x07,
	CTRL_QUICKTEXT_ENABLE   = 0x08,
	CTRL_QUICKTEXT_DISABLE  = 0x09,
	CTRL_PERSISTENT         = 0x0A,
	CTRL_EVENT              = 0x0B,
	CTRL_BOX_BREAK_DELAYED  = 0x0C,
	CTRL_AWAIT_BUTTON_PRESS = 0x0D,
	CTRL_FADE               = 0x0E,
	CTRL_NAME               = 0x0F,
	CTRL_OCARINA            = 0x10,
	CTRL_FADE2              = 0x11,
	CTRL_SFX                = 0x12,
	CTRL_ITEM_ICON          = 0x13,
	CTRL_TEXT_SPEED         = 0x14,
	CTRL_BACKGROUND         = 0x15,
	CTRL_MARATHON_TIME      = 0x16,
	CTRL_RACE_TIME          = 0x17,
	CTRL_POINTS             = 0x18,
	CTRL_TOKENS             = 0x19,
	CTRL_UNSKIPPABLE        = 0x1A,
	CTRL_TWO_CHOICE         = 0x1B,
	CTRL_THREE_CHOICE       = 0x1C,
	CTRL_FISH_INFO          = 0x1D,
	CTRL_HIGHSCORE          = 0x1E,
	CTRL_TIME               = 0x1F,
} ZeldaTextCtrl;

static void Text_Print(FILE* file, void32 segment) {
	u8* txt = TextSegment(segment);
	u32 wow = 0;
	
	for (; *txt != CTRL_END; txt++) {
		switch (*txt) {
			case CTRL_NEWLINE:
				if (!wow)
					fprintf(file, " ");
				else
					fprintf(file, "\n[$] ");
				continue;
				break;
			case CTRL_BOX_BREAK:
				fprintf(file, "\n");
				continue;
				break;
			case CTRL_TWO_CHOICE:
				fprintf(file, "\n[@] ");
				wow = 1;
				continue;
				break;
			case CTRL_THREE_CHOICE:
				fprintf(file, "\n[#] ");
				wow = 1;
				continue;
				break;
			case CTRL_QUICKTEXT_ENABLE:
			case CTRL_QUICKTEXT_DISABLE:
			case CTRL_PERSISTENT:
			case CTRL_EVENT:
			case CTRL_NAME:
			case CTRL_OCARINA:
			case CTRL_MARATHON_TIME:
			case CTRL_RACE_TIME:
			case CTRL_POINTS:
			case CTRL_TOKENS:
			case CTRL_UNSKIPPABLE:
			case CTRL_FISH_INFO:
			case CTRL_TIME:
				continue;
				break;
				
			case CTRL_COLOR:
			case CTRL_SHIFT:
			case CTRL_BOX_BREAK_DELAYED:
			case CTRL_FADE:
			case CTRL_ITEM_ICON:
			case CTRL_TEXT_SPEED:
			case CTRL_HIGHSCORE:
				txt++;
				continue;
				break;
			case CTRL_TEXTID:
			case CTRL_FADE2:
			case CTRL_SFX:
				txt += 2;
				continue;
				break;
			case CTRL_BACKGROUND:
				txt += 3;
				continue;
				break;
		}
		
		switch (*txt) {
			case 'A' ... 'Z':
				fprintf(file, "%c", tolower(*txt));
				break;
			case 'a' ... 'z':
			case '0' ... '9':
			case ' ':
			case ',':
			case '.':
			case '?':
			case '!':
			case '-':
			case '\'':
				fprintf(file, "%c", *txt);
				break;
		}
	}
	fprintf(file, "\n");
}

void Text_Dump(Rom* rom) {
	MessageTableEntry* msgTbl = (MessageTableEntry*)SegmentedToVirtual(0, 0xBC24C0);
	FILE* file;
	
	if (!gTextFlag)
		return;
	
	FileSys_Path("rom/system/text/%s/", gVanilla);
	
	file = fopen(FileSys_File("MessageTable.txt"), "w");
	SetSegment(6, SegmentedToVirtual(0, 0x8C6000));
	
	for (; msgTbl->textId != 0xFFFF; msgTbl++) {
		fprintf(file, "[%d-%d]\n", msgTbl->textId, msgTbl->typePos);
		Text_Print(file, msgTbl->segment);
	}
	
	fclose(file);
	
	MemFile mem;
	
	MemFile_LoadFile_String(&mem, FileSys_File("MessageTable.txt"));
	
	while (StrRep(mem.str, "  ", " ")) (void)0;
	
	mem.dataSize = strlen(mem.str);
	MemFile_SaveFile_String(&mem, mem.info.name);
}

void Text_Build(Rom* rom) {
	MessageTableEntry* msgTbl = (MessageTableEntry*)SegmentedToVirtual(SEG_CODE, 0xBC24C0 - RELOC_CODE);
	MemFile mem;
	char* line;
	
	struct {
		u8 val[4];
		u8 size;
		u8 quick;
	} msgEnd[2117] = { 0 };
	
	if (!Sys_Stat("rom/system/text/MessageTable.txt")) {
		return;
	}
	
	MemFile_LoadFile_String(&mem, "rom/system/text/MessageTable.txt");
	
	SetSegment(6, &rom->file.cast.u8[0x8C6000]);
	MemFile_Seek(&rom->file, 0x8C6000);
	
	printf_info("Text");
	for (s32 i = 0; i < 2117; i++) {
		char* txt = SegmentedToVirtual(6, msgTbl[i].segment - 0x07000000);
		
		if (msgTbl[i].segment == 0)
			continue;
		
		Log("%d SEG %08X RM %08X", i, msgTbl[i].segment, txt);
		for (s32 j = 0;; j++) {
			if (txt[j] == CTRL_END) {
				msgEnd[i].val[0] = CTRL_END;
				msgEnd[i].size = 1;
				break;
			}
			
			if (txt[j] == CTRL_QUICKTEXT_ENABLE)
				msgEnd[i].quick = 1;
			
			if (
				txt[j] == CTRL_EVENT ||
				txt[j] == CTRL_OCARINA ||
				txt[j] == CTRL_PERSISTENT
			) {
				msgEnd[i].val[0] = txt[j];
				msgEnd[i].val[1] = CTRL_END;
				msgEnd[i].size = 2;
				break;
			}
			
			if (txt[j] == CTRL_TEXTID) {
				msgEnd[i].val[0] = txt[j];
				msgEnd[i].val[1] = txt[j + 1];
				msgEnd[i].val[2] = txt[j + 2];
				msgEnd[i].val[3] = CTRL_END;
				msgEnd[i].size = 4;
				break;
			}
			
			if (txt[j] == CTRL_COLOR ||
				txt[j] == CTRL_SHIFT ||
				txt[j] == CTRL_BOX_BREAK_DELAYED ||
				txt[j] == CTRL_FADE ||
				txt[j] == CTRL_ITEM_ICON ||
				txt[j] == CTRL_TEXT_SPEED ||
				txt[j] == CTRL_HIGHSCORE) {
				j += 1;
				continue;
			}
			
			if (txt[j] == CTRL_FADE2 ||
				txt[j] == CTRL_SFX) {
				j += 2;
				continue;
			}
			
			if (txt[j] == CTRL_BACKGROUND) {
				j += 3;
				continue;
			}
		}
	}
	
	line = mem.str;
	for (s32 i = 0; i < 2117; i++) {
		char* word;
		char* all;
		u32 len = 0;
		u32 ln = 0;
		u32 id = msgTbl[i].textId;
		
		Log("%d - %d %08X", i, msgTbl[i].textId, msgTbl[i].segment);
		
		if (msgTbl[i].segment == 0)
			continue;
		
		line = Line(line, 1);
		
		while (!(line[len] == '\n' && line[len + 1] == '[' && isdigit(line[len + 2]))) len++;
		
		all = HeapMalloc(len + 1);
		memcpy(all, line, len);
		ln = LineNum(all);
		
		Log("msg [%s]", all);
		msgTbl[i].segment = VirtualToSegmented(6, &rom->file.cast.u8[rom->file.seekPoint]);
		
		if (strlen(all) == 0) goto end;
		Log("Write Msg\n");
		
		if (msgEnd[i].quick) {
			char val = CTRL_QUICKTEXT_ENABLE;
			MemFile_Write(&rom->file, &val, 1);
		}
		
		for (s32 j = 0; j < ln; j++) {
			char brbx = CTRL_BOX_BREAK;
			char* tline = CopyLine(all, j);
			u32 isAns = 0;
			char setColor[] = { CTRL_COLOR, 0x42 };
			char unsetColor[] = { CTRL_COLOR, 0x00 };
			word = HeapMalloc(strlen(tline) * 2);
			strcpy(word, tline);
			
			if (StrRep(word, "[#] ", "}{<&") || StrRep(word, "[#]", "}{<&"))
				isAns++;
			if (StrRep(word, "[@] ", "}{>&&") || StrRep(word, "[@]", "}{>&&"))
				isAns++;
			if (StrRep(word, "[$] ", "&") || StrRep(word, "[$]", "&"))
				isAns++;
			
			if (!isAns) {
				u32 breaker = 0;
				for (s32 k = 0; k < strlen(word); k++) {
					if (word[k] == '<' || word[k] == '>')
						break;
					
					if (k != 0 && !(k % 33)) {
						s32 l = k;
						for (; word[l - 1] != ' '; l--) (void)0;
						
						if (breaker != 0 && !(breaker % 4))
							StrIns2(word, "^", l, strlen(word) + 1);
						else
							StrIns2(word, "&", l, strlen(word) + 1);
						breaker++;
					}
				}
			}
			
			u32 sz = strlen(word);
			
			while (MemMem(word, sz, "^", 1))
				((char*)MemMem(word, sz, "^", 1))[0] = CTRL_BOX_BREAK;
			while (MemMem(word, sz, "<", 1))
				((char*)MemMem(word, sz, "<", 1))[0] = CTRL_THREE_CHOICE;
			while (MemMem(word, sz, ">", 1))
				((char*)MemMem(word, sz, ">", 1))[0] = CTRL_TWO_CHOICE;
			while (MemMem(word, sz, "&", 1))
				((char*)MemMem(word, sz, "&", 1))[0] = CTRL_NEWLINE;
			if (MemMem(word, sz, "}{", 2))
				memcpy(((char*)MemMem(word, sz, "}{", 2)), setColor, 2);
			
			MemFile_Write(&rom->file, word, sz);
			if (isAns)
				MemFile_Write(&rom->file, unsetColor, 2);
			if (j + 1 < ln)
				MemFile_Write(&rom->file, &brbx, 1);
		}
		
		if (msgEnd[i].quick) {
			char val = CTRL_QUICKTEXT_DISABLE;
			MemFile_Write(&rom->file, &val, 1);
		}
		
end:
		if (msgEnd[i].size)
			MemFile_Write(&rom->file, msgEnd[i].val, msgEnd[i].size);
		
		MemFile_Align(&rom->file, 0x10);
		
		if (id == 65532)
			break;
		line = Line(line, ln);
	}
}