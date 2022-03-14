#include "../src/ExtLib.h"

MemFile sHeadFile;
MemFile sInclFile;
char* sIncPath = "include/";

#define getTime st_mtim.tv_sec

s32 main(s32 argc, char* argv[]) {
	char* ptr;
	char* word;
	
	printf_SetPrefix("");
	printf_SetSuppressLevel(PSL_NONE);
	if (argc < 2)
		exit(0);
	
	for (s32 i = 1; i < argc - 1; i++) {
		if (MemFile_LoadFile_String(&sHeadFile, argv[i])) {
			printf_error("Could not load file [%s]", argv[i]);
		}
		
		ptr = StrStr(sHeadFile.data, "#include");
		
		while (1) {
			word = String_GetWord(ptr, 1);
			if (word[0] == '"') {
				ptr = StrStr(ptr + 1, "#include");
				if (ptr == NULL)
					break;
				
				continue;
			}
			word++;
			word[strlen(word) - 1] = '\0';
			
			{
				char path[256 * 2];
				
				sprintf(path, "%s%s", sIncPath, word);
				MemFile_LoadFile_String(&sInclFile, path);
				
				if (sInclFile.info.age > sHeadFile.info.age) {
					if (Touch(argv[i])) {
						printf_error("Touch Failed");
					}
					printf_info("  Touched [%s]", argv[i]);
					
					break;
				}
			}
			
			ptr = StrStr(ptr + 1, "#include");
			
			if (ptr == NULL)
				break;
		}
	}
	
	MemFile_Free(&sHeadFile);
	MemFile_Free(&sInclFile);
	
	return 0;
}