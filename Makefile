ifeq (,$(wildcard settings.mk))
  $(error Please run ./setup.sh to automatically install ExtLib)
endif
include settings.mk

CFLAGS          = -Ofast -Wall -DEXTLIB=220
SOURCE_C       := $(shell find src/* -type f -name '*.c')
SOURCE_D       := $(shell find src/data* -type f)
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f) $(foreach f,$(SOURCE_D:%=%.o),bin/linux/$f)
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f) $(foreach f,$(SOURCE_D:%=%.o),bin/win32/$f)

RELEASE_EXECUTABLE_LINUX := app_linux/z64rom
RELEASE_EXECUTABLE_WIN32 := app_win32/z64rom.exe
RELEASE_EXECUTABLE_WIN32_GUI := app_win32/z64rom-gui.exe

TOOLS_WIN32      := project/tools/novl.exe       \
					project/tools/z64playas.exe  \
					project/tools/z64audio.exe   \
					project/tools/z64convert.exe \
					project/tools/seqas.exe      \
					project/tools/z64upgrade.exe
TOOLS_LINUX    := $(foreach f,$(TOOLS_WIN32:.exe=),$f)

SOURCE_nOVL_C            := $(shell find tools/nOVL/src/* -type f -name '*.c')

HEADER := src/z64rom.h src/Audio.h

include $(PATH_EXTLIB)/ext_lib.mk

.PHONY: project-files-linux \
	    project-files-win32 \
		default \
		linux \
		win32 \
		all-release \
		linux-release \
		win32-release \
		clean-all \
		clean-sub \
		clean-release \
		clean \
		update-z64audio

default: linux
all: linux win32
	
PROJECT_FILES_L      = project/.vscode/c_cpp_properties.json $(shell find project/* -type f -not -name '*.exe' -not -name '*.reg')
PROJECT_FILES_W      = project/.vscode/c_cpp_properties.json $(shell find project/* -type f -not -iname 'z64convert' -not -iname 'z64audio' -not -iname 'novl' -not -iname 'seq64_console' -not -iname 'seqas')
PROJECT_FILES_L     += $(TOOLS_LINUX)
PROJECT_FILES_W     += $(TOOLS_WIN32)
PROJECT_FILES_LINUX := $(foreach f,$(PROJECT_FILES_L:project/%=%),app_linux/$f)
PROJECT_FILES_WIN32 := $(foreach f,$(PROJECT_FILES_W:project/%=%),app_win32/$f)

update:
	@cd tools/z64audio/ && git pull https://github.com/z64tools/z64audio main && cd ../..
	@cd tools/z64playas/ && git pull https://github.com/z64tools/z64playas main && cd ../..
	@cd tools/n64log/ && git pull https://github.com/Dragorn421/IS64-logging main && cd ../..

linux: project-files-linux $(RELEASE_EXECUTABLE_LINUX)
win32: project-files-win32 $(RELEASE_EXECUTABLE_WIN32)
win32gui: win32 $(RELEASE_EXECUTABLE_WIN32_GUI)

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(RELEASE_EXECUTABLE_LINUX)) \
	$(dir $(PROJECT_FILES_LINUX)) \
	$(dir $(SOURCE_O_LINUX)) \
	\
	$(dir $(RELEASE_EXECUTABLE_WIN32)) \
	$(dir $(PROJECT_FILES_WIN32)) \
	$(dir $(SOURCE_O_WIN32)) \
	, $(dir)))
$(shell mkdir -p app_linux/tools/)
$(shell mkdir -p app_win32/tools/)
$(shell cp -u settings.mk tools/z64audio/)
$(shell cp -u settings.mk tools/z64playas/)
$(shell cp -u tools/n64log/IS64Log_DebugClient_Raw.py project/tools/n64log.py)
$(shell cp -u tools/n64log/IS64Log_DebugServer_Raw.js project/tools/n64log.js)

# # # # # # # # # # # # # # # # # # # #
# PROJECT                             #
# # # # # # # # # # # # # # # # # # # #

project-files-linux: $(TOOLS_LINUX) $(PROJECT_FILES_LINUX)
project-files-win32: $(TOOLS_WIN32) $(PROJECT_FILES_WIN32)

app_linux/tools/n64log.js: project/tools/n64log.js
	@echo "$(PRNT_RSET)[copy $(PRNT_BLUE)$@$(PRNT_RSET)]"
	@cp -u $< $@
	@patch -t -s $@ n64patch.diff
app_win32/tools/n64log.js: project/tools/n64log.js
	@echo "$(PRNT_RSET)[copy $(PRNT_BLUE)$@$(PRNT_RSET)]"
	@cp -u $< $@
	@patch -t -s $@ n64patch.diff

app_linux/%: project/%
	@echo "$(PRNT_RSET)[copy $(PRNT_BLUE)$@$(PRNT_RSET)]"
	@cp -u $< $@
app_win32/%: project/%
	@echo "$(PRNT_RSET)[copy $(PRNT_BLUE)$@$(PRNT_RSET)]"
	@cp -u $< $@

# # # # # # # # # # # # # # # # # # # #
# CLEAN                               #
# # # # # # # # # # # # # # # # # # # #

clean-all: clean-sub clean-release clean

clean-sub:
	@$(MAKE) -C tools/z64audio clean --no-print-directory
	rm -f $(TOOLS_WIN32) $(TOOLS_LINUX)

clean:
	rm -rf bin

clean-release:
	rm -rf app_linux
	rm -rf app_win32
	
# # # # # # # # # # # # # # # # # # # #
# TOOLS                               #
# # # # # # # # # # # # # # # # # # # #

project/tools/novl: $(SOURCE_nOVL_C)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -o $@ -s -Os -DNOVL_DEBUG=1 -flto $^ -Wno-unused-result `pkg-config --cflags --libs libelf glib-2.0`
project/tools/novl.exe: $(SOURCE_nOVL_C)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ tools/novl/src/*.c -Wall -Wno-unused-const-variable -DNDEBUG -Os -s -flto `i686-w64-mingw32.static-pkg-config --cflags --libs glib-2.0` -luuid -Itools/novl/libelf -D__LIBELF_INTERNAL__=1 -DHAVE_MEMCPY=1 -DHAVE_MEMCMP=1 -DHAVE_MEMMOVE=1 -DSTDC_HEADERS=1 tools/novl/libelf/*.c

project/tools/z64audio: # $(ExtLib_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64audio linux -j --no-print-directory --silent
	@cp tools/z64audio/z64audio $@
project/tools/z64audio.exe: # $(ExtLib_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64audio win32 -j --no-print-directory --silent
	@cp tools/z64audio/z64audio.exe $@

project/tools/z64playas: # $(ExtLib_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64playas linux -j --no-print-directory --silent
	@cp tools/z64playas/app_linux/z64playas $@
project/tools/z64playas.exe: # $(ExtLib_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64playas win32 -j --no-print-directory --silent
	@cp tools/z64playas/app_win32/z64playas.exe $@
	
project/tools/z64convert: tools/z64convert/src/z64convert.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64convert lincli -j --no-print-directory --silent
	@cp tools/z64convert/z64convert-cli $@
project/tools/z64convert.exe: tools/z64convert/src/z64convert.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@$(MAKE) -C tools/z64convert wincli -j --no-print-directory --silent
	@cp tools/z64convert/z64convert-cli.exe $@
	
project/tools/seqas: tools/seqas.cpp
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@g++ $^ -o $@ -I tools/ -Wall -Wextra -pedantic -std=c++17 -g -O2 -lstdc++ -lm
project/tools/seqas.exe: tools/seqas.cpp
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-g++ $^ -o $@ -I tools/ -Wall -Wextra -pedantic -std=c++17 -g -O2 -lstdc++ -lm

project/tools/z64upgrade: tools/z64upgrade.c $(ExtLib_Linux_O) $(Zip_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc $^ -o $@ $(XFLAGS) $(CFLAGS)
project/tools/z64upgrade.exe: tools/z64upgrade.c $(ExtLib_Win32_O) $(Zip_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc $^ -o $@ $(XFLAGS) $(CFLAGS)


-include $(SOURCE_O_LINUX:.o=.d)
-include $(SOURCE_O_WIN32:.o=.d)

# # # # # # # # # # # # # # # # # # # #
# LINUX BUILD                         #
# # # # # # # # # # # # # # # # # # # #

$(RELEASE_EXECUTABLE_LINUX): $(SOURCE_O_LINUX) $(ExtLib_Linux_O) $(Zip_Linux_O) $(Xm_Linux_O) $(Audio_Linux_O) $(Proc_Linux_O) $(Image_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(XFLAGS) $(CFLAGS)
#	@strip --strip-unneeded $@

# # # # # # # # # # # # # # # # # # # #
# WIN32 BUILD                         #
# # # # # # # # # # # # # # # # # # # #

$(RELEASE_EXECUTABLE_WIN32): $(SOURCE_O_WIN32) $(ExtLib_Win32_O) $(Zip_Win32_O) $(Xm_Win32_O) $(Audio_Win32_O) $(Proc_Win32_O) $(Image_Win32_O) bin/win32/icon.o
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(XFLAGS) $(CFLAGS) -D_WIN32
	@i686-w64-mingw32.static-strip --strip-unneeded $@
