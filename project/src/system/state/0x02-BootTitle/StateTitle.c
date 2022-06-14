/*
 * File: z_title.c
 * Overlay: ovl_title
 * Description: Displays the Nintendo Logo
 */

#include <uLib.h>

extern u64 nintendo_rogo_static_Tex_000000[];
extern u64 nintendo_rogo_static_Tex_001800[];
extern Vtx nintendo_rogo_staticVtx_001C00[];
extern Gfx gNintendo64LogoDL[];
extern u64 nintendo_rogo_staticTex_0029C0[];

void View_LookAt(View* view, Vec3f* eye, Vec3f* at, Vec3f* up);
asm ("View_LookAt = 0x800AA358");
asm("Opening_Init = 0x80803CAC");

// Note: In other rom versions this function also updates unk_1D4, coverAlpha, addAlpha, visibleDuration to calculate
// the fade-in/fade-out + the duration of the n64 logo animation
void BootTitle_Calc(TitleContext* this) {
	if (CHK_ANY(press, BTN_A | BTN_B)) {
		this->visibleDuration = 0;
		this->addAlpha = 3;
	}
	
	if ((this->coverAlpha == 0) && (this->visibleDuration != 0)) {
		this->visibleDuration--;
		this->unk_1D4--;
		if (this->unk_1D4 == 0) {
			this->unk_1D4 = 400;
		}
	} else {
		this->coverAlpha += this->addAlpha;
		if (this->coverAlpha <= 0) {
			this->coverAlpha = 0;
			this->addAlpha = 3;
		} else if (this->coverAlpha >= 0xFF) {
			this->coverAlpha = 0xFF;
			this->exit = 1;
		}
	}
	this->uls = this->ult & 0x7F;
	this->ult++;
}

void BootTitle_SetupView(TitleContext* this, f32 x, f32 y, f32 z) {
	View* view = &this->view;
	Vec3f eye;
	Vec3f lookAt;
	Vec3f up;
	
	eye.x = x;
	eye.y = y;
	eye.z = z;
	up.x = up.z = 0.0f;
	lookAt.x = lookAt.y = lookAt.z = 0.0f;
	up.y = 1.0f;
	
	View_SetPerspective(view, 30.0f, 10.0f, 12800.0f);
	View_LookAt(view, &eye, &lookAt, &up);
	View_Apply(view, VIEW_ALL);
}

void BootTitle_Draw(TitleContext* this) {
	static s16 sTitleRotY = 0;
	static Lights1 sTitleLights = gdSPDefLights1(100, 100, 100, 255, 255, 255, 69, 69, 69);
	
	u16 y;
	u16 idx;
	s32 pad1;
	Vec3f v3;
	Vec3f v1;
	Vec3f v2;
	s32 pad2[2];
	
	OPEN_DISPS(this->state.gfxCtx, "../z_title.c", 395);
	
	v3.x = 69;
	v3.y = 69;
	v3.z = 69;
	v2.x = -4949.148;
	v2.y = 4002.5417;
	v1.x = 0;
	v1.y = 0;
	v1.z = 0;
	v2.z = 1119.0837;
	
	func_8002EABC(&v1, &v2, &v3, this->state.gfxCtx);
	gSPSetLights1(POLY_OPA_DISP++, sTitleLights);
	BootTitle_SetupView(this, 0, 150.0, 300.0);
	Gfx_SetupDL_25Opa(this->state.gfxCtx);
	Matrix_Translate(0.0, -15.0, 0, MTXMODE_NEW);
	Matrix_Scale(2.0, 2.0, 2.0, MTXMODE_APPLY);
	Matrix_RotateZYX(0, sTitleRotY, 0, MTXMODE_APPLY);
	
	gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(this->state.gfxCtx, "../z_title.c", 424), G_MTX_LOAD);
	gSPDisplayList(POLY_OPA_DISP++, gNintendo64LogoDL);
	
