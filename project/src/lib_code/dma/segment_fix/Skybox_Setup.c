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
	
	typedef struct {
		/* 0x00 */ u16 startTime;
		/* 0x02 */ u16 endTime;
		/* 0x04 */ u8  blend; // if true, blend between.. skyboxes? palettes?
		/* 0x05 */ u8  skybox1Index; // whats the difference between _pal and non _pal files?
		/* 0x06 */ u8  skybox2Index;
	} struct_8011FC1C; // size = 0x8
	
	#define skyboxTime          environmentTime
	#define envCtx_skybox1Index envCtx.unk_10[0]
	#define envCtx_skybox2Index envCtx.unk_10[1]
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
	
	#define SkyboxContext Skybox__Context
#endif

typedef struct {
	DmaEntry* file;
	DmaEntry* palette;
} NewSkyboxFiles;

s8 gSkyboxDmaIdTable[];

f32 Environment_LerpWeight(u16 max, u16 min, u16 val);
extern NewSkyboxFiles gSkyboxFiles[];
extern struct_8011FC1C D_8011FC1C[][9];
extern u8 gWeatherMode;
asm ("gSkyboxFiles = 0x8011FD3C");
asm ("D_8011FC1C = 0x8011FC1C");
asm ("gWeatherMode = 0x8011FB30");
asm ("Environment_LerpWeight = 0x8006F93C");

