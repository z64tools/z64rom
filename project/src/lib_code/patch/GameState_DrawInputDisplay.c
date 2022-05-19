#include <ULib.h>

/*
   z64ram = 0x800C4558
   z64rom = 0xB3B6F8
 */

void GameState_DrawInputDisplay(u16 input, Gfx** gfx) {
	// static const u16 sInpDispBtnColors[] = {
	// 	GPACK_RGBA5551(255, 255, 0, 1),   GPACK_RGBA5551(255, 255, 0, 1),   GPACK_RGBA5551(255, 255, 0, 1),
	// 	GPACK_RGBA5551(255, 255, 0, 1),   GPACK_RGBA5551(120, 120, 120, 1), GPACK_RGBA5551(120, 120, 120, 1),
	// 	GPACK_RGBA5551(0, 255, 255, 1),   GPACK_RGBA5551(255, 0, 255, 1),   GPACK_RGBA5551(120, 120, 120, 1),
	// 	GPACK_RGBA5551(120, 120, 120, 1), GPACK_RGBA5551(120, 120, 120, 1), GPACK_RGBA5551(120, 120, 120, 1),
	// 	GPACK_RGBA5551(255, 0, 0, 1),     GPACK_RGBA5551(120, 120, 120, 1), GPACK_RGBA5551(0, 255, 0, 1),
	// 	GPACK_RGBA5551(0, 0, 255, 1),
	// };
	// s32 i, j, k;
	// Gfx* gfxP = *gfx;
	//
	// gDPPipeSync(gfxP++);
	// gDPSetOtherMode(
	// 	gfxP++,
	// 	G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_CONV | G_TF_POINT | G_TT_NONE | G_TL_TILE |
	// 	G_TD_CLAMP | G_TP_NONE | G_CYC_FILL | G_PM_NPRIMITIVE,
	// 	G_AC_NONE | G_ZS_PIXEL | G_RM_NOOP | G_RM_NOOP2
	// );
	//
	// for (i = 0; i < 16; i++) {
	// 	j = i;
	// 	if (input & (1 << i)) {
	// 		gDPSetFillColor(gfxP++, (sInpDispBtnColors[i] << 0x10) | sInpDispBtnColors[i]);
	// 		k = i + 1;
	// 		gDPFillRectangle(gfxP++, (j * 4) + 226, 220, (k * 4) + 225, 223);
	// 		gDPPipeSync(gfxP++);
	// 	}
	// }
	//
	// *gfx = gfxP;
}