#if 0
	Gfx_SetupDL_39Opa(this->state.gfxCtx);
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetCycleType(POLY_OPA_DISP++, G_CYC_2CYCLE);
	gDPSetRenderMode(POLY_OPA_DISP++, G_RM_XLU_SURF2, G_RM_OPA_CI | CVG_DST_WRAP);
	gDPSetCombineLERP(
		POLY_OPA_DISP++,
		TEXEL1,
		PRIMITIVE,
		ENV_ALPHA,
		TEXEL0,
		0,
		0,
		0,
		TEXEL0,
		PRIMITIVE,
		ENVIRONMENT,
		COMBINED,
		ENVIRONMENT,
		COMBINED,
		0,
		PRIMITIVE,
		0
	);
	gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 170, 255, 255, 255);
	gDPSetEnvColor(POLY_OPA_DISP++, 0x79, 0, 0xF5, 128);
	
	gDPLoadMultiBlock(
		POLY_OPA_DISP++,
		nintendo_rogo_static_Tex_001800,
		0x100,
		1,
		G_IM_FMT_I,
		G_IM_SIZ_8b,
		32,
		32,
		0,
		G_TX_NOMIRROR | G_TX_WRAP,
		G_TX_NOMIRROR | G_TX_WRAP,
		5,
		5,
		2,
		11
	);
	
	for (idx = 0, y = 94; idx < 16; idx++, y += 2) {
		gDPLoadTextureBlock(
			POLY_OPA_DISP++,
			&((u8*)nintendo_rogo_static_Tex_000000)[0x180 * idx],
			G_IM_FMT_I,
			G_IM_SIZ_8b,
			192,
			2,
			0,
			G_TX_NOMIRROR | G_TX_WRAP,
			G_TX_NOMIRROR | G_TX_WRAP,
			G_TX_NOMASK,
			G_TX_NOMASK,
			G_TX_NOLOD,
			G_TX_NOLOD
		);
		
		gDPSetTileSize(POLY_OPA_DISP++, 1, this->uls, (this->ult & 0x7F) - idx * 4, 0, 0);
		gSPTextureRectangle(
			POLY_OPA_DISP++,
			64 << 2,
				(y) << 2,
				289 << 2,
				(y + 2) << 2,
				G_TX_RENDERTILE,
				0,
				0,
				1 << 10,
				1 << 10
		);
	}
#endif
	
	Environment_FillScreen(this->state.gfxCtx, 0, 0, 0, (s16)this->coverAlpha, FILL_SCREEN_XLU);
	
	sTitleRotY += 300;
	
	CLOSE_DISPS(this->state.gfxCtx, "../z_title.c", 483);
}

void BootTitle_Main(GameState* thisx) {
	TitleContext* this = (TitleContext*)thisx;
	
	OPEN_DISPS(this->state.gfxCtx, "../z_title.c", 494);
	
	gSPSegment(POLY_OPA_DISP++, 0, NULL);
	gSPSegment(POLY_OPA_DISP++, 1, this->staticSegment);
	func_80095248(this->state.gfxCtx, 0, 0, 0);
	BootTitle_Calc(this);
	BootTitle_Draw(this);
	
	if (this->exit) {
		gSaveContext.seqId = (u8)NA_BGM_DISABLED;
		gSaveContext.natureAmbienceId = 0xFF;
		gSaveContext.gameMode = 1;
		this->state.running = false;
		SET_NEXT_GAMESTATE(&this->state, Opening_Init, OpeningContext);
	}
	
	CLOSE_DISPS(this->state.gfxCtx, "../z_title.c", 541);
}

void BootTitle_StateDestroy(GameState* thisx) {
	TitleContext* this = (TitleContext*)thisx;
	
	Sram_InitSram(&this->state, &this->sramCtx);
}

void BootTitle_StateInit(GameState* thisx) {
	u32 size = gDmaDataTable[938].vromEnd - gDmaDataTable[938].vromStart;
	TitleContext* this = (TitleContext*)thisx;
	
	this->staticSegment = GameState_Alloc(&this->state, size, "../z_title.c", 611);
	DmaMgr_SendRequest1(this->staticSegment, gDmaDataTable[938].vromStart, size, "../z_title.c", 615);
	R_UPDATE_RATE = 1;
	Matrix_Init(&this->state);
	View_Init(&this->view, this->state.gfxCtx);
	this->state.main = BootTitle_Main;
	this->state.destroy = BootTitle_StateDestroy;
	this->exit = false;
	gSaveContext.fileNum = 0xFF;
	Sram_Alloc(&this->state, &this->sramCtx);
	this->ult = 0;
	this->unk_1D4 = 0x14;
	this->coverAlpha = 255;
	this->addAlpha = -3;
	this->visibleDuration = 0x3C;
}
