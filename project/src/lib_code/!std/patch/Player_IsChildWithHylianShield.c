/*
   z64ram = 0x8008E9D0
   z64rom = 0xB05B70
   z64next = 0x8008E9F8
 */

#include <uLib.h>

s32 Player_IsChildWithHylianShield(Player* this) {
#if ChildLink_HylianShieldAdjust == true
	
	return 0;
#else
	
	return Vanilla_HylianShieldCondition(this);
#endif
}