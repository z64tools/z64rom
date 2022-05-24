#include <uLib.h>
#include <vt.h>

/*
   z64ram = 0x800BFAE4
   z64rom = 0xB36C84
 */

void Gameplay_Main(GameState* thisx) {
	GlobalContext* globalCtx = (GlobalContext*)thisx;
	
	D_8012D1F8 = &globalCtx->state.input[0];
	
	DebugDisplay_Init();
	
	if ((HREG(80) == 10) && (HREG(94) != 10)) {
		HREG(81) = 1;
		HREG(82) = 1;
		HREG(83) = 1;
		HREG(84) = 3;
		HREG(85) = 1;
		HREG(86) = 1;
		HREG(87) = 1;
		HREG(88) = 1;
		HREG(89) = 1;
		HREG(90) = 15;
		HREG(91) = 1;
		HREG(92) = 1;
		HREG(93) = 1;
		HREG(94) = 10;
	}
	
	if ((HREG(80) != 10) || (HREG(81) != 0))
		Gameplay_Update(globalCtx);
	Gameplay_Draw(globalCtx);
#ifdef DEV_BUILD
	DebugSys_Update(globalCtx);
#endif
}