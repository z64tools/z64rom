CFLAGS         := -s -flto -Wall
OPT_WIN32      := -Os
OPT_LINUX      := -Ofast
SOURCE_C       := $(shell find lib/* -type f -name '*.c')
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_RELEASE_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/ndebug/$f)
SOURCE_O_RELEASE_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/ndebug/$f)

RELEASE_EXECUTABLE_LINUX := bin/release-linux/z64rom
RELEASE_EXECUTABLE_WIN32 := bin/release-win32/z64rom.exe

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
$(shell mkdir -p tools/)

ifeq (,$(wildcard ../z64audio/Makefile))
	$(shell clone https://github.com/z64tools/z64audio.git ../z64audio)
endif

.PHONY: copyz64audio clean default win32 linux all-release linux-release win32-release

default: linux
all: linux win32
all-release: tools/z64audio linux-release win32-release
linux: tools/z64audio $(SOURCE_O_LINUX) z64rom
win32: tools/z64audio $(SOURCE_O_WIN32) bin/icon.o z64rom.exe

tools/z64audio: ../z64audio/z64audio.c
	@cd ../z64audio && make all -j
	@cp ../z64audio/z64audio.exe tools/z64audio.exe
	@cp ../z64audio/z64audio tools/z64audio

linux-release: tools/z64audio $(SOURCE_O_RELEASE_LINUX) $(RELEASE_EXECUTABLE_LINUX)
	@rm -f z64rom-linux.7z
	@cp -r tools/ bin/release-linux/
	@cp -r patches/ bin/release-linux/
	@rm -f bin/release-linux/tools/z64audio.exe
	@cp bin/release-linux/z64rom z64rom
	@7z a z64rom-linux.7z ./bin/release-linux/* > /dev/null

win32-release: tools/z64audio $(SOURCE_O_RELEASE_WIN32) $(RELEASE_EXECUTABLE_WIN32)
	@rm -f z64rom-win32.7z
	@cp -r tools/ bin/release-win32/
	@cp -r patches/ bin/release-win32/
	@rm -f bin/release-win32/tools/z64audio
	@cp bin/release-win32/z64rom.exe z64rom.exe
	@7z a z64rom-win32.7z ./bin/release-win32/* > /dev/null

clear:
	@rm -f -R rom/*
	@rm -f tools/z64audio
	@rm -f tools/z64audio.exe

clean:
	@echo "$(PRNT_RSET)rm $(PRNT_RSET)[$(PRNT_CYAN)$(shell find bin/* -type f)$(PRNT_RSET)]"
	@rm -f $(shell find bin/* -type f)
	@echo "$(PRNT_RSET)rm $(PRNT_RSET)[$(PRNT_CYAN)$(shell find z64ro* -type f -not -name '*.c*')$(PRNT_RSET)]"
	@rm -f $(shell find z64ro* -type f -not -name '*.c')
	@rm -f -R bin/*
	@rm -f tools/*

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

z64rom: z64rom.c $(SOURCE_O_LINUX)
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

z64rom.exe: z64rom.c bin/icon.o $(SOURCE_O_WIN32)
	@echo "$(PRNT_RSET)$(PRNT_RSET)[$(PRNT_CYAN)$(notdir $@)$(PRNT_RSET)] [$(PRNT_CYAN)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(OPT_WIN32) $(CFLAGS) -lm -D_WIN32