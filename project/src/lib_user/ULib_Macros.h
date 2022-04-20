#ifndef __ULIB_MACROS_H__
#define __ULIB_MACROS_H__

#define CHK_ALL(AB, combo)      (~((gGlobalContext.state.input[0].AB.button) | ~(combo)) == 0)
#define CHK_ANY(AB, combo)      (((gGlobalContext.state.input[0].AB.button) & (combo)) != 0)
#define AVAL(base, type, value) ((type*)((u8*)base + value))

#endif