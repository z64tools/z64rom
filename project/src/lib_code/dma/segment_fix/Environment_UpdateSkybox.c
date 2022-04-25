#include <oot_mq_debug/z64hdr.h>

/*
   z64ram = 0x8006FC88
   z64rom = 0xAE6E28
 */

typedef struct {
	DmaEntry* file;
	DmaEntry* palette;
} NewSkyboxFiles;

void func_8006FB94(EnvironmentContext* envCtx, u8 unused);

extern NewSkyboxFiles gNewSkyboxFiles[];
asm ("gNewSkyboxFiles = 0x8011FD3C");

void Environment_UpdateSkybox(u8 skyboxId, EnvironmentContext* envCtx, SkyboxContext* skyboxCtx) {
	u32 size;
	u8 i;
	u8 newSkybox1Index = 0xFF;
	u8 newSkybox2Index = 0xFF;
	u8 skyboxBlend = 0;
	
	if (skyboxId == SKYBOX_CUTSCENE_MAP) {
		envCtx->unk_17 = 3;
		
		for (i = 0; i < ARRAY_COUNT(D_8011FC1C[envCtx->unk_17]); i++) {
			if (gSaveContext.skyboxTime >= D_8011FC1C[envCtx->unk_17][i].startTime &&
				(gSaveContext.skyboxTime < D_8011FC1C[envCtx->unk_17][i].endTime ||
				D_8011FC1C[envCtx->unk_17][i].endTime == 0xFFFF)) {
				if (D_8011FC1C[envCtx->unk_17][i].blend) {
					envCtx->skyboxBlend = Environment_LerpWeight(
						D_8011FC1C[envCtx->unk_17][i].endTime,
						D_8011FC1C[envCtx->unk_17][i].startTime,
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
		for (i = 0; i < ARRAY_COUNT(D_8011FC1C[envCtx->unk_17]); i++) {
			if (gSaveContext.skyboxTime >= D_8011FC1C[envCtx->unk_17][i].startTime &&
				(gSaveContext.skyboxTime < D_8011FC1C[envCtx->unk_17][i].endTime ||
				D_8011FC1C[envCtx->unk_17][i].endTime == 0xFFFF)) {
				newSkybox1Index = D_8011FC1C[envCtx->unk_17][i].skybox1Index;
				newSkybox2Index = D_8011FC1C[envCtx->unk_17][i].skybox2Index;
				gSkyboxBlendingEnabled = D_8011FC1C[envCtx->unk_17][i].blend;
				
				if (gSkyboxBlendingEnabled) {
					skyboxBlend = Environment_LerpWeight(
						D_8011FC1C[envCtx->unk_17][i].endTime,
						D_8011FC1C[envCtx->unk_17][i].startTime,
						((void)0, gSaveContext.skyboxTime)
						) *
						255;
				} else {
					skyboxBlend = Environment_LerpWeight(
						D_8011FC1C[envCtx->unk_17][i].endTime,
						D_8011FC1C[envCtx->unk_17][i].startTime,
						((void)0, gSaveContext.skyboxTime)
						) *
						255;
					
					skyboxBlend = (skyboxBlend < 0x80) ? 0xFF : 0;
					
					if ((envCtx->unk_19 != 0) && (envCtx->unk_19 < 3)) {
						envCtx->unk_19++;
						skyboxBlend = 0;
					}
				}
				break;
			}
		}
		
		func_8006FB94(envCtx, skyboxBlend);
		
		if (envCtx->unk_19 >= 3) {
			newSkybox1Index = D_8011FC1C[envCtx->unk_17][i].skybox1Index;
			newSkybox2Index = D_8011FC1C[envCtx->unk_18][i].skybox2Index;
			
			skyboxBlend = ((f32)envCtx->unk_24 - envCtx->unk_1A--) / (f32)envCtx->unk_24 * 255;
			
			if (envCtx->unk_1A <= 0) {
				envCtx->unk_19 = 0;
				envCtx->unk_17 = envCtx->unk_18;
			}
		}
		
		if ((envCtx->skybox1Index != newSkybox1Index) && (envCtx->skyboxDmaState == SKYBOX_DMA_INACTIVE)) {
			envCtx->skyboxDmaState = SKYBOX_DMA_FILE1_START;
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
			envCtx->skyboxDmaState = SKYBOX_DMA_FILE2_START;
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
		
		if (envCtx->skyboxDmaState == SKYBOX_DMA_FILE1_DONE) {
			envCtx->skyboxDmaState = SKYBOX_DMA_PAL1_START;
			
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
		
		if (envCtx->skyboxDmaState == SKYBOX_DMA_FILE2_DONE) {
			envCtx->skyboxDmaState = SKYBOX_DMA_PAL2_START;
			
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
		
		if ((envCtx->skyboxDmaState == SKYBOX_DMA_FILE1_START) || (envCtx->skyboxDmaState == SKYBOX_DMA_FILE2_START)) {
			if (osRecvMesg(&envCtx->loadQueue, 0, OS_MESG_NOBLOCK) == 0) {
				envCtx->skyboxDmaState++;
			}
		} else if (envCtx->skyboxDmaState >= SKYBOX_DMA_FILE1_DONE) {
			if (osRecvMesg(&envCtx->loadQueue, 0, OS_MESG_NOBLOCK) == 0) {
				envCtx->skyboxDmaState = SKYBOX_DMA_INACTIVE;
			}
		}
		
		envCtx->skyboxBlend = skyboxBlend;
	}
}