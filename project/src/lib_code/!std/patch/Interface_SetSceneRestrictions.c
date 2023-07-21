#include <uLib.h>
#include "code/z_demo.h"

/*
   z64ram = 0x8008407C
   z64rom = 0xAFB21C
   z64next = 0x80084298
 */

typedef struct {
    /* 0x00 */ u8 scene;
    /* 0x01 */ u8 flags1;
    /* 0x02 */ u8 flags2;
    /* 0x03 */ u8 flags3;
} RestrictionFlags;

typedef struct {
    u8 sceneIndex;
    struct {
        u8 unused  : 2;
        u8 bButton : 2;
        u8 aButton : 2;
        u8 bottles : 2;
    };
    struct {
        u8 tradeItem : 2;
        u8 hookshot  : 2;
        u8 ocarina   : 2;
        u8 warpSong  : 2;
    };
    struct {
        u8 sunSong : 2;
        u8 farore  : 2;
        u8 nayru   : 1;
        u8 din     : 1;
        u8 all     : 2;
    };
} RestrictionFlag;

void Interface_SetSceneRestrictions(PlayState* play) {
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    // Relocated to old "gActorOverlayTable"
    RestrictionFlag* newResFlagTbl = (void*)0x801162A0;
    s32 i;
    
    memset(&interfaceCtx->restrictions, 0, 12);
    
    for (i = 0; ; i++) {
        if (newResFlagTbl[i].sceneIndex == 0xFF)
            return;
        if (newResFlagTbl[i].sceneIndex == play->sceneId)
            break;
    }
    
    interfaceCtx->restrictions.hGauge =
        newResFlagTbl[i].unused;
    
    interfaceCtx->restrictions.bButton =
        newResFlagTbl[i].bButton;
    
    interfaceCtx->restrictions.aButton =
        newResFlagTbl[i].aButton;
    
    interfaceCtx->restrictions.bottles =
        newResFlagTbl[i].bottles;
    
    interfaceCtx->restrictions.tradeItems =
        newResFlagTbl[i].tradeItem;
    
    interfaceCtx->restrictions.hookshot =
        newResFlagTbl[i].hookshot;
    
    interfaceCtx->restrictions.ocarina =
        newResFlagTbl[i].ocarina;
    
    interfaceCtx->restrictions.warpSongs =
        newResFlagTbl[i].warpSong;
    
    interfaceCtx->restrictions.sunsSong =
        newResFlagTbl[i].sunSong;
    
    interfaceCtx->restrictions.farores =
        newResFlagTbl[i].farore;
    
    interfaceCtx->restrictions.dinsNayrus =
        newResFlagTbl[i].din + newResFlagTbl[i].nayru;
    
    interfaceCtx->restrictions.all =
        newResFlagTbl[i].all;
}
