#include "z64rom.h"

void Sym(const char* symbol) {
	MemFile uld = MemFile_Initialize();
	char* res;
	u32 oOffset = 0;
	
	MemFile_LoadFile_String(&uld, "include/z_lib_user.ld");
	
	if (Value_ValidateHex(symbol)) {
		u32 offset = oOffset = Value_Hex(symbol);
		
		while (!(res = StrStr(uld.str, xFmt("0x%08x", offset)))) {
			offset--;
			if (offset == 0)
				break;
		}
	} else
		res = StrStrCase(uld.str, symbol);
	
	if (res == NULL)
		printf_warning("Symbol not found!");
	else {
		char* var;
		u32 offset = 0;
		res = LineHead(res, uld.str);
		
		var = CopyWord(res, 0);
		offset = Config_GetInt(&uld, var);
		if (oOffset == offset)
			printf_info("Symbol: "PRNT_YELW "%s " PRNT_BLUE "%08X", var, offset);
		else
			printf_info("Symbol: "PRNT_YELW "%s " PRNT_BLUE "%08X" PRNT_GRAY " + %X", var, offset, oOffset - offset);
	}
}