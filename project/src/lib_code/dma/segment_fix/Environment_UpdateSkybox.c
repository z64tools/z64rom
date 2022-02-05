#include <oot_mq_debug/z64hdr.h>

#ifndef DO_ACTION_TEX_WIDTH
	typedef enum {
		/* 0x00 */ SKYBOX_NONE,
		/* 0x01 */ SKYBOX_NORMAL_SKY,
		/* 0x02 */ SKYBOX_BAZAAR,
		/* 0x03 */ SKYBOX_OVERCAST_SUNSET,
		/* 0x04 */ SKYBOX_MARKET_ADULT,
		/* 0x05 */ SKYBOX_CUTSCENE_MAP,
		/* 0x07 */ SKYBOX_HOUSE_LINK           = 7,
		/* 0x09 */ SKYBOX_MARKET_CHILD_DAY     = 9,
		/* 0x0A */ SKYBOX_MARKET_CHILD_NIGHT,
		/* 0x0B */ SKYBOX_HAPPY_MASK_SHOP,
		/* 0x0C */ SKYBOX_HOUSE_KNOW_IT_ALL_BROTHERS,
		/* 0x0E */ SKYBOX_HOUSE_OF_TWINS       = 14,
		/* 0x0F */ SKYBOX_STABLES,
		/* 0x10 */ SKYBOX_HOUSE_KAKARIKO,
		/* 0x11 */ SKYBOX_KOKIRI_SHOP,
		/* 0x13 */ SKYBOX_GORON_SHOP           = 19,
		/* 0x14 */ SKYBOX_ZORA_SHOP,
		/* 0x16 */ SKYBOX_POTION_SHOP_KAKARIKO = 22,
		/* 0x17 */ SKYBOX_POTION_SHOP_MARKET,
		/* 0x18 */ SKYBOX_BOMBCHU_SHOP,
		/* 0x1A */ SKYBOX_HOUSE_RICHARD        = 26,
		/* 0x1B */ SKYBOX_HOUSE_IMPA,
		/* 0x1C */ SKYBOX_TENT,
		/* 0x1D */ SKYBOX_UNSET_1D,
		/* 0x20 */ SKYBOX_HOUSE_MIDO           = 32,
		/* 0x21 */ SKYBOX_HOUSE_SARIA,
		/* 0x22 */ SKYBOX_HOUSE_ALLEY,
		/* 0x27 */ SKYBOX_UNSET_27             = 39
	} SkyboxId;
	
	typedef enum {
		/*  0 */ SKYBOX_DMA_INACTIVE,
		/*  1 */ SKYBOX_DMA_FILE1_START,
		/*  2 */ SKYBOX_DMA_FILE1_DONE,
		/*  3 */ SKYBOX_DMA_PAL1_START,
		/* 11 */ SKYBOX_DMA_FILE2_START = 11,
		/* 12 */ SKYBOX_DMA_FILE2_DONE,
		/* 13 */ SKYBOX_DMA_PAL2_START
	} SkyboxDmaState;
	
	typedef struct {
		/* 0x00 */ u16 startTime;
		/* 0x02 */ u16 endTime;
		/* 0x04 */ u8  blend; // if true, blend between.. skyboxes? palettes?
		/* 0x05 */ u8  skybox1Index; // whats the difference between _pal and non _pal files?
		/* 0x06 */ u8  skybox2Index;
	} struct_8011FC1C; // size = 0x8
	
	#define skyboxTime          environmentTime
	#define envCtx_skybox1Index envCtx->unk_10[0]
	#define envCtx_skybox2Index envCtx->unk_10[1]
	#define skyboxBlend         unk_13
	
	typedef struct {
		/* 0x000 */ char  unk_00[0x128];
		/* 0x128 */ void* staticSegments[2];
		/* 0x130 */ u16 (*palettes)[256];
		/* 0x134 */ Gfx (*dListBuf)[150];
		/* 0x138 */ Gfx*  unk_138;
		/* 0x13C */ Vtx*  roomVtx;
		/* 0x140 */ s16   unk_140;
		/* 0x144 */ Vec3f rot;
		/* 0x150 */ char  unk_150[0x10];
	} Skybox__Context; // size = 0x160
	
	#define SkyboxContext  Skybox__Context
	#define unk_17         gloomySky
	#define skyboxDisabled skyDisabled
#endif

typedef struct {
	DmaEntry* file;
	DmaEntry* palette;
} NewSkyboxFiles;

f32 Environment_LerpWeight(u16 max, u16 min, u16 val);
void func_8006FB94(EnvironmentContext* envCtx, u8 unused);
extern NewSkyboxFiles gSkyboxFiles[];
extern struct_8011FC1C D_8011FC1C[][9];
extern u8 gWeatherMode;
extern u8 gSkyboxBlendingEnabled;
asm ("gSkyboxFiles = 0x8011FD3C");
asm ("gSkyboxBlendingEnabled = 0x8011FB3C");
asm ("func_8006FB94 = 0x8006FB94");
asm ("D_8011FC1C = 0x8011FC1C");
asm ("gWeatherMode = 0x8011FB30");
asm ("Environment_LerpWeight = 0x8006F93C");

