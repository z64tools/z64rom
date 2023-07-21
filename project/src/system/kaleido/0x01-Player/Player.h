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

#define Player_SetActionFunc(play, this, func, flags) ({                                                                                   \
        if (gLibCtx.state.playerPrint) {                                                                                            \
            osLibPrintf("" PRNT_YELW "PLAYER");                                                                                     \
            osLibPrintf("From: " PRNT_GRAY "[" PRNT_BLUE "%s" PRNT_GRAY "::" PRNT_YELW "%d" PRNT_GRAY "]", __FUNCTION__, __LINE__); \
            osLibPrintf("Set:  "PRNT_GRAY "[" PRNT_BLUE #func PRNT_GRAY "]"); }                                                     \
        __Player_SetActionFunc(play, this, func, flags); })

typedef struct {
    /* 0x00 */ u8  itemId;
    /* 0x01 */ u8  field; // various bit-packed data
    /* 0x02 */ s8  gi;         // defines the draw id and chest opening animation
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

typedef PlayerFunc82C PlayerUpperActionFunc;
typedef PlayerFunc674 PlayerActionFunc;
typedef PlayerFuncA74 PlayerCutsceneFunc;

void Player_DoNothing(PlayState * play, Player * this);
void Player_DoNothing2(PlayState * play, Player * this);
void Player_SetupBowOrSlingshot(PlayState * play, Player * this);
void Player_SetupDekuStick(PlayState * play, Player * this);
void Player_SetupExplosive(PlayState * play, Player * this);
void Player_SetupHookshot(PlayState * play, Player * this);
void Player_SetupBoomerang(PlayState * play, Player * this);
void Player_ChangeItem(PlayState * play, Player * this, s8 actionParam);
s32 Player_SetupStartZTargetDefend(Player * this, PlayState * play);
s32 Player_SetupStartZTargetDefend2(Player * this, PlayState * play);
s32 Player_StartChangeItem(Player * this, PlayState * play);
s32 Player_StandingDefend(Player * this, PlayState * play);
s32 Player_EndDefend(Player * this, PlayState * play);
s32 Player_HoldFpsItem(Player * this, PlayState * play);
s32 Player_ReadyFpsItemToShoot(Player * this, PlayState * play);
s32 Player_AimFpsItem(Player * this, PlayState * play);
s32 Player_EndAimFpsItem(Player * this, PlayState * play);
s32 Player_HoldActor(Player * this, PlayState * play);
s32 Player_HoldBoomerang(Player * this, PlayState * play);
s32 Player_SetupAimBoomerang(Player * this, PlayState * play);
s32 Player_AimBoomerang(Player * this, PlayState * play);
s32 Player_ThrowBoomerang(Player * this, PlayState * play);
s32 Player_WaitForThrownBoomerang(Player * this, PlayState * play);
s32 Player_CatchBoomerang(Player * this, PlayState * play);
void Player_UseItem(PlayState * play, Player * this, s32 item);
void Player_SetupStandingStillType(Player * this, PlayState * play);
s32 Player_SetupWallJumpBehavior(Player * this, PlayState * play);
s32 Player_SetupOpenDoor(Player * this, PlayState * play);
s32 Player_SetupItemCsOrFirstPerson(Player * this, PlayState * play);
s32 Player_SetupCUpBehavior(Player * this, PlayState * play);
s32 Player_SetupSpeakOrCheck(Player * this, PlayState * play);
s32 Player_SetupJumpSlashOrRoll(Player * this, PlayState * play);
s32 Player_SetupRollOrPutAway(Player * this, PlayState * play);
s32 Player_SetupDefend(Player * this, PlayState * play);
s32 Player_SetupStartChargeSpinAttack(Player * this, PlayState * play);
s32 Player_SetupThrowDekuNut(PlayState * play, Player * this);
// void func_8083CA20(PlayState * play, Player * this);
// void func_8083CA54(PlayState * play, Player * this);
// void func_8083CA9C(PlayState * play, Player * this);
s32 Player_SetupMountHorse(Player * this, PlayState * play);
s32 Player_SetupGetItemOrHoldBehavior(Player * this, PlayState * play);
s32 Player_SetupPutDownOrThrowActor(Player * this, PlayState * play);
s32 Player_SetupSpecialWallInteraction(Player * this, PlayState * play);

void Player_UnfriendlyZTargetStandingStill(Player * this, PlayState * play);
void Player_FriendlyZTargetStandingStill(Player * this, PlayState * play);
void Player_StandingStill(Player * this, PlayState * play);
void Player_EndSidewalk(Player * this, PlayState * play);
void Player_FriendlyBackwalk(Player * this, PlayState * play);
void Player_HaltFriendlyBackwalk(Player * this, PlayState * play);
void Player_EndHaltFriendlyBackwalk(Player * this, PlayState * play);
void Player_Sidewalk(Player * this, PlayState * play);
void Player_Turn(Player * this, PlayState * play);
void Player_Run(Player * this, PlayState * play);
void Player_ZTargetingRun(Player * this, PlayState * play);
void Player_UnfriendlyBackwalk(Player * this, PlayState * play);
void Player_EndUnfriendlyBackwalk(Player * this, PlayState * play);
void func_8084279C(Player * this, PlayState * play);
void Player_AimShieldCrouched(Player * this, PlayState * play);
void Player_DeflectAttackWithShield(Player * this, PlayState * play);
void func_8084370C(Player * this, PlayState * play);
void Player_StartKnockback(Player * this, PlayState * play);
void Player_DownFromKnockback(Player * this, PlayState * play);
void Player_GetUpFromKnockback(Player * this, PlayState * play);
void Player_Die(Player * this, PlayState * play);
void Player_UpdateMidair(Player * this, PlayState * play);
void Player_Rolling(Player * this, PlayState * play);
void Player_FallingDive(Player * this, PlayState * play);
void Player_JumpSlash(Player * this, PlayState * play);
void Player_ChargeSpinAttack(Player * this, PlayState * play);
void Player_WalkChargingSpinAttack(Player * this, PlayState * play);
void Player_SidewalkChargingSpinAttack(Player * this, PlayState * play);
void Player_JumpUpToLedge(Player * this, PlayState * play);
void Player_RunCutsceneFunc(Player * this, PlayState * play);
void Player_CutsceneMovement(Player * this, PlayState * play);
void Player_OpenDoor(Player * this, PlayState * play);
void Player_LiftActor(Player * this, PlayState * play);
void Player_ThrowStonePillar(Player * this, PlayState * play);
void Player_LiftSilverBoulder(Player * this, PlayState * play);
void Player_ThrowSilverBoulder(Player * this, PlayState * play);
void Player_FailToLiftActor(Player * this, PlayState * play);
void Player_SetupPutDownActor(Player * this, PlayState * play);
void Player_StartThrowActor(Player * this, PlayState * play);
void Player_SpawnNoUpdateOrDraw(PlayState * play, Player * this);
void Player_SetupSpawnFromBlueWarp(PlayState * play, Player * this);
void Player_SpawnFromTimeTravel(PlayState * play, Player * this);
void Player_SpawnOpeningDoor(PlayState * play, Player * this);
void Player_SpawnExitingGrotto(PlayState * play, Player * this);
void Player_SpawnWithKnockback(PlayState * play, Player * this);
void Player_SetupSpawnFromWarpSong(PlayState * play, Player * this);
void Player_SetupSpawnFromFaroresWind(PlayState * play, Player * this);
void Player_FirstPersonAiming(Player * this, PlayState * play);
void Player_TalkWithActor(Player * this, PlayState * play);
void Player_GrabPushPullWall(Player * this, PlayState * play);
void Player_PushWall(Player * this, PlayState * play);
void Player_PullWall(Player * this, PlayState * play);
void Player_GrabLedge(Player * this, PlayState * play);
void Player_ClimbOntoLedge(Player * this, PlayState * play);
void Player_ClimbingWallOrDownLedge(Player * this, PlayState * play);
void Player_UpdateCommon(Player * this, PlayState * play, Input * input);
void Player_EndClimb(Player * this, PlayState * play);
void Player_InsideCrawlspace(Player * this, PlayState * play);
void Player_ExitCrawlspace(Player * this, PlayState * play);
void Player_RideHorse(Player * this, PlayState * play);
void Player_DismountHorse(Player * this, PlayState * play);
void Player_UpdateSwimIdle(Player * this, PlayState * play);
void Player_SpawnSwimming(Player * this, PlayState * play);
void Player_Swim(Player * this, PlayState * play);
void Player_ZTargetSwimming(Player * this, PlayState * play);
void Player_Dive(Player * this, PlayState * play);
void Player_GetItemInWater(Player * this, PlayState * play);
void Player_DamagedSwim(Player * this, PlayState * play);
void Player_Drown(Player * this, PlayState * play);
void Player_PlayOcarina(Player * this, PlayState * play);
void Player_ThrowDekuNut(Player * this, PlayState * play);
void Player_GetItem(Player * this, PlayState * play);
void Player_EndTimeTravel(Player * this, PlayState * play);
void Player_DrinkFromBottle(Player * this, PlayState * play);
void Player_SwingBottle(Player * this, PlayState * play);
void Player_HealWithFairy(Player * this, PlayState * play);
void Player_DropItemFromBottle(Player * this, PlayState * play);
void Player_PresentExchangeItem(Player * this, PlayState * play);
void Player_SlipOnSlope(Player * this, PlayState * play);
void Player_SetDrawAndStartCutsceneAfterTimer(Player * this, PlayState * play);
void Player_SpawnFromWarpSong(Player * this, PlayState * play);
void Player_SpawnFromBlueWarp(Player * this, PlayState * play);
void Player_EnterGrotto(Player * this, PlayState * play);
void Player_SetupOpenDoorFromSpawn(Player * this, PlayState * play);
void Player_JumpFromGrotto(Player * this, PlayState * play);
void Player_ShootingGalleryPlay(Player * this, PlayState * play);
void Player_FrozenInIce(Player * this, PlayState * play);
void Player_SetupElectricShock(Player * this, PlayState * play);
s32 Player_DebugMode(Player * this, PlayState * play);
void Player_BowStringMoveAfterShot(Player * this);
void Player_BunnyHoodPhysics(Player * this);
s32 Player_SetupStartMeleeWeaponAttack(Player * this, PlayState * play);
void Player_MeleeWeaponAttack(Player * this, PlayState * play);
void Player_MeleeWeaponRebound(Player * this, PlayState * play);
void Player_ChooseFaroresWindOption(Player * this, PlayState * play);
void Player_SpawnFromFaroresWind(Player * this, PlayState * play);
void Player_UpdateMagicSpell(Player * this, PlayState * play);
void Player_MoveAlongHookshotPath(Player * this, PlayState * play);
void Player_CastFishingRod(Player * this, PlayState * play);
void Player_ReleaseCaughtFish(Player * this, PlayState * play);

void Player_AnimPlaybackType0(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType1(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType13(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType2(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType3(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType4(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType5(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType6(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType7(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType8(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType9(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType14(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType15(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType10(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType11(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType16(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType12(PlayState * play, Player * this, void* anim);
void Player_AnimPlaybackType17(PlayState * play, Player * this, void* arg2);
void Player_Cutscene_SetupSwimIdle(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SurfaceFromDive(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_Idle(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_TurnAroundSurprisedShort(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupIdle(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_Wait(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_TurnAroundSurprisedLong(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupEnterWarp(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_EnterWarp(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupFightStance(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_FightStance(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_Unk3Update(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_Unk4Update(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupSwordPedestal(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SwordPedestal(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupWarpToSages(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_WarpToSages(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_KnockedToGround(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupStartPlayOcarina(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_DrawAndBrandishSword(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_CloseEyes(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_OpenEyes(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupGetItemInWater(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupSleeping(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_Sleeping(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupSleepingRestless(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_Awaken(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_GetOffBed(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupBlownBackward(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_BlownBackward(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_RaisedByWarp(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupIdle3(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_Idle3(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupStop(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetDraw(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_DrawSwordChild(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_TurnAroundSlowly(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_InspectGroundCarefully(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_StartPassOcarina(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_StepBackCautiously(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_DesperateLookAtZeldasCrystal(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupSpinAttackIdle(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SpinAttackIdle(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_InspectWeapon(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_SetupDoNothing4(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_DoNothing5(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupKnockedToGroundDamaged(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_KnockedToGroundDamaged(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SetupGetSwordBack(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_SwordKnockedFromHand(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_LearnOcarinaSong(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_GetSwordBack(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_Cutscene_GanonKillCombo(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_CutsceneEnd(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_CutsceneSetPosAndYaw(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_CutsceneUnk6Update(PlayState * play, Player * this, CsCmdActorAction * arg2);
void Player_StartCutscene(Player * this, PlayState * play);
s32 Player_IsDroppingFish(PlayState* play);
s32 Player_StartFishing(PlayState* play);
s32 Player_SetupRestrainedByEnemy(PlayState * play, Player * this);
s32 Player_SetupPlayerCutscene(PlayState* play, Actor* actor, s32 csMode);
void Player_SetupStandingStillMorph(Player * this, PlayState * play);
s32 Player_InflictDamage(PlayState* play, s32 damage);
void Player_StartTalkingWithActor(PlayState* play, Actor* actor);