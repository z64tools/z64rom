#include "ActorTemplate.h"

void EnActor_Init(EnActor* this, PlayState* play);
void EnActor_Destroy(EnActor* this, PlayState* play);
void EnActor_Update(EnActor* this, PlayState* play);
void EnActor_Draw(EnActor* this, PlayState* play);

const ActorInit sEnActor_InitVars = {
    [[ACTOR_ID_PLACEHOLDER]]
    .category     = ACTORCAT_BG,
    .flags        = 0x00000000,
    [[OBJECT_ID_PLACEHOLDER]]
    .instanceSize = sizeof(EnActor),
    .init         = (ActorFunc)EnActor_Init,
    .destroy      = (ActorFunc)EnActor_Destroy,
    .update       = (ActorFunc)EnActor_Update,
    .draw         = (ActorFunc)EnActor_Draw
};

void EnActor_Init(EnActor* this, PlayState* play) {
}

void EnActor_Destroy(EnActor* this, PlayState* play) {
}

void EnActor_Update(EnActor* this, PlayState* play) {
}

s32 EnActor_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dl, Vec3f* pos, Vec3s* rot, void* thisx) {
    EnActor* this = (void*)thisx;
    
    return 0;
}

void EnActor_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dl, Vec3s* rot, void* thisx) {
    EnActor* this = (void*)thisx;
}

void EnActor_Draw(EnActor* this, PlayState* play) {
}
