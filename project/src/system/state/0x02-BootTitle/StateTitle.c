/*
 * File: z_title.c
 * Overlay: ovl_title
 * Description: Displays the Nintendo Logo
 */

#include <asm_macros.h>
#include <uLib.h>

Asm_SymbolAlias("__z64_init", BootTitle_Init);
Asm_SymbolAlias("__z64_dest", BootTitle_Destroy);

extern u64 nintendo_rogo_static_Tex_000000[];
extern u64 nintendo_rogo_static_Tex_001800[];
extern Vtx nintendo_rogo_staticVtx_001C00[];
extern Gfx gNintendo64LogoDL[];
extern u64 nintendo_rogo_staticTex_0029C0[];

void View_LookAt(View* view, Vec3f* eye, Vec3f* at, Vec3f* up);
void* alloca(u32);
asm ("View_LookAt = 0x800AA358");
asm ("Opening_Init = 0x80803CAC");

// Note: In other rom versions this function also updates unk_1D4, coverAlpha, addAlpha, visibleDuration to calculate
// the fade-in/fade-out + the duration of the n64 logo animation
void BootTitle_Calc(TitleContext* this) {
	if (CHK_ANY(press, BTN_A | BTN_B)) {
		this->visibleDuration = 0;
		this->addAlpha = 6;
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

void BootTitle_MoreRamPlease(Gfx** gfxp) {
	Gfx* g;
	GfxPrint* printer;
	
	g = *gfxp;
	g = Gfx_SetupDL_28(g);
	printer = alloca(sizeof(GfxPrint));
	GfxPrint_Init(printer);
	GfxPrint_Open(printer, g);
	
	GfxPrint_SetColor(printer, 255, 75, 75, 255);
	GfxPrint_SetPos(printer, 12, 21);
	GfxPrint_Printf(printer, "Not enough RAM!");
	
	GfxPrint_SetColor(printer, 255, 255, 255, 255);
	GfxPrint_SetPos(printer, 8, 23);
	GfxPrint_Printf(printer, "Expansion Pak is required.", gBuildTeam);
	g = GfxPrint_Close(printer);
	GfxPrint_Destroy(printer);
	*gfxp = g;
}

void BootTitle_Draw(TitleContext* this) {
	static s16 sTitleRotY = 0;
	static Lights1 sTitleLights = gdSPDefLights1(100, 100, 100, 255, 255, 255, 69, 69, 69);
	
	Vec3f v3;
	Vec3f v1;
	Vec3f v2;
	
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
	
	if (osMemSize > 0x400000U) {
		BootTitle_Calc(this);
		BootTitle_Draw(this);
	} else {
		Gfx* gfx = POLY_OPA_DISP;
		
		BootTitle_MoreRamPlease(&gfx);
		POLY_OPA_DISP = gfx;
	}
	
	if (this->exit) {
		gSaveContext.seqId = (u8)NA_BGM_DISABLED;
		gSaveContext.natureAmbienceId = 0xFF;
		gSaveContext.gameMode = 1;
		this->state.running = false;
		SET_NEXT_GAMESTATE(&this->state, Opening_Init, OpeningContext);
	}
	
	CLOSE_DISPS(this->state.gfxCtx, "../z_title.c", 541);
}

void BootTitle_Destroy(GameState* thisx) {
	TitleContext* this = (TitleContext*)thisx;
	
	Sram_InitSram(&this->state, &this->sramCtx);
}

void BootTitle_Init(GameState* thisx) {
	TitleContext* this = (TitleContext*)thisx;
	
	if (osMemSize > 0x400000U) {
		u32 size = gDmaDataTable[938].vromEnd - gDmaDataTable[938].vromStart;
		this->staticSegment = GameState_Alloc(&this->state, size, "../z_title.c", 611);
		DmaMgr_SendRequest1(this->staticSegment, gDmaDataTable[938].vromStart, size, "../z_title.c", 615);
	}
	
	R_UPDATE_RATE = 1;
	Matrix_Init(&this->state);
	View_Init(&this->view, this->state.gfxCtx);
	this->state.main = BootTitle_Main;
	this->state.destroy = BootTitle_Destroy;
	this->exit = false;
	gSaveContext.fileNum = 0xFF;
	Sram_Alloc(&this->state, &this->sramCtx);
	this->ult = 0;
	this->unk_1D4 = 0x14;
	this->coverAlpha = 255;
	this->addAlpha = -3;
	this->visibleDuration = 0x3C;
}