#define envCtx_loadQueue      (void*)((u8*)envCtx + 0x68)
#define envCtx_loadMsg        (void*)((u8*)envCtx + 0x80)
#define envCtx_skyboxDmaState (*((s8*)envCtx + 0x44))
#define envCtx_dmaRequest     (void*)((u8*)envCtx + 0x48)

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
		
		if ((envCtx_skybox1Index != newSkybox1Index) && (envCtx_skyboxDmaState == SKYBOX_DMA_INACTIVE)) {
			envCtx_skyboxDmaState = SKYBOX_DMA_FILE1_START;
			size = gSkyboxFiles[newSkybox1Index].file->vromEnd - gSkyboxFiles[newSkybox1Index].file->vromStart;
			
			osCreateMesgQueue(envCtx_loadQueue, envCtx_loadMsg, 1);
			DmaMgr_SendRequest2(
				envCtx_dmaRequest,
				(u32)skyboxCtx->staticSegments[0],
				gSkyboxFiles[newSkybox1Index].file->vromStart,
				size,
				0,
				envCtx_loadQueue,
				NULL,
				NULL,
				__LINE__
			);
			envCtx_skybox1Index = newSkybox1Index;
		}
		
		if ((envCtx_skybox2Index != newSkybox2Index) && (envCtx_skyboxDmaState == SKYBOX_DMA_INACTIVE)) {
			envCtx_skyboxDmaState = SKYBOX_DMA_FILE2_START;
			size = gSkyboxFiles[newSkybox2Index].file->vromEnd - gSkyboxFiles[newSkybox2Index].file->vromStart;
			
			osCreateMesgQueue(envCtx_loadQueue, envCtx_loadMsg, 1);
			DmaMgr_SendRequest2(
				envCtx_dmaRequest,
				(u32)skyboxCtx->staticSegments[1],
				gSkyboxFiles[newSkybox2Index].file->vromStart,
				size,
				0,
				envCtx_loadQueue,
				NULL,
				NULL,
				__LINE__
			);
			envCtx_skybox2Index = newSkybox2Index;
		}
		
		if (envCtx_skyboxDmaState == SKYBOX_DMA_FILE1_DONE) {
			envCtx_skyboxDmaState = SKYBOX_DMA_PAL1_START;
			
			if ((newSkybox1Index & 1) ^ ((newSkybox1Index & 4) >> 2)) {
				size = gSkyboxFiles[newSkybox1Index].palette->vromEnd - gSkyboxFiles[newSkybox1Index].palette->vromStart;
				
				osCreateMesgQueue(envCtx_loadQueue, envCtx_loadMsg, 1);
				DmaMgr_SendRequest2(
					envCtx_dmaRequest,
					(u32)skyboxCtx->palettes,
					gSkyboxFiles[newSkybox1Index].palette->vromStart,
					size,
					0,
					envCtx_loadQueue,
					NULL,
					NULL,
					__LINE__
				);
			} else {
				size = gSkyboxFiles[newSkybox1Index].palette->vromEnd - gSkyboxFiles[newSkybox1Index].palette->vromStart;
				osCreateMesgQueue(envCtx_loadQueue, envCtx_loadMsg, 1);
				DmaMgr_SendRequest2(
					envCtx_dmaRequest,
					(u32)skyboxCtx->palettes + size,
					gSkyboxFiles[newSkybox1Index].palette->vromStart,
					size,
					0,
					envCtx_loadQueue,
					NULL,
					NULL,
					__LINE__
				);
			}
		}
		
		if (envCtx_skyboxDmaState == SKYBOX_DMA_FILE2_DONE) {
			envCtx_skyboxDmaState = SKYBOX_DMA_PAL2_START;
			
			if ((newSkybox2Index & 1) ^ ((newSkybox2Index & 4) >> 2)) {
				size = gSkyboxFiles[newSkybox2Index].palette->vromEnd - gSkyboxFiles[newSkybox2Index].palette->vromStart;
				
				osCreateMesgQueue(envCtx_loadQueue, envCtx_loadMsg, 1);
				DmaMgr_SendRequest2(
					envCtx_dmaRequest,
					(u32)skyboxCtx->palettes,
					gSkyboxFiles[newSkybox2Index].palette->vromStart,
					size,
					0,
					envCtx_loadQueue,
					NULL,
					NULL,
					__LINE__
				);
			} else {
				size = gSkyboxFiles[newSkybox2Index].palette->vromEnd - gSkyboxFiles[newSkybox2Index].palette->vromStart;
				osCreateMesgQueue(envCtx_loadQueue, envCtx_loadMsg, 1);
				DmaMgr_SendRequest2(
					envCtx_dmaRequest,
					(u32)skyboxCtx->palettes + size,
					gSkyboxFiles[newSkybox2Index].palette->vromStart,
					size,
					0,
					envCtx_loadQueue,
					NULL,
					NULL,
					__LINE__
				);
			}
		}
		
		if ((envCtx_skyboxDmaState == SKYBOX_DMA_FILE1_START) || (envCtx_skyboxDmaState == SKYBOX_DMA_FILE2_START)) {
			if (osRecvMesg(envCtx_loadQueue, 0, OS_MESG_NOBLOCK) == 0) {
				envCtx_skyboxDmaState++;
			}
		} else if (envCtx_skyboxDmaState >= SKYBOX_DMA_FILE1_DONE) {
			if (osRecvMesg(envCtx_loadQueue, 0, OS_MESG_NOBLOCK) == 0) {
				envCtx_skyboxDmaState = SKYBOX_DMA_INACTIVE;
			}
		}
		
		envCtx->skyboxBlend = skyboxBlend;
	}
}