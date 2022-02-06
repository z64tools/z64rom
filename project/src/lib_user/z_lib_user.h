#include <oot_mq_debug/z64hdr.h>
#include <Color/Color.h>
#include <Math/Math.h>

typedef struct {
	u32 myMagicValue;
} LibContext;

extern LibContext gLibCtx;

void ULib_Update(GameState* gameState);

#define CHK_ALL(AB, combo)      (~((gGlobalContext.state.input[0].AB.button) | ~(combo)) == 0)
#define CHK_ANY(AB, combo)      (((gGlobalContext.state.input[0].AB.button) & (combo)) != 0)
#define AVAL(base, type, value) ((type*)((u8*)base + value))