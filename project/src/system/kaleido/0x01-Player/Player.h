#include <uLib.h>
#include "playas_child.h"
#include "playas_adult.h"

#include "overlays/actors/ovl_Bg_Heavy_Block/z_bg_heavy_block.h"
#include "overlays/actors/ovl_Door_Shutter/z_door_shutter.h"
#include "overlays/actors/ovl_En_Boom/z_en_boom.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "overlays/actors/ovl_En_Fish/z_en_fish.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "overlays/actors/ovl_En_Insect/z_en_insect.h"
#include "overlays/effects/ovl_Effect_Ss_Fhg_Flash/z_eff_ss_fhg_flash.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "assets/objects/object_link_child/object_link_child.h"

#define func_80835C58(play, this, func, flags) ({ \
		if (gLibCtx.state.playerPrint) { \
			osLibPrintf("" PRNT_YELW "PLAYER"); \
			osLibPrintf("From: " PRNT_GRAY "[" PRNT_BLUE "%s" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]", __FUNCTION__, __LINE__); \
			osLibPrintf("Set:  "PRNT_GRAY "[" PRNT_BLUE #func PRNT_GRAY "]"); } \
		__func_80835C58(play, this, func, flags); })

typedef struct {
	/* 0x00 */ u8  itemId;
	/* 0x01 */ u8  field; // various bit-packed data
	/* 0x02 */ s8  gi;     // defines the draw id and chest opening animation
	/* 0x03 */ u8  textId;
	/* 0x04 */ u16 objectId;
} GetItemEntry; // size = 0x06

#define GET_ITEM(itemId, objectId, drawId, textId, field, chestAnim) \
	{ itemId, field, (chestAnim != CHEST_ANIM_SHORT ? 1 : -1) * (drawId + 1), textId, objectId }

#define CHEST_ANIM_SHORT 0
#define CHEST_ANIM_LONG  1

#define GET_ITEM_NONE \
	{ ITEM_NONE, 0, 0, 0, OBJECT_INVALID }

typedef enum {
	/* 0x00 */ KNOB_ANIM_ADULT_L,
	/* 0x01 */ KNOB_ANIM_CHILD_L,
	/* 0x02 */ KNOB_ANIM_ADULT_R,
	/* 0x03 */ KNOB_ANIM_CHILD_R
} KnobDoorAnim;

typedef struct {
	/* 0x00 */ u8  itemId;
	/* 0x02 */ s16 actorId;
} ExplosiveInfo; // size = 0x04

typedef struct {
	/* 0x00 */ s16 actorId;
	/* 0x02 */ u8  itemId;
	/* 0x03 */ u8  actionParam;
	/* 0x04 */ u8  textId;
} BottleCatchInfo; // size = 0x06

typedef struct {
	/* 0x00 */ s16 actorId;
	/* 0x02 */ s16 actorParams;
} BottleDropInfo; // size = 0x04

typedef struct {
	/* 0x00 */ s8  damage;
	/* 0x01 */ u8  unk_01;
	/* 0x02 */ u8  unk_02;
	/* 0x03 */ u8  unk_03;
	/* 0x04 */ u16 sfxId;
} FallImpactInfo; // size = 0x06

typedef struct {
	/* 0x00 */ Vec3f pos;
	/* 0x0C */ s16   yaw;
} SpecialRespawnInfo; // size = 0x10

typedef struct {
	/* 0x00 */ u16 sfxId;
	/* 0x02 */ s16 field;
} struct_80832924; // size = 0x04

typedef struct {
	/* 0x00 */ u16 unk_00;
	/* 0x02 */ s16 unk_02;
} struct_808551A4; // size = 0x04

typedef struct {
	/* 0x00 */ LinkAnimationHeader* anim;
	/* 0x04 */ u8 unk_04;
} struct_808540F4; // size = 0x08

typedef struct {
	/* 0x00 */ LinkAnimationHeader* unk_00;
	/* 0x04 */ LinkAnimationHeader* unk_04;
	/* 0x08 */ u8 unk_08;
	/* 0x09 */ u8 unk_09;
} struct_80854554; // size = 0x0C

typedef struct {
	/* 0x00 */ LinkAnimationHeader* unk_00;
	/* 0x04 */ LinkAnimationHeader* unk_04;
	/* 0x08 */ LinkAnimationHeader* unk_08;
	/* 0x0C */ u8 unk_0C;
	/* 0x0D */ u8 unk_0D;
} struct_80854190; // size = 0x10

typedef struct {
	/* 0x00 */ LinkAnimationHeader* anim;
	/* 0x04 */ f32 unk_04;
	/* 0x04 */ f32 unk_08;
} struct_80854578; // size = 0x0C

typedef struct {
	/* 0x00 */ s8 type;
	/* 0x04 */ union {
		void* ptr;
		void (* func)(PlayState*, Player*, CsCmdActorAction*);
	};
} struct_80854B18; // size = 0x08

typedef struct {
	/* 0x00 */ s16 unk_00;
	/* 0x02 */ s16 unk_02;
	/* 0x04 */ s16 unk_04;
	/* 0x06 */ s16 unk_06;
	/* 0x08 */ s16 unk_08;
} struct_80858AC8; // size = 0x0A

void func_80833770(PlayState* play, Player* this);
void func_80833790(PlayState* play, Player* this);
void func_8083379C(PlayState* play, Player* this);
void func_8083377C(PlayState* play, Player* this);
void func_808337D4(PlayState* play, Player* this);
void func_80833910(PlayState* play, Player* this);
void func_80833984(PlayState* play, Player* this);
void func_8083399C(PlayState* play, Player* this, s8 actionParam);
s32 func_8083485C(Player* this, PlayState* play);
s32 func_808349DC(Player* this, PlayState* play);
s32 func_80834A2C(Player* this, PlayState* play);
s32 func_80834B5C(Player* this, PlayState* play);
s32 func_80834C74(Player* this, PlayState* play);
s32 func_8083501C(Player* this, PlayState* play);
s32 func_808351D4(Player* this, PlayState* play);
s32 func_808353D8(Player* this, PlayState* play);
s32 func_80835588(Player* this, PlayState* play);
s32 func_808356E8(Player* this, PlayState* play);
s32 func_80835800(Player* this, PlayState* play);
s32 func_80835884(Player* this, PlayState* play);
s32 func_808358F0(Player* this, PlayState* play);
s32 func_808359FC(Player* this, PlayState* play);
s32 func_80835B60(Player* this, PlayState* play);
s32 func_80835C08(Player* this, PlayState* play);
void func_80835F44(PlayState* play, Player* this, s32 item);
void func_80839F90(Player* this, PlayState* play);
s32 func_80838A14(Player* this, PlayState* play);
s32 func_80839800(Player* this, PlayState* play);
s32 func_8083B040(Player* this, PlayState* play);
s32 func_8083B998(Player* this, PlayState* play);
s32 func_8083B644(Player* this, PlayState* play);
s32 func_8083BDBC(Player* this, PlayState* play);
s32 func_8083C1DC(Player* this, PlayState* play);
s32 func_8083C2B0(Player* this, PlayState* play);
s32 func_8083C544(Player* this, PlayState* play);
s32 func_8083C61C(PlayState* play, Player* this);
void func_8083CA20(PlayState* play, Player* this);
void func_8083CA54(PlayState* play, Player* this);
void func_8083CA9C(PlayState* play, Player* this);
s32 func_8083E0FC(Player* this, PlayState* play);
s32 func_8083E5A8(Player* this, PlayState* play);
s32 func_8083EB44(Player* this, PlayState* play);
s32 func_8083F7BC(Player* this, PlayState* play);
void func_80840450(Player* this, PlayState* play);
void func_808407CC(Player* this, PlayState* play);
void func_80840BC8(Player* this, PlayState* play);
void func_80840DE4(Player* this, PlayState* play);
void func_808414F8(Player* this, PlayState* play);
void func_8084170C(Player* this, PlayState* play);
void func_808417FC(Player* this, PlayState* play);
void func_8084193C(Player* this, PlayState* play);
void func_80841BA8(Player* this, PlayState* play);
void func_80842180(Player* this, PlayState* play);
void func_8084227C(Player* this, PlayState* play);
void func_8084279C(Player* this, PlayState* play);
void func_808423EC(Player* this, PlayState* play);
void func_8084251C(Player* this, PlayState* play);
void func_80843188(Player* this, PlayState* play);
void func_808435C4(Player* this, PlayState* play);
void func_8084370C(Player* this, PlayState* play);
void func_8084377C(Player* this, PlayState* play);
void func_80843954(Player* this, PlayState* play);
void func_80843A38(Player* this, PlayState* play);
void func_80843CEC(Player* this, PlayState* play);
void func_8084411C(Player* this, PlayState* play);
void func_80844708(Player* this, PlayState* play);
void func_80844A44(Player* this, PlayState* play);
void func_80844AF4(Player* this, PlayState* play);
void func_80844E68(Player* this, PlayState* play);
void func_80845000(Player* this, PlayState* play);
void func_80845308(Player* this, PlayState* play);
void func_80845668(Player* this, PlayState* play);
void func_808458D0(Player* this, PlayState* play);
void func_80845CA4(Player* this, PlayState* play);
void func_80845EF8(Player* this, PlayState* play);
void func_80846050(Player* this, PlayState* play);
void func_80846120(Player* this, PlayState* play);
void func_80846260(Player* this, PlayState* play);
void func_80846358(Player* this, PlayState* play);
void func_80846408(Player* this, PlayState* play);
void func_808464B0(Player* this, PlayState* play);
void func_80846578(Player* this, PlayState* play);
void func_80846648(PlayState* play, Player* this);
void func_80846660(PlayState* play, Player* this);
void func_808467D4(PlayState* play, Player* this);
void func_808468A8(PlayState* play, Player* this);
void func_808468E8(PlayState* play, Player* this);
void func_80846978(PlayState* play, Player* this);
void func_808469BC(PlayState* play, Player* this);
void func_80846A68(PlayState* play, Player* this);
void func_8084B1D8(Player* this, PlayState* play);
void func_8084B530(Player* this, PlayState* play);
void func_8084B78C(Player* this, PlayState* play);
void func_8084B898(Player* this, PlayState* play);
void func_8084B9E4(Player* this, PlayState* play);
void func_8084BBE4(Player* this, PlayState* play);
void func_8084BDFC(Player* this, PlayState* play);
void func_8084BF1C(Player* this, PlayState* play);
void Player_UpdateCommon(Player* this, PlayState* play, Input* input);
void func_8084C5F8(Player* this, PlayState* play);
void func_8084C760(Player* this, PlayState* play);
void func_8084C81C(Player* this, PlayState* play);
void func_8084CC98(Player* this, PlayState* play);
void func_8084D3E4(Player* this, PlayState* play);
void func_8084D610(Player* this, PlayState* play);
void func_8084D7C4(Player* this, PlayState* play);
void func_8084D84C(Player* this, PlayState* play);
void func_8084DAB4(Player* this, PlayState* play);
void func_8084DC48(Player* this, PlayState* play);
void func_8084E1EC(Player* this, PlayState* play);
void func_8084E30C(Player* this, PlayState* play);
void func_8084E368(Player* this, PlayState* play);
void func_8084E3C4(Player* this, PlayState* play);
void func_8084E604(Player* this, PlayState* play);
void func_8084E6D4(Player* this, PlayState* play);
void func_8084E9AC(Player* this, PlayState* play);
void func_8084EAC0(Player* this, PlayState* play);
void func_8084ECA4(Player* this, PlayState* play);
void func_8084EED8(Player* this, PlayState* play);
void func_8084EFC0(Player* this, PlayState* play);
void func_8084F104(Player* this, PlayState* play);
void func_8084F390(Player* this, PlayState* play);
void func_8084F608(Player* this, PlayState* play);
void func_8084F698(Player* this, PlayState* play);
void func_8084F710(Player* this, PlayState* play);
void func_8084F88C(Player* this, PlayState* play);
void func_8084F9A0(Player* this, PlayState* play);
void func_8084F9C0(Player* this, PlayState* play);
void func_8084FA54(Player* this, PlayState* play);
void func_8084FB10(Player* this, PlayState* play);
void func_8084FBF4(Player* this, PlayState* play);
s32 Player_DebugMode(Player* this, PlayState* play);
void func_8084FF7C(Player* this);
void func_8085002C(Player* this);
s32 func_80850224(Player* this, PlayState* play);
void func_808502D0(Player* this, PlayState* play);
void func_808505DC(Player* this, PlayState* play);
void func_8085063C(Player* this, PlayState* play);
void func_8085076C(Player* this, PlayState* play);
void func_808507F4(Player* this, PlayState* play);
void func_80850AEC(Player* this, PlayState* play);
void func_80850C68(Player* this, PlayState* play);
void func_80850E84(Player* this, PlayState* play);
void func_80851008(PlayState* play, Player* this, void* anim);
void func_80851030(PlayState* play, Player* this, void* anim);
void func_80851050(PlayState* play, Player* this, void* anim);
void func_80851094(PlayState* play, Player* this, void* anim);
void func_808510B4(PlayState* play, Player* this, void* anim);
void func_808510D4(PlayState* play, Player* this, void* anim);
void func_808510F4(PlayState* play, Player* this, void* anim);
void func_80851114(PlayState* play, Player* this, void* anim);
void func_80851134(PlayState* play, Player* this, void* anim);
void func_80851154(PlayState* play, Player* this, void* anim);
void func_80851174(PlayState* play, Player* this, void* anim);
void func_80851194(PlayState* play, Player* this, void* anim);
void func_808511B4(PlayState* play, Player* this, void* anim);
void func_808511D4(PlayState* play, Player* this, void* anim);
void func_808511FC(PlayState* play, Player* this, void* anim);
void func_80851248(PlayState* play, Player* this, void* anim);
void func_80851294(PlayState* play, Player* this, void* anim);
void func_808512E0(PlayState* play, Player* this, void* arg2);
void func_80851368(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808513BC(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808514C0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_8085157C(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808515A4(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851688(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851750(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851788(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851828(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808518DC(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_8085190C(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851998(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808519C0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808519EC(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851A50(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851B90(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851BE8(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851CA4(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851D2C(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851D80(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851DEC(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851E28(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851E64(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851E90(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851ECC(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851F84(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80851FB0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852048(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852080(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852174(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808521B8(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808521F4(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852234(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_8085225C(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852280(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852358(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852388(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852298(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852328(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852480(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852450(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808524B0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808524D0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852514(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852544(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852554(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852564(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808525C0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852608(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852648(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808526EC(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_8085283C(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808528C8(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852944(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_808529D0(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852C50(PlayState* play, Player* this, CsCmdActorAction* arg2);
void func_80852E14(Player* this, PlayState* play);
s32 Player_IsDroppingFish(PlayState* play);
s32 Player_StartFishing(PlayState* play);
s32 func_80852F38(PlayState* play, Player* this);
s32 func_80852FFC(PlayState* play, Actor* actor, s32 csMode);
void func_80853080(Player* this, PlayState* play);
s32 Player_InflictDamage(PlayState* play, s32 damage);
void func_80853148(PlayState* play, Actor* actor);