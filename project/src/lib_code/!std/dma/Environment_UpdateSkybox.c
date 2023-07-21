#include <z64hdr.h>
#include "code/z_kankyo.h"

/*
   z64ram = 0x8006FC88
   z64rom = 0xAE6E28
 */

typedef struct {
    DmaEntry* file;
    DmaEntry* palette;
} NewSkyboxFiles;

extern NewSkyboxFiles gNewSkyboxFiles[];
asm ("gNewSkyboxFiles = 0x8011FD3C");

void Environment_UpdateSkybox(u8 skyboxId, EnvironmentContext* envCtx, SkyboxContext* skyboxCtx) {
    u32 size;
    u8 i;
    u8 newSkybox1Index = 0xFF;
    u8 newSkybox2Index = 0xFF;
    u8 skyboxBlend = 0;
    
    if (skyboxId == SKYBOX_CUTSCENE_MAP) {
        envCtx->skyboxConfig = 3;
        
        for (i = 0; i < ARRAY_COUNT(gTimeBasedSkyboxConfigs[envCtx->skyboxConfig]); i++) {
            if (
                gSaveContext.skyboxTime >= gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].startTime &&
                (gSaveContext.skyboxTime < gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].endTime ||
                gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].endTime == 0xFFFF)
            ) {
                if (gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].changeSkybox) {
                    envCtx->skyboxBlend = Environment_LerpWeight(
                        gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].endTime,
                        gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].startTime,
                        ((void)0, gSaveContext.skyboxTime)
                        ) *
                        255;
                } else {
                    envCtx->skyboxBlend = 0;
                }
                break;
            }
        }
    } else if (skyboxId == SKYBOX_NORMAL_SKY && !envCtx->skyboxDisabled) {
        for (i = 0; i < ARRAY_COUNT(gTimeBasedSkyboxConfigs[envCtx->skyboxConfig]); i++) {
            if (
                gSaveContext.skyboxTime >= gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].startTime &&
                (gSaveContext.skyboxTime < gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].endTime ||
                gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].endTime == 0xFFFF)
            ) {
                newSkybox1Index = gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].skybox1Index;
                newSkybox2Index = gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].skybox2Index;
                gSkyboxIsChanging = gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].changeSkybox;
                
                if (gSkyboxIsChanging) {
                    skyboxBlend = Environment_LerpWeight(
                        gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].endTime,
                        gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].startTime,
                        ((void)0, gSaveContext.skyboxTime)
                        ) *
                        255;
                } else {
                    skyboxBlend = Environment_LerpWeight(
                        gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].endTime,
                        gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].startTime,
                        ((void)0, gSaveContext.skyboxTime)
                        ) *
                        255;
                    
                    skyboxBlend = (skyboxBlend < 0x80) ? 0xFF : 0;
                    
                    if (
                        (envCtx->changeSkyboxState != CHANGE_SKYBOX_INACTIVE) &&
                        (envCtx->changeSkyboxState < CHANGE_SKYBOX_ACTIVE)
                    ) {
                        envCtx->changeSkyboxState++;
                        skyboxBlend = 0;
                    }
                }
                break;
            }
        }
        
        Environment_UpdateStorm(envCtx, skyboxBlend);
        
        if (envCtx->changeSkyboxState >= 3) {
            newSkybox1Index = gTimeBasedSkyboxConfigs[envCtx->skyboxConfig][i].skybox1Index;
            newSkybox2Index = gTimeBasedSkyboxConfigs[envCtx->changeSkyboxNextConfig][i].skybox2Index;
            
            skyboxBlend = ((f32)envCtx->changeDuration - envCtx->changeSkyboxTimer--) / (f32)envCtx->changeDuration * 255;
            
            if (envCtx->changeSkyboxTimer <= 0) {
                envCtx->changeSkyboxState = 0;
                envCtx->skyboxConfig = envCtx->changeSkyboxNextConfig;
            }
        }
        
        if ((envCtx->skybox1Index != newSkybox1Index) && (envCtx->skyboxDmaState == SKYBOX_DMA_INACTIVE)) {
            envCtx->skyboxDmaState = SKYBOX_DMA_TEXTURE1_START;
            size = gNewSkyboxFiles[newSkybox1Index].file->vromEnd - gNewSkyboxFiles[newSkybox1Index].file->vromStart;
            
            osCreateMesgQueue(&envCtx->loadQueue, &envCtx->loadMsg, 1);
            DmaMgr_SendRequest2(
                &envCtx->dmaRequest,
                (u32)skyboxCtx->staticSegments[0],
                gNewSkyboxFiles[newSkybox1Index].file->vromStart,
                size,
                0,
                &envCtx->loadQueue,
                NULL,
                "EnUS",
                1264
            );
            envCtx->skybox1Index = newSkybox1Index;
        }
        
        if ((envCtx->skybox2Index != newSkybox2Index) && (envCtx->skyboxDmaState == SKYBOX_DMA_INACTIVE)) {
            envCtx->skyboxDmaState = SKYBOX_DMA_TEXTURE2_START;
            size = gNewSkyboxFiles[newSkybox2Index].file->vromEnd - gNewSkyboxFiles[newSkybox2Index].file->vromStart;
            
            osCreateMesgQueue(&envCtx->loadQueue, &envCtx->loadMsg, 1);
            DmaMgr_SendRequest2(
                &envCtx->dmaRequest,
                (u32)skyboxCtx->staticSegments[1],
                gNewSkyboxFiles[newSkybox2Index].file->vromStart,
                size,
                0,
                &envCtx->loadQueue,
                NULL,
                "EnUS",
                1281
            );
            envCtx->skybox2Index = newSkybox2Index;
        }
        
        if (envCtx->skyboxDmaState == SKYBOX_DMA_TEXTURE1_DONE) {
            envCtx->skyboxDmaState = SKYBOX_DMA_TLUT1_START;
            
            if ((newSkybox1Index & 1) ^ ((newSkybox1Index & 4) >> 2)) {
                size = gNewSkyboxFiles[newSkybox1Index].palette->vromEnd - gNewSkyboxFiles[newSkybox1Index].palette->vromStart;
                
                osCreateMesgQueue(&envCtx->loadQueue, &envCtx->loadMsg, 1);
                DmaMgr_SendRequest2(
                    &envCtx->dmaRequest,
                    (u32)skyboxCtx->palettes,
                    gNewSkyboxFiles[newSkybox1Index].palette->vromStart,
                    size,
                    0,
                    &envCtx->loadQueue,
                    NULL,
                    "EnUS",
                    1307
                );
            } else {
                size = gNewSkyboxFiles[newSkybox1Index].palette->vromEnd - gNewSkyboxFiles[newSkybox1Index].palette->vromStart;
                osCreateMesgQueue(&envCtx->loadQueue, &envCtx->loadMsg, 1);
                DmaMgr_SendRequest2(
                    &envCtx->dmaRequest,
                    (u32)skyboxCtx->palettes + size,
                    gNewSkyboxFiles[newSkybox1Index].palette->vromStart,
                    size,
                    0,
                    &envCtx->loadQueue,
                    NULL,
                    "EnUS",
                    1320
                );
            }
        }
        
        if (envCtx->skyboxDmaState == SKYBOX_DMA_TEXTURE2_DONE) {
            envCtx->skyboxDmaState = SKYBOX_DMA_TLUT2_START;
            
            if ((newSkybox2Index & 1) ^ ((newSkybox2Index & 4) >> 2)) {
                size = gNewSkyboxFiles[newSkybox2Index].palette->vromEnd - gNewSkyboxFiles[newSkybox2Index].palette->vromStart;
                
                osCreateMesgQueue(&envCtx->loadQueue, &envCtx->loadMsg, 1);
                DmaMgr_SendRequest2(
                    &envCtx->dmaRequest,
                    (u32)skyboxCtx->palettes,
                    gNewSkyboxFiles[newSkybox2Index].palette->vromStart,
                    size,
                    0,
                    &envCtx->loadQueue,
                    NULL,
                    "EnUS",
                    1342
                );
            } else {
                size = gNewSkyboxFiles[newSkybox2Index].palette->vromEnd - gNewSkyboxFiles[newSkybox2Index].palette->vromStart;
                osCreateMesgQueue(&envCtx->loadQueue, &envCtx->loadMsg, 1);
                DmaMgr_SendRequest2(
                    &envCtx->dmaRequest,
                    (u32)skyboxCtx->palettes + size,
                    gNewSkyboxFiles[newSkybox2Index].palette->vromStart,
                    size,
                    0,
                    &envCtx->loadQueue,
                    NULL,
                    "EnUS",
                    1355
                );
            }
        }
        
        if ((envCtx->skyboxDmaState == SKYBOX_DMA_TEXTURE1_START) || (envCtx->skyboxDmaState == SKYBOX_DMA_TEXTURE2_START)) {
            if (osRecvMesg(&envCtx->loadQueue, 0, OS_MESG_NOBLOCK) == 0) {
                envCtx->skyboxDmaState++;
            }
        } else if (envCtx->skyboxDmaState >= SKYBOX_DMA_TEXTURE1_DONE) {
            if (osRecvMesg(&envCtx->loadQueue, 0, OS_MESG_NOBLOCK) == 0) {
                envCtx->skyboxDmaState = SKYBOX_DMA_INACTIVE;
            }
        }
        
        envCtx->skyboxBlend = skyboxBlend;
    }
}
