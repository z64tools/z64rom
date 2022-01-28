CFLAGS         := -s -flto -Wall
OPT_WIN32      := -Os
OPT_LINUX      := -Ofast
SOURCE_C       := $(shell find lib/* -type f -name '*.c')
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_RELEASE_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/ndebug/$f)
SOURCE_O_RELEASE_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/ndebug/$f)

PROJECT_TOOLS_C := $(shell find project/tools/* -type f -name '*.c')
PROJECT_TOOLS_APP := $(foreach f,$(PROJECT_TOOLS_C:.c=),$f)

RELEASE_EXECUTABLE_LINUX := z64rom_linux/z64rom
RELEASE_EXECUTABLE_WIN32 := z64rom_win32/z64rom.exe
DEBUG_EXECUTABLE_LINUX   := z64rom_linux/z64rom-debug
DEBUG_EXECUTABLE_WIN32   := z64rom_win32/z64rom-debug.exe

PRNT_DGRY := \e[90;2m
PRNT_GRAY := \e[0;90m
PRNT_DRED := \e[91;2m
PRNT_REDD := \e[0;91m
PRNT_GREN := \e[0;92m
PRNT_YELW := \e[0;93m
PRNT_BLUE := \e[0;94m
PRNT_PRPL := \e[0;95m
PRNT_CYAN := \e[0;96m
PRNT_RSET := \e[m

HEADER := lib/z64rom.h lib/ExtLib.h lib/Audio.h

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(SOURCE_O_WIN32)) \
	$(dir $(SOURCE_O_LINUX)) \
	$(dir $(RELEASE_EXECUTABLE_LINUX)) \
	$(dir $(RELEASE_EXECUTABLE_WIN32)) \
	$(dir $(SOURCE_O_RELEASE_WIN32)) \
	$(dir $(SOURCE_O_RELEASE_LINUX)), $(dir)))
$(shell mkdir -p z64rom_win32/tools/)
$(shell mkdir -p z64rom_win32/lib/)
$(shell mkdir -p z64rom_win32/code/)
$(shell mkdir -p z64rom_linux/tools/)
$(shell mkdir -p z64rom_linux/lib/)
$(shell mkdir -p z64rom_linux/code/)

ifeq (,$(wildcard ../z64audio/Makefile))
	$(shell clone https://github.com/z64tools/z64audio.git ../z64audio)
endif

.PHONY: copyz64audio project-files-linux project-files-win32 clean tools clean-release default win32 linux all-release linux-release win32-release

default: linux
all: linux win32
all-release: linux-release win32-release

project-files-linux: tools project/tools/z64audio
	@cp -r project/patches          z64rom_linux/
	@cp -r project/code             z64rom_linux/
	@rm -f                          z64rom_linux/code/compile_flags.txt
	@cp -R project/*                z64rom_linux/
	@rm -f                          z64rom_linux/*/*.txt
	@rm -f                          z64rom_win32/tools/*.c

project-files-win32: tools project/tools/z64audio
	@cp -r project/patches          z64rom_win32/
	@cp -r project/code             z64rom_win32/
	@rm -f                          z64rom_win32/code/compile_flags.txt
	@cp -R project/*                z64rom_win32/
	@rm -f                          z64rom_win32/*/*.txt
	@rm -f                          z64rom_win32/tools/*.c

project/tools/z64audio: ../z64audio/z64audio.c
	@cd ../z64audio && make all -j
	@cp ../z64audio/z64audio.exe project/tools/z64audio.exe
	@cp ../z64audio/z64audio     project/tools/z64audio
	
linux:         project-files-linux $(SOURCE_O_RELEASE_LINUX) $(DEBUG_EXECUTABLE_LINUX)
linux-release: project-files-linux $(SOURCE_O_RELEASE_LINUX) $(RELEASE_EXECUTABLE_LINUX)
	
win32:         project-files-win32 $(SOURCE_O_RELEASE_WIN32) $(DEBUG_EXECUTABLE_WIN32)
win32-release: project-files-win32 $(SOURCE_O_RELEASE_WIN32) $(RELEASE_EXECUTABLE_WIN32)

clean:
	rm -f $(shell find bin/* -type f)
	rm -f $(shell find z64ro* -type f -not -name '*.c')
	rm -f -R bin
	rm -f $(PROJECT_TOOLS_APP)
	rm -f -R z64rom_linux
	rm -f -R z64rom_win32
	rm -f project/tools/z64audio
	rm -f project/tools/z64audio.exe

tools: $(PROJECT_TOOLS_APP)

project/tools/%: project/tools/%.c
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -o $@ $< lib/ExtLib.c $(OPT_LINUX) $(CFLAGS) -DNDEBUG
# LINUX
bin/linux/ndebug/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) -DNDEBUG
	
bin/linux/ndebug/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) -DNDEBUG -Wno-missing-braces

bin/linux/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS)
	
bin/linux/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS) -Wno-missing-braces

$(RELEASE_EXECUTABLE_LINUX): z64rom.c $(SOURCE_O_RELEASE_LINUX)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(OPT_LINUX) $(CFLAGS) -DNDEBUG -lm
#	@upx -9 --lzma $@ > /dev/null

$(DEBUG_EXECUTABLE_LINUX): z64rom.c $(SOURCE_O_LINUX)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(OPT_LINUX) $(CFLAGS) -lm

# WINDOWS32
bin/win32/ndebug/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -DNDEBUG -D_WIN32
	
bin/win32/ndebug/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -DNDEBUG -D_WIN32 -Wno-missing-braces
	
bin/win32/%.o: %.c %.h $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32
	
bin/win32/%.o: %.c $(HEADER)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32 -Wno-missing-braces

bin/icon.o: lib/icon.rc lib/icon.ico
	@i686-w64-mingw32.static-windres -o $@ $<

$(RELEASE_EXECUTABLE_WIN32): z64rom.c bin/icon.o $(SOURCE_O_RELEASE_WIN32)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(OPT_WIN32) $(CFLAGS) -DNDEBUG -lm -D_WIN32
#	@upx -9 --lzma $@ > /dev/null

$(DEBUG_EXECUTABLE_WIN32): z64rom.c bin/icon.o $(SOURCE_O_WIN32)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(OPT_WIN32) $(CFLAGS) -lm -D_WIN32