void Skybox_Setup(GlobalContext* globalCtx, SkyboxContext* skyboxCtx, s16 skyboxId) {
	u32 size;
	s16 i;
	u8 sp41 = 0; // imageIdx
	u8 sp40 = 0; // imageIdx2
	u32 start;
	s32 phi_v1;
	
	if (gSkyboxDmaIdTable[skyboxId] > -1) {
		DmaEntry* entry = &gDmaDataTable[gSkyboxDmaIdTable[skyboxId] + 941];
		
		start = entry->vromStart;
		size = entry->vromEnd - entry->vromStart;
		skyboxCtx->staticSegments[0] = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
		DmaMgr_SendRequest1(skyboxCtx->staticSegments[0], start, size, "Skybox_Setup", __LINE__);
		entry++;
		start = entry->vromStart;
		size = entry->vromEnd - entry->vromStart;
		skyboxCtx->palettes = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
		DmaMgr_SendRequest1(skyboxCtx->palettes, start, size, "Skybox_Setup", __LINE__);
	}
	
	switch (skyboxId) {
		case SKYBOX_NORMAL_SKY:
			phi_v1 = 0;
			if (gSaveContext.unk_13C3 != 0 && gSaveContext.sceneSetupIndex < 4 && gWeatherMode > 0 &&
				gWeatherMode < 6) {
				phi_v1 = 1;
			}
			
			for (i = 0; i < 9; i++) {
				if (gSaveContext.skyboxTime >= D_8011FC1C[phi_v1][i].startTime &&
					(gSaveContext.skyboxTime < D_8011FC1C[phi_v1][i].endTime ||
					D_8011FC1C[phi_v1][i].endTime == 0xFFFF)) {
					globalCtx->envCtx_skybox1Index = sp41 = D_8011FC1C[phi_v1][i].skybox1Index;
					globalCtx->envCtx_skybox2Index = sp40 = D_8011FC1C[phi_v1][i].skybox2Index;
					if (D_8011FC1C[phi_v1][i].blend != 0) {
						globalCtx->envCtx.skyboxBlend =
							Environment_LerpWeight(
							D_8011FC1C[phi_v1][i].endTime,
							D_8011FC1C[phi_v1][i].startTime,
							((void)0, gSaveContext.skyboxTime)
							) *
							255.0f;
					} else {
						globalCtx->envCtx.skyboxBlend = 0;
					}
					break;
				}
			}
			
			size = gSkyboxFiles[sp41].file->vromEnd - gSkyboxFiles[sp41].file->vromStart;
			skyboxCtx->staticSegments[0] = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
			
			DmaMgr_SendRequest1(
				skyboxCtx->staticSegments[0],
				gSkyboxFiles[sp41].file->vromStart,
				size,
				"Skybox_Setup",
				__LINE__
			);
			
			size = gSkyboxFiles[sp40].file->vromEnd - gSkyboxFiles[sp40].file->vromStart;
			skyboxCtx->staticSegments[1] = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
			
			DmaMgr_SendRequest1(
				skyboxCtx->staticSegments[1],
				gSkyboxFiles[sp40].file->vromStart,
				size,
				"Skybox_Setup",
				__LINE__
			);
			
			if ((sp41 & 1) ^ ((sp41 & 4) >> 2)) {
				size = gSkyboxFiles[sp41].palette->vromEnd - gSkyboxFiles[sp41].palette->vromStart;
				
				skyboxCtx->palettes = GameState_Alloc(&globalCtx->state, size * 2, "Skybox_Setup", __LINE__);
				
				DmaMgr_SendRequest1(
					skyboxCtx->palettes,
					gSkyboxFiles[sp41].palette->vromStart,
					size,
					"Skybox_Setup",
					__LINE__
				);
				DmaMgr_SendRequest1(
					(void*)((u32)skyboxCtx->palettes + size),
					gSkyboxFiles[sp40].palette->vromStart,
					size,
					"Skybox_Setup",
					__LINE__
				);
			} else {
				size = gSkyboxFiles[sp41].palette->vromEnd - gSkyboxFiles[sp41].palette->vromStart;
				
				skyboxCtx->palettes = GameState_Alloc(&globalCtx->state, size * 2, "Skybox_Setup", __LINE__);
				
				DmaMgr_SendRequest1(
					skyboxCtx->palettes,
					gSkyboxFiles[sp40].palette->vromStart,
					size,
					"Skybox_Setup",
					__LINE__
				);
				DmaMgr_SendRequest1(
					(void*)((u32)skyboxCtx->palettes + size),
					gSkyboxFiles[sp41].palette->vromStart,
					size,
					"Skybox_Setup",
					__LINE__
				);
			}
			break;
			
		case SKYBOX_OVERCAST_SUNSET:
			start = gDmaDataTable[953].vromStart;
			size = gDmaDataTable[953].vromEnd - start;
			
			skyboxCtx->staticSegments[0] = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
			DmaMgr_SendRequest1(skyboxCtx->staticSegments[0], start, size, "Skybox_Setup", 1159);
			skyboxCtx->staticSegments[1] = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
			DmaMgr_SendRequest1(skyboxCtx->staticSegments[1], start, size, "Skybox_Setup", 1166);
			
			start = gDmaDataTable[953 + 1].vromStart;
			size = gDmaDataTable[953 + 1].vromEnd - start;
			skyboxCtx->palettes = GameState_Alloc(&globalCtx->state, size * 2, "Skybox_Setup", __LINE__);
			
			DmaMgr_SendRequest1(skyboxCtx->palettes, start, size, "Skybox_Setup", __LINE__);
			DmaMgr_SendRequest1((void*)((u32)skyboxCtx->palettes + size), start, size, "Skybox_Setup", __LINE__);
			break;
			
		case SKYBOX_CUTSCENE_MAP:
			start = gDmaDataTable[957].vromStart;
			size = gDmaDataTable[957].vromEnd - start;
			skyboxCtx->staticSegments[0] = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
			
			DmaMgr_SendRequest1(skyboxCtx->staticSegments[0], start, size, "Skybox_Setup", __LINE__);
			
			start = gDmaDataTable[957 + 2].vromStart;
			size = gDmaDataTable[957 + 2].vromEnd - start;
			skyboxCtx->staticSegments[1] = GameState_Alloc(&globalCtx->state, size, "Skybox_Setup", __LINE__);
			
			DmaMgr_SendRequest1(skyboxCtx->staticSegments[1], start, size, "Skybox_Setup", __LINE__);
			
			start = gDmaDataTable[958].vromStart;
			size = gDmaDataTable[958].vromEnd - start;
			skyboxCtx->palettes = GameState_Alloc(&globalCtx->state, size * 2, "Skybox_Setup", __LINE__);
			
			DmaMgr_SendRequest1(skyboxCtx->palettes, start, size, "Skybox_Setup", __LINE__);
			DmaMgr_SendRequest1(
				(void*)((u32)skyboxCtx->palettes + size),
				gDmaDataTable[958 + 2].vromStart,
				size,
				"Skybox_Setup",
				__LINE__
			);
			break;
			
		case SKYBOX_MARKET_ADULT:
		case SKYBOX_HOUSE_LINK:
		case SKYBOX_MARKET_CHILD_DAY:
		case SKYBOX_MARKET_CHILD_NIGHT:
		case SKYBOX_HOUSE_KNOW_IT_ALL_BROTHERS:
		case SKYBOX_STABLES:
		case SKYBOX_HOUSE_KAKARIKO:
		case SKYBOX_HOUSE_RICHARD:
		case SKYBOX_HOUSE_IMPA:
			skyboxCtx->unk_140 = 1;
			break;
			
		case SKYBOX_BAZAAR:
		case SKYBOX_HAPPY_MASK_SHOP:
		case SKYBOX_KOKIRI_SHOP:
		case SKYBOX_GORON_SHOP:
		case SKYBOX_ZORA_SHOP:
		case SKYBOX_POTION_SHOP_KAKARIKO:
		case SKYBOX_POTION_SHOP_MARKET:
		case SKYBOX_BOMBCHU_SHOP:
			skyboxCtx->unk_140 = 1;
			skyboxCtx->rot.y = 0.8f;
			break;
			
		case SKYBOX_TENT:
		case SKYBOX_HOUSE_OF_TWINS:
		case SKYBOX_HOUSE_MIDO:
		case SKYBOX_HOUSE_SARIA:
		case SKYBOX_HOUSE_ALLEY:
			skyboxCtx->unk_140 = 2;
			break;
	}
}

