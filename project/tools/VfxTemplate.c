#include "VfxTemplate.h"

u32 VfxTemplate_Init(PlayState*, u32, EffectSs*, void*);
void VfxTemplate_Destroy(PlayState*, u32, VfxTemplate*);
void VfxTemplate_Update(PlayState*, u32, VfxTemplate*);
void VfxTemplate_Draw(PlayState*, u32, VfxTemplate*);

////////////////////////////////////////////////////////////////////////////////

const EffectSsInit VfxTemplate_InitVars = {
    [[VFX_ID_PLACEHOLDER]],
    VfxTemplate_Init,
};

////////////////////////////////////////////////////////////////////////////////

u32 VfxTemplate_Init(PlayState* play, u32 index, EffectSs* effect, void* _params) {
    VfxTemplate* this = (void*)effect;
    VfxTemplateInitParams* init = _params;
    
    this->effect.draw = (void*)VfxTemplate_Draw;
    this->effect.update = (void*)VfxTemplate_Update;
    // life is decremented by the EffectSs system. Once it's zero the
    // particle will be killed.
    this->effect.life = 1;
    
    return 1;
}

void VfxTemplate_Update(PlayState* play, u32 index, VfxTemplate* this) {
}

void VfxTemplate_Draw(PlayState* play, u32 index, VfxTemplate* this) {
}
