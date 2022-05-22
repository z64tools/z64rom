#ifndef __ULIB_MACROS_H__
#define __ULIB_MACROS_H__

#define CHK_ALL(AB, combo)      (~((gGlobalContext.state.input[0].AB.button) | ~(combo)) == 0)
#define CHK_ANY(AB, combo)      (((gGlobalContext.state.input[0].AB.button) & (combo)) != 0)
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

#endif