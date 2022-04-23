#include <ExtLib.h>

MemFile new;
MemFile old;

void Exit() {
	MemFile_Free(&new);
	MemFile_Free(&old);
	
	exit(0);
}

void UseNew(char* new, char* old) {
	printf_info("New [%s]", old);
	if (remove(old))
		printf_error("Could not remove [%s]", old);
	if (rename(new, old))
		printf_error("Could not rename [%s]", new);
	
	Exit();
}

void UseOld(char* new, char* old) {
	printf_info("Old [%s]", old);
	if (remove(new))
		printf_error("Could not remove [%s]", new);
	
	Exit();
}

s32 main(s32 argc, char* argv[]) {
	printf_SetPrefix("");
	printf_SetSuppressLevel(PSL_NO_WARNING);
	if (argc != 3)
		printf_error("Segmentation Fault");
	
	if (MemFile_LoadFile_String(&new, argv[1])) {
		printf_error("Could not load new file");
	}
	if (MemFile_LoadFile_String(&old, argv[2])) {
		if (rename(argv[1], argv[2]))
			printf_error("Could not rename [%s]", new);
		
		printf_info("Set [%s]", argv[2]);
		
		Exit();
	}
	
	if (new.dataSize != old.dataSize) {
		UseNew(argv[1], argv[2]);
	}
	
	for (s32 i = 0; i < new.dataSize; i++) {
		if (new.cast.s8[i] != old.cast.s8[i]) {
			UseNew(argv[1], argv[2]);
		}
	}
	
	UseOld(argv[1], argv[2]);
	
	return 0;
}