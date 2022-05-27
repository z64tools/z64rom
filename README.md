# z64rom

##### For questions and feature requests [join z64tools discord](https://discord.gg/52DgAggYAT)!

##### [Download z64rom here!](https://github.com/z64tools/z64rom/releases)

### Dumping

Copy rom to same directory where you have your z64rom set and run **z64rom.exe**. Alternatively you can keep the folder open and drag your rom from different directory on top of **z64rom.exe** to let it handle the copying and dump it.

### Building

You can do 2 different builds, **build-dev** and **build-release**.

| Build         | How to build                                     | Info                                      |
| ------------- | ------------------------------------------------ | ----------------------------------------- |
| build-dev     | Run **z64rom.exe**                               | Debug/Developement, #define **DEV_BUILD** |
| build-release | Drag'n'drop **z64project.toml** to **z64rom.exe** | Release                                   |

##### build-developement

Has debugging, testing and developement related code enabled. This includes things like _level select_, _collision viewer_, etc.

##### build-release

Has all that's listed in **build-developement** disabled.

### Compressing

To compress your build, drag and drop it on **z64rom.exe**

### Replacing Samples

To replace a sample, copy the sample folder you want to replace from **rom/sound/sample/.vanilla/\*** and paste it in **rom/sound/sample/\***. Copy your audio into this folder copy and you should be done.

##### Supported Audio Formats

mp3, wav, aiff

##### Auto Converting

z64rom will convert the newest audio file it finds from this folder so you do not have to clean old samples from this directory. Also naming does not matter as long as it's one of the formats listed above.

![](readme/z64rom-new-sample.gif)

### Arguments

| Argument          | Action                                            | Example                   |
| ----------------- | ------------------------------------------------- | ------------------------- |
| --zmap            | Renames all `.zroom`s to `.zmap`                  |                           |
| --zroom           | Renames all `.zmap`s to `.zroom`                  |                           |
| --target [option] | Make only `sound / code`                          | `--target sound`          |
| --actor [id]      | Actor info, provide .z64 rom also as an argument  | `--actor 7 oot-debug.z64` |
| --dma [id]        | DMA info, provide .z64 rom also as an argument    | `--dma 7 oot-debug.z64`   |
| --scene [id]      | Scene info, provide .z64 rom also as an argument  | `--scene 7 oot-debug.z64` |
| --info            | Print extra info, DMA entries, rom visualization  |                           |
| --force           | Force compile/convert                             |                           |
| --make-only       | Do not build, only make                           |                           |
| --update          | Update z64hdr                                     |                           |
| --generic         | Use generic names on dump `Sample_001`            |                           |
| --no-threading    | Process only on single thread. Good for debugging |                           |
| --log             | Print Log before closing                          |                           |
| --no-wait         | Do not ask to press enter on exit, if successful  |                           |
| --no-make         | Do not compile/convert                            |                           |
| --no-wav          | Do not dump wavs                                  |                           |
| --no-beta         | Remove all OoT unused assets from the project     |                           |

## Credits

**Documentation:** <br>
DezZival <br>
sklitte22

**Testers:** <br>
Zeldaboy14 <br>
Nokaubure <br>
sklitte22

**Special Thanks:** <br>
Sauraen <br>
Tharo
