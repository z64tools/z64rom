#include <uLib.h>

/*
   z64ram = 0x800A336C
   z64rom = 0xB1A50C
 */

AnimationEntry* AnimationContext_AddEntry(AnimationContext* animationCtx, AnimationType type);
asm ("AnimationContext_AddEntry = 0x800A3334");

#define NEW_LINK_ANIMATION_OFFSET(addr, offset) \
    ((gDmaDataTable[6].vromStart) + ((u32)addr) - (0x07000000) + ((u32)offset))

void AnimationContext_SetLoadFrame(PlayState* playState, LinkAnimationHeader* animation, s32 frame, s32 limbCount, Vec3s* frameTable) {
    AnimationEntry* entry = AnimationContext_AddEntry(&playState->animationCtx, ANIMENTRY_LOADFRAME);
    
    if (entry != NULL) {
        LinkAnimationHeader* linkAnimHeader = SEGMENTED_TO_VIRTUAL(animation);
        
        osCreateMesgQueue(&entry->data.load.msgQueue, &entry->data.load.msg, 1);
        DmaMgr_SendRequest2(
            &entry->data.load.req,
            (u32)frameTable,
            NEW_LINK_ANIMATION_OFFSET(linkAnimHeader->segment, ((sizeof(Vec3s) * limbCount + 2) * frame)),
            sizeof(Vec3s) * limbCount + 2,
            0,
            &entry->data.load.msgQueue,
            NULL,
            0,
            0
        );
    }
}
