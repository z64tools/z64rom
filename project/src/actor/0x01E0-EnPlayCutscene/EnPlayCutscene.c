#include "EnPlayCutscene.h"
#include <uLib_vector.h>

void EnPlayCutscene_Init(EnPlayCutscene* this, PlayState* play);
void EnPlayCutscene_Update(EnPlayCutscene* this, PlayState* play);

const ActorInit sEnPlayCutscene_InitVars = {
    .id           = 0x01E0,
    .category     = ACTORCAT_SWITCH,
    .flags        = 0x00000000,
    .objectId     = 0x0001,
    .instanceSize = sizeof(EnPlayCutscene),
    .init         = (ActorFunc)EnPlayCutscene_Init,
    .destroy      = NULL,
    .update       = (ActorFunc)EnPlayCutscene_Update,
    .draw         = NULL
};

static s32 EnPlayCutscene_GetFlag(Flag* flag) {
    PlayState* play = Effect_GetPlayState();
    
    if (flag->val == 0xFF)
        return 2;
    
    switch (flag->type) {
        case FLAG_TYPE_SWITCH:
            return Flags_GetSwitch(play, flag->val) ? 1 : 0;
        case FLAG_TYPE_CHEST:
            return Flags_GetTreasure(play, flag->val) ? 1 : 0;
        case FLAG_TYPE_COLLECTIBLE:
            return Flags_GetCollectible(play, flag->val) ? 1 : 0;
        case FLAG_TYPE_GLOBAL:
            return Flags_GetEventChkInf(flag->val) ? 1 : 0;
    }
    
    return false;
}

static void EnPlayCutscene_SetFlag(Flag* flag) {
    PlayState* play = Effect_GetPlayState();
    
    if (flag->val == 0xFF)
        return;
    
    switch (flag->type) {
        case FLAG_TYPE_SWITCH:
            Flags_SetSwitch(play, flag->val);
            break;
        case FLAG_TYPE_CHEST:
            Flags_SetTreasure(play, flag->val);
            break;
        case FLAG_TYPE_COLLECTIBLE:
            Flags_SetCollectible(play, flag->val);
            break;
        case FLAG_TYPE_GLOBAL:
            Flags_SetEventChkInf(flag->val);
            break;
    }
}

void EnPlayCutscene_Init(EnPlayCutscene* this, PlayState* play) {
    this->setFlag.type = this->actor.params >> 14;
    this->setFlag.val = this->actor.home.rot.x >> 8;
    
    this->depFlag.type = this->actor.params >> 12;
    this->depFlag.val = this->actor.home.rot.x;
    
    this->distance = (this->actor.params & 0x3FF) * 10.0f;
    this->behaviour = (this->actor.params >> 10) & 0b11;
    
    this->headerIndex = this->actor.home.rot.y & 0xF;
    
    if (EnPlayCutscene_GetFlag(&this->setFlag) == 1) {
        Actor_Kill(&this->actor);
        
        return;
    }
}

void EnPlayCutscene_Update(EnPlayCutscene* this, PlayState* play) {
    Player* p = GET_PLAYER(play);
    
    if (EnPlayCutscene_GetFlag(&this->depFlag) == (this->settings & PARAM_FLIP_DEP_FLAG) ? 1 : 0)
        return;
    
    if (!this->queued) {
        if (this->distance) {
            switch (this->distType) {
                case DIST_XZ:
                    if (Vec3f_DistXZ(this->actor.pos, p->actor.pos) > this->distance)
                        return;
                    
                    break;
                case DIST_XYZ:
                    if (Vec3f_DistXYZ(this->actor.pos, p->actor.pos) > this->distance)
                        return;
                    
                    break;
            }
            
        }
        
        this->queued = true;
    }
    
    if (this->queued) {
        void* cutscene;
        if (this->behaviour == BEHAVIOUR_QUEUE)
            if (Player_InCsMode(play))
                return;
        
        if (!(cutscene = Segment_Scene_GetCutscene(play->sceneSegment, this->headerIndex))) {
            osLibPrintf("" PRNT_REDD "\aWARNING: " PRNT_RSET "Cutscene Header Index 0x%X returned NULL!", this->headerIndex);
            Actor_Kill(&this->actor);
            
            return;
        }
        
        EnPlayCutscene_SetFlag(&this->setFlag);
        Cutscene_PlaySegment(play, cutscene);
        Actor_Kill(&this->actor);
    }
}
