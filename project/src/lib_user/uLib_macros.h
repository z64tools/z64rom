#ifndef __ULIB_MACROS_H__
#define __ULIB_MACROS_H__

#ifdef gDPSetTileCustom
#undef gDPSetTileCustom
#endif

#define Asm_VanillaHook(func)                    \
    asm (".global " "__vanilla_hook_" #func "\n" \
    "__vanilla_hook_" #func " = " #func)

#define CS_CMD_EXITPARAM 0xDE00

#define CHK_ALL(AB, combo)      (~((gPlayState.state.input[0].AB.button) | ~(combo)) == 0)
#define CHK_ANY(AB, combo)      (((gPlayState.state.input[0].AB.button) & (combo)) != 0)
#define AVAL(base, type, value) ((type*)((u8*)base + value))

#define PRNT_DGRY "\e[90;2m"
#define PRNT_DRED "\e[91;2m"
#define PRNT_GRAY "\e[0;90m"
#define PRNT_REDD "\e[0;91m"
#define PRNT_GREN "\e[0;92m"
#define PRNT_YELW "\e[0;93m"
#define PRNT_BLUE "\e[0;94m"
#define PRNT_PRPL "\e[0;95m"
#define PRNT_CYAN "\e[0;96m"
#define PRNT_RSET "\e[m"

#define BinToMb(x) ((f32)(x) / (f32)0x100000)
#define BinToKb(x) ((f32)(x) / (f32)0x400)
#define MbToBin(x) (u32)(0x100000 * (x))
#define KbToBin(x) (u32)(0x400 * (x))

#define UnfoldVec3f(vec) (vec).x, (vec).y, (vec).z

#define STRUCT_ALIGN16 __attribute__((aligned(16)))

#define Align(val, align) ((((val) % (align)) != 0) ? (val) + (align) - ((val) % (align)) : (val))

#define U32_RGB(x) (u8)(x >> 24), (u8)(x >> 16), (u8)(x >> 8)

#define gDPSetTileCustom(pkt, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    do {                                                                                            \
        gDPPipeSync(pkt);                                                                           \
        gDPTileSync(pkt);                                                                           \
        gDPSetTile(                                                                                 \
            pkt,                                                                                    \
            fmt,                                                                                    \
            siz,                                                                                    \
            (((width) * siz) + 7) >> 3,                                                             \
            0,                                                                                      \
            G_TX_LOADTILE,                                                                          \
            0,                                                                                      \
            cmt,                                                                                    \
            maskt,                                                                                  \
            shiftt,                                                                                 \
            cms,                                                                                    \
            masks,                                                                                  \
            shifts                                                                                  \
        );                                                                                          \
        gDPTileSync(pkt);                                                                           \
        gDPSetTile(                                                                                 \
            pkt,                                                                                    \
            fmt,                                                                                    \
            siz,                                                                                    \
            (((width) * siz) + 7) >> 3,                                                             \
            0,                                                                                      \
            G_TX_RENDERTILE,                                                                        \
            pal,                                                                                    \
            cmt,                                                                                    \
            maskt,                                                                                  \
            shiftt,                                                                                 \
            cms,                                                                                    \
            masks,                                                                                  \
            shifts                                                                                  \
        );                                                                                          \
        gDPSetTileSize(                                                                             \
            pkt,                                                                                    \
            G_TX_RENDERTILE,                                                                        \
            0,                                                                                      \
            0,                                                                                      \
            ((width) - 1) << 2,                                                                     \
                ((height) - 1) << 2                                                                 \
        );                                                                                          \
    } while (0)

// compile-time assert
#define CASSERT(predicate) _impl_CASSERT_LINE(predicate, __LINE__)
#define _impl_PASTE(a, b)  a ## b
#define _impl_CASSERT_LINE(predicate, line) \
    typedef char (_impl_PASTE (CASSERT, line))[2 * !!(predicate) - 1]

#define VFX_STRUCT_START \
    EffectSs effect;     \
    struct {             \
        u8 __vfx_pad[0x40];

#define VFX_STRUCT_END \
    s16 __vfx_end;     \
    };

#endif