#define ID(x) ((x) - 941)

s8 gSkyboxDmaIdTable[] = {
	/* SKYBOX_NONE                       */ -1,
	/* SKYBOX_NORMAL_SKY                 */ -1,
	/* SKYBOX_BAZAAR                     */ ID(977),
	/* SKYBOX_OVERCAST_SUNSET            */ -1,
	/* SKYBOX_MARKET_ADULT               */ ID(965),
	/* SKYBOX_CUTSCENE_MAP               */ -1,
	-1,
	/* SKYBOX_HOUSE_LINK                 */ ID(967),
	-1,
	/* SKYBOX_MARKET_CHILD_DAY           */ ID(961),
	/* SKYBOX_MARKET_CHILD_NIGHT         */ ID(963),
	/* SKYBOX_HAPPY_MASK_SHOP            */ ID(1003),
	/* SKYBOX_HOUSE_KNOW_IT_ALL_BROTHERS */ ID(969),
	-1,
	/* SKYBOX_HOUSE_OF_TWINS             */ ID(971),
	/* SKYBOX_STABLES                    */ ID(979),
	/* SKYBOX_HOUSE_KAKARIKO             */ ID(981),
	/* SKYBOX_KOKIRI_SHOP                */ ID(987),
	-1,
	/* SKYBOX_GORON_SHOP                 */ ID(989),
	/* SKYBOX_ZORA_SHOP                  */ ID(991),
	-1,
	/* SKYBOX_POTION_SHOP_KAKARIKO       */ ID(993),
	/* SKYBOX_POTION_SHOP_MARKET         */ ID(995),
	/* SKYBOX_BOMBCHU_SHOP               */ ID(997),
	-1,
	/* SKYBOX_HOUSE_RICHARD              */ ID(985),
	/* SKYBOX_HOUSE_IMPA                 */ ID(999),
	/* SKYBOX_TENT                       */ ID(1001),
	/* SKYBOX_UNSET_1D                   */ -1,
	-1,
	-1,
	/* SKYBOX_HOUSE_MIDO                 */ ID(973),
	/* SKYBOX_HOUSE_SARIA                */ ID(975),
	/* SKYBOX_HOUSE_ALLEY                */ ID(983),
	-1,
	-1,
	-1,
	-1,
	/* SKYBOX_UNSET_27                   */ -1,
};