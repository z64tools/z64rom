#include <ExtLib.h>

int main(int argc, char* argv[]) {
	MemFile inp = MemFile_Initialize();
	MemFile out = MemFile_Initialize();
	s32 lineCount;
	char* fileInput, * fileOutput;
	
	if (argc != 3) {
		printf_info("Usage: elf2ld input.ld output.ld");
		
		return 1;
	}
	
	fileInput = argv[1];
	fileOutput = argv[2];
	
	if (MemFile_LoadFile_String(&inp, fileInput)) printf_error("Exiting...");
	MemFile_Malloc(&out, MbToBin(1.00));
	
	lineCount = String_GetLineCount(inp.data);
	for (s32 i = 0; i < lineCount; i++) {
		s32 blanker = 0;
		char* line = String_Line(inp.data, i);
		char* word = String_GetWord(line, 5);
		
		// Cleanup weird dotted things
		for (s32 i = 0; i < strlen(word); i++) {
			if (word[i] == '.')
				blanker = 1;
			if (blanker > 0) {
				word[i] = '\0';
			}
		}
		
		MemFile_Printf(&out, "%s = ", word);
		
		MemFile_Printf(&out, "0x%s;\n", String_GetWord(line, 0));
	}
	
	if (MemFile_SaveFile_String(&out, fileOutput)) printf_error("Exiting...");
	MemFile_Free(&inp);
	MemFile_Free(&out);
	
	return 0;
}