#ifndef EFFECT_INIT_ONLY
#ifndef EFFECT_TEMPLATE_H
#define EFFECT_TEMPLATE_H

// z64vfx-magic-word
#include <uLib.h>

typedef union {
    EFFECT_STRUCT_START;
    
    EFFECT_STRUCT_END;
} VfxTemplate;

// Make sure the variables do not exceed the amount of regs
// that are available to effect. Max size is `0x1A` bytes
CASSERT(offsetof(VfxTemplate, __vfx_end) <= offsetof(EffectSs, flags));

#endif // EFFECT_TEMPLATE_H
#endif // EFFECT_INIT_ONLY

////////////////////////////////////////////////////////////////////////////////

#ifndef EFFECT_TEMPLATE_INIT_H
#define EFFECT_TEMPLATE_INIT_H

typedef struct {
} VfxTemplateInitParams;

#endif // EFFECT_TEMPLATE_INIT_H