/*
 * File: z_player.c
 * Overlay: ovl_player_actor
 * Description: Link
 */

#include <asm_macros.h>
#include "Player.h"

Asm_SymbolAlias("__z64_init", Player_Init);
Asm_SymbolAlias("__z64_dest", Player_Destroy);
Asm_SymbolAlias("__z64_updt", Player_Update);
Asm_SymbolAlias("__z64_draw", Player_Draw);

// .bss part 1
static s32 sPrevSkelAnimeMoveFlags;
static s32 sCurrentMask;
static Vec3f sWallIntersectPos;
static Input* sControlInput;

// .data

static u8 D_80853410[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

static PlayerAgeProperties sAgeProperties[] = {
    {
        56.0f,
        90.0f,
        1.0f,
        111.0f,
        70.0f,
        79.4f,
        59.0f,
        41.0f,
        19.0f,
        36.0f,
        44.8f,
        56.0f,
        68.0f,
        70.0f,
        18.0f,
        15.0f,
        70.0f,
        { 9,                                     4671,                                359                                 },
        {
            { 8,                                     4694,                                380                                 },
            { 9,                                     6122,                                359                                 },
            { 8,                                     4694,                                380                                 },
            { 9,                                     6122,                                359                                 },
        },
        {
            { 9,                                     6122,                                359                                 },
            { 9,                                     7693,                                380                                 },
            { 9,                                     6122,                                359                                 },
            { 9,                                     7693,                                380                                 },
        },
        {
            { 8,                                     4694,                                380                                 },
            { 9,                                     6122,                                359                                 },
        },
        {
            { -1592,                                 4694,                                380                                 },
            { -1591,                                 6122,                                359                                 },
        },
        0,
        0x80,
        &gPlayerAnim_link_demo_Tbox_open,
        &gPlayerAnim_link_demo_back_to_past,
        &gPlayerAnim_link_demo_return_to_past,
        &gPlayerAnim_link_normal_climb_startA,
        &gPlayerAnim_link_normal_climb_startB,
        { &gPlayerAnim_link_normal_climb_upL,    &gPlayerAnim_link_normal_climb_upR,  &gPlayerAnim_link_normal_Fclimb_upL, &gPlayerAnim_link_normal_Fclimb_upR},
        { &gPlayerAnim_link_normal_Fclimb_sideL, &gPlayerAnim_link_normal_Fclimb_sideR },
        { &gPlayerAnim_link_normal_climb_endAL,  &gPlayerAnim_link_normal_climb_endAR },
        { &gPlayerAnim_link_normal_climb_endBR,  &gPlayerAnim_link_normal_climb_endBL },
    },
    {
        40.0f,
        60.0f,
        11.0f / 17.0f,
        71.0f,
        50.0f,
        47.0f,
        39.0f,
        27.0f,
        19.0f,
        22.0f,
        29.6f,
        32.0f,
        48.0f,
        70.0f * (11.0f / 17.0f),
        14.0f,
        12.0f,
        55.0f,
        { -24,                                   3565,                                876                                 },
        {
            { -24,                                   3474,                                862                                 },
            { -24,                                   4977,                                937                                 },
            { 8,                                     4694,                                380                                 },
            { 9,                                     6122,                                359                                 },
        },
        {
            { -24,                                   4977,                                937                                 },
            { -24,                                   6495,                                937                                 },
            { 9,                                     6122,                                359                                 },
            { 9,                                     7693,                                380                                 },
        },
        {
            { 8,                                     4694,                                380                                 },
            { 9,                                     6122,                                359                                 },
        },
        {
            { -1592,                                 4694,                                380                                 },
            { -1591,                                 6122,                                359                                 },
        },
        0x20,
        0,
        &gPlayerAnim_clink_demo_Tbox_open,
        &gPlayerAnim_clink_demo_goto_future,
        &gPlayerAnim_clink_demo_return_to_future,
        &gPlayerAnim_clink_normal_climb_startA,
        &gPlayerAnim_clink_normal_climb_startB,
        { &gPlayerAnim_clink_normal_climb_upL,   &gPlayerAnim_clink_normal_climb_upR, &gPlayerAnim_link_normal_Fclimb_upL, &gPlayerAnim_link_normal_Fclimb_upR},
        { &gPlayerAnim_link_normal_Fclimb_sideL, &gPlayerAnim_link_normal_Fclimb_sideR },
        { &gPlayerAnim_clink_normal_climb_endAL, &gPlayerAnim_clink_normal_climb_endAR },
        { &gPlayerAnim_clink_normal_climb_endBR, &gPlayerAnim_clink_normal_climb_endBL },
    },
};

static u32 sDebugModeFlag = false;
static f32 sAnalogStickMod = 0.0f;
static s16 sAnalogStickYaw = 0;
static s16 sAnalogStickYawCamOffset = 0;
static s32 D_808535E0 = 0;
static s32 sFloorSpecialProperty = 0;
static f32 sWaterSpeedScale = 1.0f;
static f32 sWaterSpeedInvScale = 1.0f;
static u32 sTouchedWallFlags = 0;
static u32 sConveyorSpeedIndex = 0;
static s16 sIsFloorConveyor = false;
static s16 sConveyorYaw = 0;
static f32 sFloorDistY = 0.0f;
static s32 sFloorProperty = 0;
static s32 sTouchedWallYaw = 0;
static s32 sTouchedWallYaw2 = 0;
static s16 sFloorPitch = 0;
static s32 sActiveItemUseFlag = 0;
static s32 sActiveItemUseFlag2 = 0;

static u16 sInterruptableSfx[] = {
    NA_SE_VO_LI_SWEAT,
    NA_SE_VO_LI_SNEEZE,
    NA_SE_VO_LI_RELAX,
    NA_SE_VO_LI_FALL_L,
};

static GetItemEntry sGetItemTable[] = {
    GET_ITEM(ITEM_BOMBS_5,          OBJECT_GI_BOMB_1,              GID_BOMB,                     0x32,              0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUTS_5,           OBJECT_GI_NUTS,                GID_NUTS,                     0x34,              0x0C, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBCHU,          OBJECT_GI_BOMB_2,              GID_BOMBCHU,                  0x33,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOW,              OBJECT_GI_BOW,                 GID_BOW,                      0x31,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SLINGSHOT,        OBJECT_GI_PACHINKO,            GID_SLINGSHOT,                0x30,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOOMERANG,        OBJECT_GI_BOOMERANG,           GID_BOOMERANG,                0x35,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STICK,            OBJECT_GI_STICK,               GID_STICK,                    0x37,              0x0D, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HOOKSHOT,         OBJECT_GI_HOOKSHOT,            GID_HOOKSHOT,                 0x36,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LONGSHOT,         OBJECT_GI_HOOKSHOT,            GID_LONGSHOT,                 0x4F,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LENS,             OBJECT_GI_GLASSES,             GID_LENS,                     0x39,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LETTER_ZELDA,     OBJECT_GI_LETTER,              GID_LETTER_ZELDA,             0x69,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_OCARINA_TIME,     OBJECT_GI_OCARINA,             GID_OCARINA_TIME,             0x3A,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_HAMMER,           OBJECT_GI_HAMMER,              GID_HAMMER,                   0x38,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_COJIRO,           OBJECT_GI_NIWATORI,            GID_COJIRO,                   0x02,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOTTLE,           OBJECT_GI_BOTTLE,              GID_BOTTLE,                   0x42,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POTION_RED,       OBJECT_GI_LIQUID,              GID_POTION_RED,               0x43,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POTION_GREEN,     OBJECT_GI_LIQUID,              GID_POTION_GREEN,             0x44,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POTION_BLUE,      OBJECT_GI_LIQUID,              GID_POTION_BLUE,              0x45,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_FAIRY,            OBJECT_GI_BOTTLE,              GID_BOTTLE,                   0x46,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MILK_BOTTLE,      OBJECT_GI_MILK,                GID_MILK,                     0x98,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_LETTER_RUTO,      OBJECT_GI_BOTTLE_LETTER,       GID_LETTER_RUTO,              0x99,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BEAN,             OBJECT_GI_BEAN,                GID_BEAN,                     0x48,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_MASK_SKULL,       OBJECT_GI_SKJ_MASK,            GID_MASK_SKULL,               0x10,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_SPOOKY,      OBJECT_GI_REDEAD_MASK,         GID_MASK_SPOOKY,              0x11,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_CHICKEN,          OBJECT_GI_NIWATORI,            GID_CHICKEN,                  0x48,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_KEATON,      OBJECT_GI_KI_TAN_MASK,         GID_MASK_KEATON,              0x12,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_BUNNY,       OBJECT_GI_RABIT_MASK,          GID_MASK_BUNNY,               0x13,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_TRUTH,       OBJECT_GI_TRUTH_MASK,          GID_MASK_TRUTH,               0x17,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POCKET_EGG,       OBJECT_GI_EGG,                 GID_EGG,                      0x01,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POCKET_CUCCO,     OBJECT_GI_NIWATORI,            GID_CHICKEN,                  0x48,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ODD_MUSHROOM,     OBJECT_GI_MUSHROOM,            GID_ODD_MUSHROOM,             0x03,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ODD_POTION,       OBJECT_GI_POWDER,              GID_ODD_POTION,               0x04,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SAW,              OBJECT_GI_SAW,                 GID_SAW,                      0x05,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SWORD_BROKEN,     OBJECT_GI_BROKENSWORD,         GID_SWORD_BROKEN,             0x08,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_PRESCRIPTION,     OBJECT_GI_PRESCRIPTION,        GID_PRESCRIPTION,             0x09,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_FROG,             OBJECT_GI_FROG,                GID_FROG,                     0x0D,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_EYEDROPS,         OBJECT_GI_EYE_LOTION,          GID_EYEDROPS,                 0x0E,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_CLAIM_CHECK,      OBJECT_GI_TICKETSTONE,         GID_CLAIM_CHECK,              0x0A,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SWORD_KOKIRI,     OBJECT_GI_SWORD_1,             GID_SWORD_KOKIRI,             0xA4,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SWORD_BGS,        OBJECT_GI_LONGSWORD,           GID_SWORD_BGS,                0x4B,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SHIELD_DEKU,      OBJECT_GI_SHIELD_1,            GID_SHIELD_DEKU,              0x4C,              0xA0, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SHIELD_HYLIAN,    OBJECT_GI_SHIELD_2,            GID_SHIELD_HYLIAN,            0x4D,              0xA0, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SHIELD_MIRROR,    OBJECT_GI_SHIELD_3,            GID_SHIELD_MIRROR,            0x4E,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_TUNIC_GORON,      OBJECT_GI_CLOTHES,             GID_TUNIC_GORON,              0x50,              0xA0, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_TUNIC_ZORA,       OBJECT_GI_CLOTHES,             GID_TUNIC_ZORA,               0x51,              0xA0, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOOTS_IRON,       OBJECT_GI_BOOTS_2,             GID_BOOTS_IRON,               0x53,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOOTS_HOVER,      OBJECT_GI_HOVERBOOTS,          GID_BOOTS_HOVER,              0x54,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_QUIVER_40,        OBJECT_GI_ARROWCASE,           GID_QUIVER_40,                0x56,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_QUIVER_50,        OBJECT_GI_ARROWCASE,           GID_QUIVER_50,                0x57,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOMB_BAG_20,      OBJECT_GI_BOMBPOUCH,           GID_BOMB_BAG_20,              0x58,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOMB_BAG_30,      OBJECT_GI_BOMBPOUCH,           GID_BOMB_BAG_30,              0x59,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BOMB_BAG_40,      OBJECT_GI_BOMBPOUCH,           GID_BOMB_BAG_40,              0x5A,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_GAUNTLETS_SILVER, OBJECT_GI_GLOVES,              GID_GAUNTLETS_SILVER,         0x5B,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_GAUNTLETS_GOLD,   OBJECT_GI_GLOVES,              GID_GAUNTLETS_GOLD,           0x5C,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SCALE_SILVER,     OBJECT_GI_SCALE,               GID_SCALE_SILVER,             0xCD,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SCALE_GOLDEN,     OBJECT_GI_SCALE,               GID_SCALE_GOLDEN,             0xCE,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STONE_OF_AGONY,   OBJECT_GI_MAP,                 GID_STONE_OF_AGONY,           0x68,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_GERUDO_CARD,      OBJECT_GI_GERUDO,              GID_GERUDO_CARD,              0x7B,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_OCARINA_FAIRY,    OBJECT_GI_OCARINA_0,           GID_OCARINA_FAIRY,            0x3A,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SEEDS,            OBJECT_GI_SEED,                GID_SEEDS,                    0xDC,              0x50, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HEART_CONTAINER,  OBJECT_GI_HEARTS,              GID_HEART_CONTAINER,          0xC6,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_HEART_PIECE_2,    OBJECT_GI_HEARTS,              GID_HEART_PIECE,              0xC2,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_KEY_BOSS,         OBJECT_GI_BOSSKEY,             GID_KEY_BOSS,                 0xC7,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_COMPASS,          OBJECT_GI_COMPASS,             GID_COMPASS,                  0x67,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_DUNGEON_MAP,      OBJECT_GI_MAP,                 GID_DUNGEON_MAP,              0x66,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_KEY_SMALL,        OBJECT_GI_KEY,                 GID_KEY_SMALL,                0x60,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_MAGIC_SMALL,      OBJECT_GI_MAGICPOT,            GID_MAGIC_SMALL,              0x52,              0x6F, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_MAGIC_LARGE,      OBJECT_GI_MAGICPOT,            GID_MAGIC_LARGE,              0x52,              0x6E, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_WALLET_ADULT,     OBJECT_GI_PURSE,               GID_WALLET_ADULT,             0x5E,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_WALLET_GIANT,     OBJECT_GI_PURSE,               GID_WALLET_GIANT,             0x5F,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_WEIRD_EGG,        OBJECT_GI_EGG,                 GID_EGG,                      0x9A,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_RECOVERY_HEART,   OBJECT_GI_HEART,               GID_RECOVERY_HEART,           0x55,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROWS_SMALL,     OBJECT_GI_ARROW,               GID_ARROWS_SMALL,             0xE6,              0x48, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_ARROWS_MEDIUM,    OBJECT_GI_ARROW,               GID_ARROWS_MEDIUM,            0xE6,              0x49, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_ARROWS_LARGE,     OBJECT_GI_ARROW,               GID_ARROWS_LARGE,             0xE6,              0x4A, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_GREEN,      OBJECT_GI_RUPY,                GID_RUPEE_GREEN,              0x6F,              0x00, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_BLUE,       OBJECT_GI_RUPY,                GID_RUPEE_BLUE,               0xCC,              0x01, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_RED,        OBJECT_GI_RUPY,                GID_RUPEE_RED,                0xF0,              0x02, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HEART_CONTAINER,  OBJECT_GI_HEARTS,              GID_HEART_CONTAINER,          0xC6,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MILK,             OBJECT_GI_MILK,                GID_MILK,                     0x98,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_GORON,       OBJECT_GI_GOLONMASK,           GID_MASK_GORON,               0x14,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_ZORA,        OBJECT_GI_ZORAMASK,            GID_MASK_ZORA,                0x15,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_MASK_GERUDO,      OBJECT_GI_GERUDOMASK,          GID_MASK_GERUDO,              0x16,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BRACELET,         OBJECT_GI_BRACELET,            GID_BRACELET,                 0x79,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_RUPEE_PURPLE,     OBJECT_GI_RUPY,                GID_RUPEE_PURPLE,             0xF1,              0x14, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_GOLD,       OBJECT_GI_RUPY,                GID_RUPEE_GOLD,               0xF2,              0x13, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SWORD_BGS,        OBJECT_GI_LONGSWORD,           GID_SWORD_BGS,                0x0C,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROW_FIRE,       OBJECT_GI_M_ARROW,             GID_ARROW_FIRE,               0x70,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROW_ICE,        OBJECT_GI_M_ARROW,             GID_ARROW_ICE,                0x71,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_ARROW_LIGHT,      OBJECT_GI_M_ARROW,             GID_ARROW_LIGHT,              0x72,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_SKULL_TOKEN,      OBJECT_GI_SUTARU,              GID_SKULL_TOKEN,              0xB4,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_DINS_FIRE,        OBJECT_GI_GODDESS,             GID_DINS_FIRE,                0xAD,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_FARORES_WIND,     OBJECT_GI_GODDESS,             GID_FARORES_WIND,             0xAE,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_NAYRUS_LOVE,      OBJECT_GI_GODDESS,             GID_NAYRUS_LOVE,              0xAF,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BULLET_BAG_30,    OBJECT_GI_DEKUPOUCH,           GID_BULLET_BAG,               0x07,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BULLET_BAG_40,    OBJECT_GI_DEKUPOUCH,           GID_BULLET_BAG,               0x07,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STICKS_5,         OBJECT_GI_STICK,               GID_STICK,                    0x37,              0x0D, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_STICKS_10,        OBJECT_GI_STICK,               GID_STICK,                    0x37,              0x0D, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUTS_5,           OBJECT_GI_NUTS,                GID_NUTS,                     0x34,              0x0C, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUTS_10,          OBJECT_GI_NUTS,                GID_NUTS,                     0x34,              0x0C, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMB,             OBJECT_GI_BOMB_1,              GID_BOMB,                     0x32,              0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBS_10,         OBJECT_GI_BOMB_1,              GID_BOMB,                     0x32,              0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBS_20,         OBJECT_GI_BOMB_1,              GID_BOMB,                     0x32,              0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBS_30,         OBJECT_GI_BOMB_1,              GID_BOMB,                     0x32,              0x59, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_SEEDS_30,         OBJECT_GI_SEED,                GID_SEEDS,                    0xDC,              0x50, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBCHUS_5,       OBJECT_GI_BOMB_2,              GID_BOMBCHU,                  0x33,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BOMBCHUS_20,      OBJECT_GI_BOMB_2,              GID_BOMBCHU,                  0x33,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_FISH,             OBJECT_GI_FISH,                GID_FISH,                     0x47,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BUG,              OBJECT_GI_INSECT,              GID_BUG,                      0x7A,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BLUE_FIRE,        OBJECT_GI_FIRE,                GID_BLUE_FIRE,                0x5D,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_POE,              OBJECT_GI_GHOST,               GID_POE,                      0x97,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_BIG_POE,          OBJECT_GI_GHOST,               GID_BIG_POE,                  0xF9,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_KEY_SMALL,        OBJECT_GI_KEY,                 GID_KEY_SMALL,                0xF3,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_GREEN,      OBJECT_GI_RUPY,                GID_RUPEE_GREEN,              0xF4,              0x00, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_BLUE,       OBJECT_GI_RUPY,                GID_RUPEE_BLUE,               0xF5,              0x01, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_RED,        OBJECT_GI_RUPY,                GID_RUPEE_RED,                0xF6,              0x02, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_RUPEE_PURPLE,     OBJECT_GI_RUPY,                GID_RUPEE_PURPLE,             0xF7,              0x14, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_HEART_PIECE_2,    OBJECT_GI_HEARTS,              GID_HEART_PIECE,              0xFA,              0x80, CHEST_ANIM_LONG),
    GET_ITEM(ITEM_STICK_UPGRADE_20, OBJECT_GI_STICK,               GID_STICK,                    0x90,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_STICK_UPGRADE_30, OBJECT_GI_STICK,               GID_STICK,                    0x91,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUT_UPGRADE_30,   OBJECT_GI_NUTS,                GID_NUTS,                     0xA7,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_NUT_UPGRADE_40,   OBJECT_GI_NUTS,                GID_NUTS,                     0xA8,              0x80, CHEST_ANIM_SHORT),
    GET_ITEM(ITEM_BULLET_BAG_50,    OBJECT_GI_DEKUPOUCH,           GID_BULLET_BAG_50,            0x6C,              0x80, CHEST_ANIM_LONG),
    GET_ITEM_NONE,
    GET_ITEM_NONE,
};

#define GET_PLAYER_ANIM(group, type) sAnims_AnimTypeGroup[group * PLAYER_ANIMTYPE_MAX + type]

static LinkAnimationHeader* sAnims_AnimTypeGroup[PLAYER_ANIMGROUP_MAX * PLAYER_ANIMTYPE_MAX] = {
    /* PLAYER_ANIMGROUP_STANDING_STILL */
    &gPlayerAnim_link_normal_wait_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_wait, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_wait, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_wait_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_wait_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_wait_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_WALKING */
    &gPlayerAnim_link_normal_walk_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_walk, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_walk, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_walk_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_walk_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_RUNNING */
    &gPlayerAnim_link_normal_run_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_fighter_run, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_run, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_run_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_run_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_run_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_RUNNING_DAMAGED */
    &gPlayerAnim_link_normal_damage_run_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_fighter_damage_run, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_damage_run_free, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_damage_run_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_damage_run_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_damage_run_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_IRON_BOOTS */
    &gPlayerAnim_link_normal_heavy_run_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_heavy_run, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_heavy_run_free, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_heavy_run_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_heavy_run_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_heavy_run_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_FIGHTING_LEFT_OF_ENEMY */
    &gPlayerAnim_link_normal_waitL_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_waitL, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_waitL, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitL_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_waitL_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_waitL_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_FIGHTING_RIGHT_OF_ENEMY */
    &gPlayerAnim_link_normal_waitR_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_waitR, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_waitR, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_waitR_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_waitR_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_START_FIGHTING */
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_wait2waitR, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_wait2waitR, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_fighter_wait2waitR_long, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_8 */
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_fighter_normal2fighter, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_fighter_normal2fighter, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_normal2fighter_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_LEFT */
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_demo_doorA_link_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_LEFT */
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_clink_demo_doorA_link, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_ADULT_RIGHT */
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_demo_doorB_link_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_OPEN_DOOR_CHILD_RIGHT */
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_clink_demo_doorB_link, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_HOLDING_OBJECT */
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_carryB, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_carryB, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_carryB_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_TALL_JUMP_LANDING */
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_landing, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_landing, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_landing_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SHORT_JUMP_LANDING */
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_short_landing, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_short_landing, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_short_landing_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_ROLLING */
    &gPlayerAnim_link_normal_landing_roll_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_landing_roll, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_landing_roll, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_landing_roll_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_landing_roll_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_landing_roll_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_ROLL_BONKING */
    &gPlayerAnim_link_normal_hip_down_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_hip_down, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_hip_down, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_hip_down_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_hip_down_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_hip_down_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_WALK_ON_LEFT_FOOT */
    &gPlayerAnim_link_normal_walk_endL_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_walk_endL, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endL, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_walk_endL_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endL_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_walk_endL_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_WALK_ON_RIGHT_FOOT */
    &gPlayerAnim_link_normal_walk_endR_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_walk_endR, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endR, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_walk_endR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_walk_endR_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_walk_endR_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_START_DEFENDING */
    &gPlayerAnim_link_normal_defense_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_defense, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_defense, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_defense_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_bow_defense, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_defense_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_DEFENDING */
    &gPlayerAnim_link_normal_defense_wait_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_defense_wait, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_defense_wait, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_defense_wait_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_bow_defense_wait, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_defense_wait_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_DEFENDING */
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_defense_end, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_defense_end, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_defense_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_SIDEWALKING */
    &gPlayerAnim_link_normal_side_walk_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_side_walk, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walk, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_side_walk_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walk_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_side_walk_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SIDEWALKING_LEFT */
    &gPlayerAnim_link_normal_side_walkL_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_side_walkL, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_side_walkL, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_side_walkL_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walkL_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_side_walkL_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SIDEWALKING_RIGHT */
    &gPlayerAnim_link_normal_side_walkR_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_anchor_side_walkR, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_anchor_side_walkR, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_side_walkR_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_side_walkR_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_side_walkR_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SHUFFLE_TURN */
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_45_turn, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_45_turn, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_45_turn_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_FIGHTING_LEFT_OF_ENEMY */
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_waitL2wait, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_waitL2wait, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_fighter_waitL2wait_long, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_FIGHTING_RIGHT_OF_ENEMY */
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_waitR2wait, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_waitR2wait, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_fighter_waitR2wait_long, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_THROWING_OBJECT */
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_throw, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_throw, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_throw_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_PUTTING_DOWN_OBJECT */
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_put, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_put, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_put_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_BACKWALKING */
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_back_walk, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_START_CHECKING_OR_SPEAKING */
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_check, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_check, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_check_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_CHECKING_OR_SPEAKING */
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_check_wait, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_check_wait, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_check_wait_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_CHECKING_OR_SPEAKING */
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_check_end, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_check_end, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_check_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_PULL_OBJECT */
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_pull_start, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_start, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_pull_start_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_PULL_OBJECT */
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_pulling, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_pulling, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_pulling_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_PUSH_OBJECT */
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_pull_end, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_end, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_pull_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_KNOCKED_FROM_CLIMBING */
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_fall_up, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_fall_up, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_fall_up_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_HANGING_FROM_LEDGE */
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_jump_climb_hold, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_hold, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_jump_climb_hold_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_CLIMBING_IDLE */
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_jump_climb_wait, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_wait, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_jump_climb_wait_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_CLIMBING */
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_jump_climb_up, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_up, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_jump_climb_up_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_SLIDING_DOWN_SLOPE */
    &gPlayerAnim_link_normal_down_slope_slip_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_down_slope_slip_end, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_down_slope_slip_end, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_down_slope_slip_end_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_down_slope_slip_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_down_slope_slip_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_END_SLIDING_DOWN_SLOPE */
    &gPlayerAnim_link_normal_up_slope_slip_end_free, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_link_normal_up_slope_slip_end, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_link_normal_up_slope_slip_end, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_link_normal_up_slope_slip_end_long, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_link_normal_up_slope_slip_end_free, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_link_normal_up_slope_slip_end_free, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
    /* PLAYER_ANIMGROUP_RELAX */
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_DEFAULT
    &gPlayerAnim_lkt_nwait, // PLAYER_ANIMTYPE_HOLDING_ONE_HAND_WEAPON
    &gPlayerAnim_lkt_nwait, // PLAYER_ANIMTYPE_HOLDING_SHIELD
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_HOLDING_TWO_HAND_WEAPON
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_HOLDING_ITEM_IN_LEFT_HAND
    &gPlayerAnim_sude_nwait, // PLAYER_ANIMTYPE_USED_EXPLOSIVE
};

static LinkAnimationHeader* sAnims_ManualJump[][3] = {
    { &gPlayerAnim_link_fighter_front_jump,    &gPlayerAnim_link_fighter_front_jump_end,    &gPlayerAnim_link_fighter_front_jump_endR    },
    { &gPlayerAnim_link_fighter_Lside_jump,    &gPlayerAnim_link_fighter_Lside_jump_end,    &gPlayerAnim_link_fighter_Lside_jump_endL    },
    { &gPlayerAnim_link_fighter_backturn_jump, &gPlayerAnim_link_fighter_backturn_jump_end, &gPlayerAnim_link_fighter_backturn_jump_endR },
    { &gPlayerAnim_link_fighter_Rside_jump,    &gPlayerAnim_link_fighter_Rside_jump_end,    &gPlayerAnim_link_fighter_Rside_jump_endR    },
};

static LinkAnimationHeader* sAnims_Idle[][2] = {
    { &gPlayerAnim_link_normal_wait_typeA_20f, &gPlayerAnim_link_normal_waitF_typeA_20f }, { &gPlayerAnim_link_normal_wait_typeC_20f, &gPlayerAnim_link_normal_waitF_typeC_20f },
    { &gPlayerAnim_link_normal_wait_typeB_20f, &gPlayerAnim_link_normal_waitF_typeB_20f }, { &gPlayerAnim_link_normal_wait_typeB_20f, &gPlayerAnim_link_normal_waitF_typeB_20f },
    { &gPlayerAnim_link_wait_typeD_20f,        &gPlayerAnim_link_waitF_typeD_20f        }, { &gPlayerAnim_link_wait_typeD_20f,        &gPlayerAnim_link_waitF_typeD_20f        },
    { &gPlayerAnim_link_wait_typeD_20f,        &gPlayerAnim_link_waitF_typeD_20f        }, { &gPlayerAnim_link_wait_heat1_20f,        &gPlayerAnim_link_waitF_heat1_20f        },
    { &gPlayerAnim_link_wait_heat2_20f,        &gPlayerAnim_link_waitF_heat2_20f        }, { &gPlayerAnim_link_wait_itemD1_20f,       &gPlayerAnim_link_wait_itemD1_20f        },
    { &gPlayerAnim_link_wait_itemA_20f,        &gPlayerAnim_link_waitF_itemA_20f        }, { &gPlayerAnim_link_wait_itemB_20f,        &gPlayerAnim_link_waitF_itemB_20f        },
    { &gPlayerAnim_link_wait_itemC_20f,        &gPlayerAnim_link_wait_itemC_20f         }, { &gPlayerAnim_link_wait_itemD2_20f,       &gPlayerAnim_link_wait_itemD2_20f        }
};

static struct_80832924 sAnimSfx_Idle_Sneeze[] = {
    { NA_SE_VO_LI_SNEEZE, -0x2008 },
};

static struct_80832924 sAnimSfx_Idle_Sweat[] = {
    { NA_SE_VO_LI_SWEAT, -0x2012 },
};

static struct_80832924 D_80853DF4[] = {
    { NA_SE_VO_LI_BREATH_REST, -0x200D },
};

static struct_80832924 D_80853DF8[] = {
    { NA_SE_VO_LI_BREATH_REST, -0x200A },
};

static struct_80832924 D_80853DFC[] = {
    { NA_SE_PL_CALM_HIT, 0x82C }, { NA_SE_PL_CALM_HIT, 0x830  },  { NA_SE_PL_CALM_HIT, 0x834 },
    { NA_SE_PL_CALM_HIT, 0x838 }, { NA_SE_PL_CALM_HIT, -0x83C },
};

static struct_80832924 D_80853E10[] = {
    { 0, 0x4019 }, { 0, 0x401E }, { 0, 0x402C }, { 0, 0x4030 }, { 0, 0x4034 }, { 0, -0x4038 },
};

static struct_80832924 D_80853E28[] = {
    { NA_SE_IT_SHIELD_POSTURE, 0x810  },
    { NA_SE_IT_SHIELD_POSTURE, 0x814  },
    { NA_SE_IT_SHIELD_POSTURE, -0x846 },
};

static struct_80832924 D_80853E34[] = {
    { NA_SE_IT_HAMMER_SWING, 0x80A     },
    { NA_SE_VO_LI_AUTO_JUMP, 0x200A    },
    { NA_SE_IT_SWORD_SWING,  0x816     },
    { NA_SE_VO_LI_SWORD_N,   -0x2016   },
};

static struct_80832924 D_80853E44[] = {
    { NA_SE_IT_SWORD_SWING, 0x827    },
    { NA_SE_VO_LI_SWORD_N,  -0x2027  },
};

static struct_80832924 D_80853E4C[] = {
    { NA_SE_VO_LI_RELAX, -0x2014 },
};

static struct_80832924* sAnimSfx_Idle[] = {
    sAnimSfx_Idle_Sneeze, sAnimSfx_Idle_Sweat, D_80853DF4, D_80853DF8, D_80853DFC, D_80853E10,
    D_80853E28,           D_80853E34,          D_80853E44, D_80853E4C, NULL,
};

static u8 sAnimSfx_IndexOffsetTable[] = {
    0, 0, 1, 1, 2, 2, 2, 2, 10, 10, 10, 10, 10, 10, 3, 3, 4, 4, 8, 8, 5, 5, 6, 6, 7, 7, 9, 9, 0,
};

// Used to map item IDs to action params
static s8 sItemActionParams[] = {
    PLAYER_AP_STICK,
    PLAYER_AP_NUT,
    PLAYER_AP_BOMB,
    PLAYER_AP_BOW,
    PLAYER_AP_BOW_FIRE,
    PLAYER_AP_DINS_FIRE,
    PLAYER_AP_SLINGSHOT,
    PLAYER_AP_OCARINA_FAIRY,
    PLAYER_AP_OCARINA_TIME,
    PLAYER_AP_BOMBCHU,
    PLAYER_AP_HOOKSHOT,
    PLAYER_AP_LONGSHOT,
    PLAYER_AP_BOW_ICE,
    PLAYER_AP_FARORES_WIND,
    PLAYER_AP_BOOMERANG,
    PLAYER_AP_LENS,
    PLAYER_AP_BEAN,
    PLAYER_AP_HAMMER,
    PLAYER_AP_BOW_LIGHT,
    PLAYER_AP_NAYRUS_LOVE,
    PLAYER_AP_BOTTLE,
    PLAYER_AP_BOTTLE_POTION_RED,
    PLAYER_AP_BOTTLE_POTION_GREEN,
    PLAYER_AP_BOTTLE_POTION_BLUE,
    PLAYER_AP_BOTTLE_FAIRY,
    PLAYER_AP_BOTTLE_FISH,
    PLAYER_AP_BOTTLE_MILK,
    PLAYER_AP_BOTTLE_LETTER,
    PLAYER_AP_BOTTLE_FIRE,
    PLAYER_AP_BOTTLE_BUG,
    PLAYER_AP_BOTTLE_BIG_POE,
    PLAYER_AP_BOTTLE_MILK_HALF,
    PLAYER_AP_BOTTLE_POE,
    PLAYER_AP_WEIRD_EGG,
    PLAYER_AP_CHICKEN,
    PLAYER_AP_LETTER_ZELDA,
    PLAYER_AP_MASK_KEATON,
    PLAYER_AP_MASK_SKULL,
    PLAYER_AP_MASK_SPOOKY,
    PLAYER_AP_MASK_BUNNY,
    PLAYER_AP_MASK_GORON,
    PLAYER_AP_MASK_ZORA,
    PLAYER_AP_MASK_GERUDO,
    PLAYER_AP_MASK_TRUTH,
    PLAYER_AP_SWORD_MASTER,
    PLAYER_AP_POCKET_EGG,
    PLAYER_AP_POCKET_CUCCO,
    PLAYER_AP_COJIRO,
    PLAYER_AP_ODD_MUSHROOM,
    PLAYER_AP_ODD_POTION,
    PLAYER_AP_SAW,
    PLAYER_AP_SWORD_BROKEN,
    PLAYER_AP_PRESCRIPTION,
    PLAYER_AP_FROG,
    PLAYER_AP_EYEDROPS,
    PLAYER_AP_CLAIM_CHECK,
    PLAYER_AP_BOW_FIRE,
    PLAYER_AP_BOW_ICE,
    PLAYER_AP_BOW_LIGHT,
    PLAYER_AP_SWORD_KOKIRI,
    PLAYER_AP_SWORD_MASTER,
    PLAYER_AP_SWORD_BGS,
};

static s32 (*sUpperBodyItemFuncs[])(Player* this, PlayState* play) = {
    [PLAYER_AP_NONE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_LAST_USED] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_FISHING_POLE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_SWORD_MASTER] = Player_SetupStartZTargetDefend2,
    [PLAYER_AP_SWORD_KOKIRI] = Player_SetupStartZTargetDefend2,
    [PLAYER_AP_SWORD_BGS] = Player_SetupStartZTargetDefend2,
    [PLAYER_AP_STICK] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_HAMMER] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOW] = Player_HoldFpsItem,
    [PLAYER_AP_BOW_FIRE] = Player_HoldFpsItem,
    [PLAYER_AP_BOW_ICE] = Player_HoldFpsItem,
    [PLAYER_AP_BOW_LIGHT] = Player_HoldFpsItem,
    [PLAYER_AP_BOW_0C] = Player_HoldFpsItem,
    [PLAYER_AP_BOW_0D] = Player_HoldFpsItem,
    [PLAYER_AP_BOW_0E] = Player_HoldFpsItem,
    [PLAYER_AP_SLINGSHOT] = Player_HoldFpsItem,
    [PLAYER_AP_HOOKSHOT] = Player_HoldFpsItem,
    [PLAYER_AP_LONGSHOT] = Player_HoldFpsItem,
    [PLAYER_AP_BOMB] = Player_HoldActor,
    [PLAYER_AP_BOMBCHU] = Player_HoldActor,
    [PLAYER_AP_BOOMERANG] = Player_HoldBoomerang,
    [PLAYER_AP_MAGIC_SPELL_15] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MAGIC_SPELL_16] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MAGIC_SPELL_17] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_FARORES_WIND] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_NAYRUS_LOVE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_DINS_FIRE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_NUT] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_OCARINA_FAIRY] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_OCARINA_TIME] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_FISH] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_FIRE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_BUG] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_POE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_BIG_POE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_LETTER] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_POTION_RED] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_POTION_BLUE] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_POTION_GREEN] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_MILK] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_MILK_HALF] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BOTTLE_FAIRY] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_LETTER_ZELDA] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_WEIRD_EGG] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_CHICKEN] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_BEAN] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_POCKET_EGG] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_POCKET_CUCCO] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_COJIRO] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_ODD_MUSHROOM] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_ODD_POTION] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_SAW] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_SWORD_BROKEN] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_PRESCRIPTION] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_FROG] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_EYEDROPS] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_CLAIM_CHECK] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_KEATON] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_SKULL] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_SPOOKY] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_BUNNY] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_GORON] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_ZORA] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_GERUDO] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_MASK_TRUTH] = Player_SetupStartZTargetDefend,
    [PLAYER_AP_LENS] = Player_SetupStartZTargetDefend,
};

static void (*sItemChangeFuncs[])(PlayState* play, Player* this) = {
    [PLAYER_AP_NONE] = Player_DoNothing,
    [PLAYER_AP_LAST_USED] = Player_DoNothing,
    [PLAYER_AP_FISHING_POLE] = Player_DoNothing,
    [PLAYER_AP_SWORD_MASTER] = Player_DoNothing,
    [PLAYER_AP_SWORD_KOKIRI] = Player_DoNothing,
    [PLAYER_AP_SWORD_BGS] = Player_DoNothing,
    [PLAYER_AP_STICK] = Player_SetupDekuStick,
    [PLAYER_AP_HAMMER] = Player_DoNothing2,
    [PLAYER_AP_BOW] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_BOW_FIRE] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_BOW_ICE] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_BOW_LIGHT] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_BOW_0C] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_BOW_0D] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_BOW_0E] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_SLINGSHOT] = Player_SetupBowOrSlingshot,
    [PLAYER_AP_HOOKSHOT] = Player_SetupHookshot,
    [PLAYER_AP_LONGSHOT] = Player_SetupHookshot,
    [PLAYER_AP_BOMB] = Player_SetupExplosive,
    [PLAYER_AP_BOMBCHU] = Player_SetupExplosive,
    [PLAYER_AP_BOOMERANG] = Player_SetupBoomerang,
    [PLAYER_AP_MAGIC_SPELL_15] = Player_DoNothing,
    [PLAYER_AP_MAGIC_SPELL_16] = Player_DoNothing,
    [PLAYER_AP_MAGIC_SPELL_17] = Player_DoNothing,
    [PLAYER_AP_FARORES_WIND] = Player_DoNothing,
    [PLAYER_AP_NAYRUS_LOVE] = Player_DoNothing,
    [PLAYER_AP_DINS_FIRE] = Player_DoNothing,
    [PLAYER_AP_NUT] = Player_DoNothing,
    [PLAYER_AP_OCARINA_FAIRY] = Player_DoNothing,
    [PLAYER_AP_OCARINA_TIME] = Player_DoNothing,
    [PLAYER_AP_BOTTLE] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_FISH] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_FIRE] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_BUG] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_POE] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_BIG_POE] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_LETTER] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_POTION_RED] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_POTION_BLUE] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_POTION_GREEN] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_MILK] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_MILK_HALF] = Player_DoNothing,
    [PLAYER_AP_BOTTLE_FAIRY] = Player_DoNothing,
    [PLAYER_AP_LETTER_ZELDA] = Player_DoNothing,
    [PLAYER_AP_WEIRD_EGG] = Player_DoNothing,
    [PLAYER_AP_CHICKEN] = Player_DoNothing,
    [PLAYER_AP_BEAN] = Player_DoNothing,
    [PLAYER_AP_POCKET_EGG] = Player_DoNothing,
    [PLAYER_AP_POCKET_CUCCO] = Player_DoNothing,
    [PLAYER_AP_COJIRO] = Player_DoNothing,
    [PLAYER_AP_ODD_MUSHROOM] = Player_DoNothing,
    [PLAYER_AP_ODD_POTION] = Player_DoNothing,
    [PLAYER_AP_SAW] = Player_DoNothing,
    [PLAYER_AP_SWORD_BROKEN] = Player_DoNothing,
    [PLAYER_AP_PRESCRIPTION] = Player_DoNothing,
    [PLAYER_AP_FROG] = Player_DoNothing,
    [PLAYER_AP_EYEDROPS] = Player_DoNothing,
    [PLAYER_AP_CLAIM_CHECK] = Player_DoNothing,
    [PLAYER_AP_MASK_KEATON] = Player_DoNothing,
    [PLAYER_AP_MASK_SKULL] = Player_DoNothing,
    [PLAYER_AP_MASK_SPOOKY] = Player_DoNothing,
    [PLAYER_AP_MASK_BUNNY] = Player_DoNothing,
    [PLAYER_AP_MASK_GORON] = Player_DoNothing,
    [PLAYER_AP_MASK_ZORA] = Player_DoNothing,
    [PLAYER_AP_MASK_GERUDO] = Player_DoNothing,
    [PLAYER_AP_MASK_TRUTH] = Player_DoNothing,
    [PLAYER_AP_LENS] = Player_DoNothing,
};

typedef enum {
    /*  0 */ PLAYER_ITEM_CHANGE_DEFAULT,
    /*  1 */ PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND,
    /*  2 */ PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND,
    /*  3 */ PLAYER_ITEM_CHANGE_SHIELD,
    /*  4 */ PLAYER_ITEM_CHANGE_2HAND_TO_1HAND,
    /*  5 */ PLAYER_ITEM_CHANGE_1HAND,
    /*  6 */ PLAYER_ITEM_CHANGE_2HAND,
    /*  7 */ PLAYER_ITEM_CHANGE_2HAND_TO_2HAND,
    /*  8 */ PLAYER_ITEM_CHANGE_DEFAULT_2,
    /*  9 */ PLAYER_ITEM_CHANGE_1HAND_TO_BOMB,
    /* 10 */ PLAYER_ITEM_CHANGE_2HAND_TO_BOMB,
    /* 11 */ PLAYER_ITEM_CHANGE_BOMB,
    /* 12 */ PLAYER_ITEM_CHANGE_UNK_12,
    /* 13 */ PLAYER_ITEM_CHANGE_LEFT_HAND,
    /* 14 */ PLAYER_ITEM_CHANGE_MAX
} PlayersItemChangeAnimsIndex;

static struct_808540F4 D_808540F4[PLAYER_ITEM_CHANGE_MAX] = {
    [PLAYER_ITEM_CHANGE_DEFAULT] =          { &gPlayerAnim_link_normal_free2free,      12 },
    [PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND] =  { &gPlayerAnim_link_normal_normal2fighter, 6  },
    [PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND] =  { &gPlayerAnim_link_hammer_normal2long,    8  },
    [PLAYER_ITEM_CHANGE_SHIELD] =           { &gPlayerAnim_link_normal_normal2free,    8  },
    [PLAYER_ITEM_CHANGE_2HAND_TO_1HAND] =   { &gPlayerAnim_link_fighter_fighter2long,  8  },
    [PLAYER_ITEM_CHANGE_1HAND] =            { &gPlayerAnim_link_normal_fighter2free,   10 },
    [PLAYER_ITEM_CHANGE_2HAND] =            { &gPlayerAnim_link_hammer_long2free,      7  },
    [PLAYER_ITEM_CHANGE_2HAND_TO_2HAND] =   { &gPlayerAnim_link_hammer_long2long,      11 },
    [PLAYER_ITEM_CHANGE_DEFAULT_2] =        { &gPlayerAnim_link_normal_free2free,      12 },
    [PLAYER_ITEM_CHANGE_1HAND_TO_BOMB] =    { &gPlayerAnim_link_normal_normal2bom,     4  },
    [PLAYER_ITEM_CHANGE_2HAND_TO_BOMB] =    { &gPlayerAnim_link_normal_long2bom,       4  },
    [PLAYER_ITEM_CHANGE_BOMB] =             { &gPlayerAnim_link_normal_free2bom,       4  },
    [PLAYER_ITEM_CHANGE_UNK_12] =           { &gPlayerAnim_link_anchor_anchor2fighter, 5  },
    [PLAYER_ITEM_CHANGE_LEFT_HAND] =        { &gPlayerAnim_link_normal_free2freeB,     13 },
};

static s8 sAnimTypeSwapTable[PLAYER_ANIMTYPE_MAX][PLAYER_ANIMTYPE_MAX] = {
    {
        PLAYER_ITEM_CHANGE_DEFAULT_2,
        -PLAYER_ITEM_CHANGE_1HAND,
        -PLAYER_ITEM_CHANGE_SHIELD,
        -PLAYER_ITEM_CHANGE_2HAND,
        PLAYER_ITEM_CHANGE_DEFAULT_2,
        PLAYER_ITEM_CHANGE_BOMB
    },
    {
        PLAYER_ITEM_CHANGE_1HAND,
        PLAYER_ITEM_CHANGE_DEFAULT,
        -PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND,
        PLAYER_ITEM_CHANGE_2HAND_TO_1HAND,
        PLAYER_ITEM_CHANGE_1HAND,
        PLAYER_ITEM_CHANGE_1HAND_TO_BOMB
    },
    {
        PLAYER_ITEM_CHANGE_SHIELD,
        PLAYER_ITEM_CHANGE_SHIELD_TO_1HAND,
        PLAYER_ITEM_CHANGE_DEFAULT,
        PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND,
        PLAYER_ITEM_CHANGE_SHIELD,
        PLAYER_ITEM_CHANGE_1HAND_TO_BOMB
    },
    {
        PLAYER_ITEM_CHANGE_2HAND,
        -PLAYER_ITEM_CHANGE_2HAND_TO_1HAND,
        -PLAYER_ITEM_CHANGE_SHIELD_TO_2HAND,
        PLAYER_ITEM_CHANGE_2HAND_TO_2HAND,
        PLAYER_ITEM_CHANGE_2HAND,
        PLAYER_ITEM_CHANGE_2HAND_TO_BOMB
    },
    {
        PLAYER_ITEM_CHANGE_DEFAULT_2,
        -PLAYER_ITEM_CHANGE_1HAND,
        -PLAYER_ITEM_CHANGE_SHIELD,
        -PLAYER_ITEM_CHANGE_2HAND,
        PLAYER_ITEM_CHANGE_DEFAULT_2,
        PLAYER_ITEM_CHANGE_BOMB
    },
    {
        PLAYER_ITEM_CHANGE_DEFAULT_2,
        -PLAYER_ITEM_CHANGE_1HAND,
        -PLAYER_ITEM_CHANGE_SHIELD,
        -PLAYER_ITEM_CHANGE_2HAND,
        PLAYER_ITEM_CHANGE_DEFAULT_2,
        PLAYER_ITEM_CHANGE_BOMB
    },
};

static ExplosiveInfo sExplosiveInfos[] = {
    { ITEM_BOMB,    ACTOR_EN_BOM     },
    { ITEM_BOMBCHU, ACTOR_EN_BOM_CHU },
};

static struct_80854190 sAnims_MeleeAttack[PLAYER_MWA_MAX] = {
    [PLAYER_MWA_FORWARD_SLASH_1H] = { &gPlayerAnim_link_fighter_normal_kiru,          &gPlayerAnim_link_fighter_normal_kiru_end,        &gPlayerAnim_link_fighter_normal_kiru_endR,       1, 4  },
    [PLAYER_MWA_FORWARD_SLASH_2H] = { &gPlayerAnim_link_fighter_Lnormal_kiru,         &gPlayerAnim_link_fighter_Lnormal_kiru_end,       &gPlayerAnim_link_anchor_Lnormal_kiru_endR,       1, 4  },
    [PLAYER_MWA_FORWARD_COMBO_1H] = { &gPlayerAnim_link_fighter_normal_kiru_finsh,    &gPlayerAnim_link_fighter_normal_kiru_finsh_end,  &gPlayerAnim_link_anchor_normal_kiru_finsh_endR,  0, 5  },
    [PLAYER_MWA_FORWARD_COMBO_2H] = { &gPlayerAnim_link_fighter_Lnormal_kiru_finsh,   &gPlayerAnim_link_fighter_Lnormal_kiru_finsh_end, &gPlayerAnim_link_anchor_Lnormal_kiru_finsh_endR, 1, 7  },
    [PLAYER_MWA_RIGHT_SLASH_1H] =   { &gPlayerAnim_link_fighter_Lside_kiru,           &gPlayerAnim_link_fighter_Lside_kiru_end,         &gPlayerAnim_link_anchor_Lside_kiru_endR,         1, 4  },
    [PLAYER_MWA_RIGHT_SLASH_2H] =   { &gPlayerAnim_link_fighter_LLside_kiru,          &gPlayerAnim_link_fighter_LLside_kiru_end,        &gPlayerAnim_link_anchor_LLside_kiru_endL,        0, 5  },
    [PLAYER_MWA_RIGHT_COMBO_1H] =   { &gPlayerAnim_link_fighter_Lside_kiru_finsh,     &gPlayerAnim_link_fighter_Lside_kiru_finsh_end,   &gPlayerAnim_link_anchor_Lside_kiru_finsh_endR,   2, 8  },
    [PLAYER_MWA_RIGHT_COMBO_2H] =   { &gPlayerAnim_link_fighter_LLside_kiru_finsh,    &gPlayerAnim_link_fighter_LLside_kiru_finsh_end,  &gPlayerAnim_link_anchor_LLside_kiru_finsh_endR,  3, 8  },
    [PLAYER_MWA_LEFT_SLASH_1H] =    { &gPlayerAnim_link_fighter_Rside_kiru,           &gPlayerAnim_link_fighter_Rside_kiru_end,         &gPlayerAnim_link_anchor_Rside_kiru_endR,         0, 4  },
    [PLAYER_MWA_LEFT_SLASH_2H] =    { &gPlayerAnim_link_fighter_LRside_kiru,          &gPlayerAnim_link_fighter_LRside_kiru_end,        &gPlayerAnim_link_anchor_LRside_kiru_endR,        0, 5  },
    [PLAYER_MWA_LEFT_COMBO_1H] =    { &gPlayerAnim_link_fighter_Rside_kiru_finsh,     &gPlayerAnim_link_fighter_Rside_kiru_finsh_end,   &gPlayerAnim_link_anchor_Rside_kiru_finsh_endR,   0, 6  },
    [PLAYER_MWA_LEFT_COMBO_2H] =    { &gPlayerAnim_link_fighter_LRside_kiru_finsh,    &gPlayerAnim_link_fighter_LRside_kiru_finsh_end,  &gPlayerAnim_link_anchor_LRside_kiru_finsh_endL,  1, 5  },
    [PLAYER_MWA_STAB_1H] =          { &gPlayerAnim_link_fighter_pierce_kiru,          &gPlayerAnim_link_fighter_pierce_kiru_end,        &gPlayerAnim_link_anchor_pierce_kiru_endR,        0, 3  },
    [PLAYER_MWA_STAB_2H] =          { &gPlayerAnim_link_fighter_Lpierce_kiru,         &gPlayerAnim_link_fighter_Lpierce_kiru_end,       &gPlayerAnim_link_anchor_Lpierce_kiru_endL,       0, 3  },
    [PLAYER_MWA_STAB_COMBO_1H] =    { &gPlayerAnim_link_fighter_pierce_kiru_finsh,    &gPlayerAnim_link_fighter_pierce_kiru_finsh_end,  &gPlayerAnim_link_anchor_pierce_kiru_finsh_endR,  1, 9  },
    [PLAYER_MWA_STAB_COMBO_2H] =    { &gPlayerAnim_link_fighter_Lpierce_kiru_finsh,   &gPlayerAnim_link_fighter_Lpierce_kiru_finsh_end, &gPlayerAnim_link_anchor_Lpierce_kiru_finsh_endR, 1, 8  },
    [PLAYER_MWA_FLIPSLASH_START] =  { &gPlayerAnim_link_fighter_jump_rollkiru,        &gPlayerAnim_link_fighter_jump_kiru_finsh,        &gPlayerAnim_link_fighter_jump_kiru_finsh,        1, 10 },
    [PLAYER_MWA_JUMPSLASH_START] =  { &gPlayerAnim_link_fighter_Lpower_jump_kiru,     &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit,   &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit,   1, 11 },
    [PLAYER_MWA_FLIPSLASH_FINISH] = { &gPlayerAnim_link_fighter_jump_kiru_finsh,      &gPlayerAnim_link_fighter_jump_kiru_finsh_end,    &gPlayerAnim_link_fighter_jump_kiru_finsh_end,    1, 2  },
    [PLAYER_MWA_JUMPSLASH_FINISH] = { &gPlayerAnim_link_fighter_Lpower_jump_kiru_hit, &gPlayerAnim_link_fighter_Lpower_jump_kiru_end,   &gPlayerAnim_link_fighter_Lpower_jump_kiru_end,   1, 2  },
    [PLAYER_MWA_BACKSLASH_RIGHT] =  { &gPlayerAnim_link_fighter_turn_kiruR,           &gPlayerAnim_link_fighter_turn_kiruR_end,         &gPlayerAnim_link_fighter_turn_kiruR_end,         1, 5  },
    [PLAYER_MWA_BACKSLASH_LEFT] =   { &gPlayerAnim_link_fighter_turn_kiruL,           &gPlayerAnim_link_fighter_turn_kiruL_end,         &gPlayerAnim_link_fighter_turn_kiruL_end,         1, 4  },
    [PLAYER_MWA_HAMMER_FORWARD] =   { &gPlayerAnim_link_hammer_hit,                   &gPlayerAnim_link_hammer_hit_end,                 &gPlayerAnim_link_hammer_hit_endR,                3, 10 },
    [PLAYER_MWA_HAMMER_SIDE] =      { &gPlayerAnim_link_hammer_side_hit,              &gPlayerAnim_link_hammer_side_hit_end,            &gPlayerAnim_link_hammer_side_hit_endR,           2, 11 },
    [PLAYER_MWA_SPIN_ATTACK_1H] =   { &gPlayerAnim_link_fighter_rolling_kiru,         &gPlayerAnim_link_fighter_rolling_kiru_end,       &gPlayerAnim_link_anchor_rolling_kiru_endR,       0, 12 },
    [PLAYER_MWA_SPIN_ATTACK_2H] =   { &gPlayerAnim_link_fighter_Lrolling_kiru,        &gPlayerAnim_link_fighter_Lrolling_kiru_end,      &gPlayerAnim_link_anchor_Lrolling_kiru_endR,      0, 15 },
    [PLAYER_MWA_BIG_SPIN_1H] =      { &gPlayerAnim_link_fighter_Wrolling_kiru,        &gPlayerAnim_link_fighter_Wrolling_kiru_end,      &gPlayerAnim_link_anchor_rolling_kiru_endR,       0, 16 },
    [PLAYER_MWA_BIG_SPIN_2H] =      { &gPlayerAnim_link_fighter_Wrolling_kiru,        &gPlayerAnim_link_fighter_Wrolling_kiru_end,      &gPlayerAnim_link_anchor_Lrolling_kiru_endR,      0, 16 },
};

static LinkAnimationHeader* sAnims_SpinAttack2[] = {
    &gPlayerAnim_link_fighter_power_kiru_start,
    &gPlayerAnim_link_fighter_Lpower_kiru_start,
};

static LinkAnimationHeader* sAnims_SpinAttack1[] = {
    &gPlayerAnim_link_fighter_power_kiru_startL,
    &gPlayerAnim_link_fighter_Lpower_kiru_start,
};

static LinkAnimationHeader* sAnims_SpinAttackCharge[] = {
    &gPlayerAnim_link_fighter_power_kiru_wait,
    &gPlayerAnim_link_fighter_Lpower_kiru_wait,
};

static LinkAnimationHeader* sAnims_SpinAttackCancel[] = {
    &gPlayerAnim_link_fighter_power_kiru_wait_end,
    &gPlayerAnim_link_fighter_Lpower_kiru_wait_end,
};

static LinkAnimationHeader* sAnims_SpinAttackWalk[] = {
    &gPlayerAnim_link_fighter_power_kiru_walk,
    &gPlayerAnim_link_fighter_Lpower_kiru_walk,
};

static LinkAnimationHeader* sAnims_SpinAttackSidewalk[] = {
    &gPlayerAnim_link_fighter_power_kiru_side_walk,
    &gPlayerAnim_link_fighter_Lpower_kiru_side_walk,
};

static u8 sAnimMwaFlag_SmallSpin[2] = { PLAYER_MWA_SPIN_ATTACK_1H, PLAYER_MWA_SPIN_ATTACK_2H };
static u8 sAnimMwaFlag_BigSpin[2] = { PLAYER_MWA_BIG_SPIN_1H, PLAYER_MWA_BIG_SPIN_2H };

static u16 sItemButtonID[] = { BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };

static u8 sMagicSpellCosts[] = { 12, 24, 24, 12, 24, 12 };

static u16 sSfxID_FpsItemStandby[] = { NA_SE_IT_BOW_DRAW, NA_SE_IT_SLING_DRAW, NA_SE_IT_HOOKSHOT_READY };

static u8 sMagicArrowCosts[] = { 4, 4, 8 };

static LinkAnimationHeader* sAnims_DefendStanceRight[] = {
    &gPlayerAnim_link_anchor_waitR2defense,
    &gPlayerAnim_link_anchor_waitR2defense_long,
};

static LinkAnimationHeader* sAnims_DefendStanceLeft[] = {
    &gPlayerAnim_link_anchor_waitL2defense,
    &gPlayerAnim_link_anchor_waitL2defense_long,
};

static LinkAnimationHeader* sAnims_DefendStanceLeft_Deflect[] = {
    &gPlayerAnim_link_anchor_defense_hit,
    &gPlayerAnim_link_anchor_defense_long_hitL,
};

static LinkAnimationHeader* sAnims_DefendStanceRight_Deflect[] = {
    &gPlayerAnim_link_anchor_defense_hit,
    &gPlayerAnim_link_anchor_defense_long_hitR,
};

static LinkAnimationHeader* sAnims_Deflect[] = {
    &gPlayerAnim_link_normal_defense_hit,
    &gPlayerAnim_link_fighter_defense_long_hit,
};

static LinkAnimationHeader* sAnims_FpsItemStandy_Walking[] = {
    &gPlayerAnim_link_bow_walk2ready,
    &gPlayerAnim_link_hook_walk2ready,
};

static LinkAnimationHeader* sAnims_FpsItemStandy[] = {
    &gPlayerAnim_link_bow_bow_wait,
    &gPlayerAnim_link_hook_wait,
};

// return type can't be void due to regalloc in Player_DebugMode
void Player_StopMovement(Player* this) {
    this->actor.speedXZ = 0.0f;
    this->linearVelocity = 0.0f;
}

// return type can't be void due to regalloc in Player_StartGrabPushPullWall
void Player_ClearAttentionModeAndStopMoving(Player* this) {
    Player_StopMovement(this);
    this->unk_6AD = 0;
}

s32 Player_CheckActorTalkRequested(PlayState* play) {
    Player* this = GET_PLAYER(play);
    
    return CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_8);
}

void Player_PlayAnimOnce(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayOnce(play, &this->skelAnime, anim);
}

void Player_PlayAnimLoop(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayLoop(play, &this->skelAnime, anim);
}

void Player_PlayAnimLoopSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, anim, 2.0f / 3.0f);
}

void Player_PlayAnimOnceSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, 2.0f / 3.0f);
}

void Player_AddRootYawToShapeYaw(Player* this) {
    this->actor.shape.rot.y += this->skelAnime.jointTable[1].y;
    this->skelAnime.jointTable[1].y = 0;
}

void Player_InactivateMeleeWeapon(Player* this) {
    this->stateFlags2 &= ~PLAYER_STATE2_17;
    this->meleeWeaponState = 0;
    this->meleeWeaponInfo[0].active = this->meleeWeaponInfo[1].active = this->meleeWeaponInfo[2].active = 0;
}

void Player_ResetSubCam(PlayState* play, Player* this) {
    Camera* subCam;
    
    if (this->subCamId != CAM_ID_NONE) {
        subCam = play->cameraPtrs[this->subCamId];
        if ((subCam != NULL) && (subCam->csId == 1100)) {
            OnePointCutscene_EndCutscene(play, this->subCamId);
            this->subCamId = CAM_ID_NONE;
        }
    }
    
    this->stateFlags2 &= ~(PLAYER_STATE2_10 | PLAYER_STATE2_11);
}

void Player_DetatchHeldActor(PlayState* play, Player* this) {
    Actor* heldActor = this->heldActor;
    
    if ((heldActor != NULL) && !Player_HoldsHookshot(this)) {
        this->actor.child = NULL;
        this->heldActor = NULL;
        this->interactRangeActor = NULL;
        heldActor->parent = NULL;
        this->stateFlags1 &= ~PLAYER_STATE1_11;
    }
    
    if (Player_GetExplosiveHeld(this) >= 0) {
        Player_ChangeItem(play, this, PLAYER_AP_NONE);
        this->heldItemId = ITEM_NONE_FE;
    }
}

void Player_ResetAttributes(PlayState* play, Player* this) {
    if ((this->stateFlags1 & PLAYER_STATE1_11) && (this->heldActor == NULL)) {
        if (this->interactRangeActor != NULL) {
            if (this->getItemId == GI_NONE) {
                this->stateFlags1 &= ~PLAYER_STATE1_11;
                this->interactRangeActor = NULL;
            }
        } else {
            this->stateFlags1 &= ~PLAYER_STATE1_11;
        }
    }
    
    Player_InactivateMeleeWeapon(this);
    this->unk_6AD = 0;
    
    Player_ResetSubCam(play, this);
    func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    
    this->stateFlags1 &= ~(PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_20 | PLAYER_STATE1_21);
    this->stateFlags2 &= ~(PLAYER_STATE2_4 | PLAYER_STATE2_7 | PLAYER_STATE2_18);
    
    this->actor.shape.rot.x = 0;
    this->actor.shape.yOffset = 0.0f;
    
    this->unk_845 = this->unk_844 = 0;
}

s32 Player_UnequipItem(PlayState* play, Player* this) {
    if (this->heldItemActionParam >= PLAYER_AP_FISHING_POLE) {
        Player_UseItem(play, this, ITEM_NONE);
        
        return 1;
    } else {
        return 0;
    }
}

void Player_ResetAttributesAndHeldActor(PlayState* play, Player* this) {
    Player_ResetAttributes(play, this);
    Player_DetatchHeldActor(play, this);
}

s32 Player_CanBreakFree(Player* this, s32 arg1, s32 arg2) {
    s16 temp = this->unk_A80 - sAnalogStickYaw;
    
    this->unk_850 += arg1 + (s16)(ABS(temp) * fabsf(sAnalogStickMod) * 2.5415802156203426e-06f);
    
    if (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B)) {
        this->unk_850 += 5;
    }
    
    return this->unk_850 > arg2;
}

void Player_SetFreezeFlashTimer(PlayState* play) {
    if (play->actorCtx.freezeFlashTimer == 0) {
        play->actorCtx.freezeFlashTimer = 1;
    }
}

void Player_SetRumble(Player* this, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    if (this->actor.category == ACTORCAT_PLAYER) {
        Rumble_Request(arg4, arg1, arg2, arg3);
    }
}

void Player_PlayVoiceSfxForAge(Player* this, u16 sfxId) {
    if (this->actor.category == ACTORCAT_PLAYER) {
        func_8002F7DC(&this->actor, sfxId + this->ageProperties->unk_92);
    } else {
        func_800F4190(&this->actor.projectedPos, sfxId);
    }
}

void Player_StopInterruptableSfx(Player* this) {
    u16* entry = &sInterruptableSfx[0];
    s32 i;
    
    for (i = 0; i < 4; i++) {
        Audio_StopSfxById((u16)(*entry + this->ageProperties->unk_92));
        entry++;
    }
}

u16 Player_GetMoveSfx(Player* this, u16 sfxId) {
    return sfxId + this->unk_89E;
}

void Player_PlayMoveSfx(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, Player_GetMoveSfx(this, sfxId));
}

u16 Player_GetMoveSfxForAge(Player* this, u16 sfxId) {
    return sfxId + this->unk_89E + this->ageProperties->unk_94;
}

void Player_PlayMoveSfxForAge(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, Player_GetMoveSfxForAge(this, sfxId));
}

void Player_PlayWalkSfx(Player* this, f32 arg1) {
    s32 sfxId;
    
    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_WALK_HEAVYBOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_WALK_GROUND);
    }
    
    func_800F4010(&this->actor.projectedPos, sfxId, arg1);
}

void Player_PlayJumpSfx(Player* this) {
    s32 sfxId;
    
    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_JUMP_HEAVYBOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_JUMP);
    }
    
    func_8002F7DC(&this->actor, sfxId);
}

void Player_PlayLandingSfx(Player* this) {
    s32 sfxId;
    
    if (this->currentBoots == PLAYER_BOOTS_IRON) {
        sfxId = NA_SE_PL_LAND_HEAVYBOOTS;
    } else {
        sfxId = Player_GetMoveSfxForAge(this, NA_SE_PL_LAND);
    }
    
    func_8002F7DC(&this->actor, sfxId);
}

void Player_PlayReactableSfx(Player* this, u16 sfxId) {
    func_8002F7DC(&this->actor, sfxId);
    this->stateFlags2 |= PLAYER_STATE2_3;
}

void Player_PlayAnimSfx(Player* this, struct_80832924* entry) {
    s32 data;
    s32 flags;
    u32 cont;
    
    do {
        data = ABS(entry->field);
        flags = data & 0x7800;
        if (LinkAnimation_OnFrame(&this->skelAnime, ABS(data & 0x7FF))) {
            if (flags == 0x800) {
                func_8002F7DC(&this->actor, entry->sfxId);
            } else if (flags == 0x1000) {
                Player_PlayMoveSfx(this, entry->sfxId);
            } else if (flags == 0x1800) {
                Player_PlayMoveSfxForAge(this, entry->sfxId);
            } else if (flags == 0x2000) {
                Player_PlayVoiceSfxForAge(this, entry->sfxId);
            } else if (flags == 0x2800) {
                Player_PlayLandingSfx(this);
            } else if (flags == 0x3000) {
                Player_PlayWalkSfx(this, 6.0f);
            } else if (flags == 0x3800) {
                Player_PlayJumpSfx(this);
            } else if (flags == 0x4000) {
                Player_PlayWalkSfx(this, 0.0f);
            } else if (flags == 0x4800) {
                func_800F4010(&this->actor.projectedPos, this->ageProperties->unk_94 + NA_SE_PL_WALK_LADDER, 0.0f);
            }
        }
        cont = (entry->field >= 0);
        entry++;
    } while (cont);
}

void Player_ChangeAnimMorphToLastFrame(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE, -6.0f);
}

void Player_ChangeAnimSlowedMorphToLastFrame(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        anim,
        2.0f / 3.0f,
        0.0f,
        Animation_GetLastFrame(anim),
        ANIMMODE_ONCE,
        -6.0f
    );
}

void Player_ChangeAnimShortMorphLoop(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -6.0f);
}

void Player_ChangeAnimOnce(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, 0.0f);
}

void Player_ChangeAnimLongMorphLoop(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -16.0f);
}

s32 Player_LoopAnimContinuously(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, anim);
        
        return 1;
    } else {
        return 0;
    }
}

void Player_AnimUpdatePrevTranslRot(Player* this) {
    this->skelAnime.prevTransl = this->skelAnime.baseTransl;
    this->skelAnime.prevRot = this->actor.shape.rot.y;
}

void Player_AnimUpdatePrevTranslRotApplyAgeScale(Player* this) {
    Player_AnimUpdatePrevTranslRot(this);
    this->skelAnime.prevTransl.x *= this->ageProperties->unk_08;
    this->skelAnime.prevTransl.y *= this->ageProperties->unk_08;
    this->skelAnime.prevTransl.z *= this->ageProperties->unk_08;
}

void Player_ClearRootLimbRotY(Player* this) {
    this->skelAnime.jointTable[1].y = 0;
}

void Player_EndAnimMovement(Player* this) {
    if (this->skelAnime.moveFlags != 0) {
        Player_AddRootYawToShapeYaw(this);
        this->skelAnime.jointTable[0].x = this->skelAnime.baseTransl.x;
        this->skelAnime.jointTable[0].z = this->skelAnime.baseTransl.z;
        if (this->skelAnime.moveFlags & 8) {
            if (this->skelAnime.moveFlags & 2) {
                this->skelAnime.jointTable[0].y = this->skelAnime.prevTransl.y;
            }
        } else {
            this->skelAnime.jointTable[0].y = this->skelAnime.baseTransl.y;
        }
        Player_AnimUpdatePrevTranslRot(this);
        this->skelAnime.moveFlags = 0;
    }
}

void Player_UpdateAnimMovement(Player* this, s32 flags) {
    Vec3f pos;
    
    this->skelAnime.moveFlags = flags;
    this->skelAnime.prevTransl = this->skelAnime.baseTransl;
    SkelAnime_UpdateTranslation(&this->skelAnime, &pos, this->actor.shape.rot.y);
    
    if (flags & 1) {
        if (!LINK_IS_ADULT) {
            pos.x *= 0.64f;
            pos.z *= 0.64f;
        }
        this->actor.world.pos.x += pos.x * this->actor.scale.x;
        this->actor.world.pos.z += pos.z * this->actor.scale.z;
    }
    
    if (flags & 2) {
        if (!(flags & 4)) {
            pos.y *= this->ageProperties->unk_08;
        }
        this->actor.world.pos.y += pos.y * this->actor.scale.y;
    }
    
    Player_AddRootYawToShapeYaw(this);
}

void Player_SetupAnimMovement(PlayState* play, Player* this, s32 flags) {
    if (flags & 0x200) {
        Player_AnimUpdatePrevTranslRotApplyAgeScale(this);
    } else if ((flags & 0x100) || (this->skelAnime.moveFlags != 0)) {
        Player_AnimUpdatePrevTranslRot(this);
    } else {
        this->skelAnime.prevTransl = this->skelAnime.jointTable[0];
        this->skelAnime.prevRot = this->actor.shape.rot.y;
    }
    
    this->skelAnime.moveFlags = flags;
    Player_StopMovement(this);
    AnimationContext_DisableQueue(play);
}

void Player_PlayAnimOnceWithMovementSetSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags, f32 playbackSpeed) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, playbackSpeed);
    Player_SetupAnimMovement(play, this, flags);
}

void Player_PlayAnimOnceWithMovement(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimOnceWithMovementSetSpeed(play, this, anim, flags, 1.0f);
}

void Player_PlayAnimOnceWithMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimOnceWithMovementSetSpeed(play, this, anim, flags, 2.0f / 3.0f);
}

void Player_PlayAnimOnceWithMovementPresetFlagsSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, anim, 0x1C);
}

void Player_PlayAnimLoopWithMovementSetSpeed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags, f32 playbackSpeed) {
    LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, anim, playbackSpeed);
    Player_SetupAnimMovement(play, this, flags);
}

void Player_PlayAnimLoopWithMovement(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimLoopWithMovementSetSpeed(play, this, anim, flags, 1.0f);
}

void Player_PlayAnimLoopWithMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim, s32 flags) {
    Player_PlayAnimLoopWithMovementSetSpeed(play, this, anim, flags, 2.0f / 3.0f);
}

void Player_PlayAnimLoopWithMovementPresetFlagsSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_PlayAnimLoopWithMovementSlowed(play, this, anim, 0x1C);
}

void Player_UpdateAnalogInput(PlayState* play, Player* this) {
    s8 phi_v1;
    s8 phi_v0;
    
    this->unk_A7C = sAnalogStickMod;
    this->unk_A80 = sAnalogStickYaw;
    
    func_80077D10(&sAnalogStickMod, &sAnalogStickYaw, sControlInput);
    
    sAnalogStickYawCamOffset = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)) + sAnalogStickYaw;
    
    this->unk_846 = (this->unk_846 + 1) % 4;
    
    if (sAnalogStickMod < 55.0f) {
        phi_v0 = -1;
        phi_v1 = -1;
    } else {
        phi_v1 = (u16)(sAnalogStickYaw + 0x2000) >> 9;
        phi_v0 = (u16)((s16)(sAnalogStickYawCamOffset - this->actor.shape.rot.y) + 0x2000) >> 14;
    }
    
    this->unk_847[this->unk_846] = phi_v1;
    this->unk_84B[this->unk_846] = phi_v0;
}

void Player_PlayAnimOnce_WaterSpeedScale(PlayState* play, Player* this, LinkAnimationHeader* linkAnim) {
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, linkAnim, sWaterSpeedScale);
}

s32 Player_IsSwimming(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_27) && (this->currentBoots != PLAYER_BOOTS_IRON);
}

s32 Player_IsAiming_Boomerang(Player* this) {
    return (this->stateFlags1 & PLAYER_STATE1_24);
}

void Player_SetGetItemID(Player* this, PlayState* play) {
    GetItemEntry* giEntry = &sGetItemTable[this->getItemId - 1];
    
    this->unk_862 = ABS(giEntry->gi);
}

static LinkAnimationHeader* Player_GetAnim_StandingStill(Player* this) {
    return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_0, this->modelAnimType);
}

s32 Player_IsPlayingIdleAnim(Player* this) {
    LinkAnimationHeader** entry;
    s32 i;
    
    if (Player_GetAnim_StandingStill(this) != this->skelAnime.animation) {
        for (i = 0, entry = &sAnims_Idle[0][0]; i < 28; i++, entry++) {
            if (this->skelAnime.animation == *entry) {
                return i + 1;
            }
        }
        
        return 0;
    }
    
    return -1;
}

void Player_PlayIdleAnimSfx(Player* this, s32 arg1) {
    if (sAnimSfx_IndexOffsetTable[arg1] != 0) {
        Player_PlayAnimSfx(this, sAnimSfx_Idle[sAnimSfx_IndexOffsetTable[arg1] - 1]);
    }
}

LinkAnimationHeader* Player_GetAnim_Running(Player* this) {
    if (this->unk_890 != 0) {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_3, this->modelAnimType);
    } else if (
        !(this->stateFlags1 & (PLAYER_STATE1_27 | PLAYER_STATE1_29)) &&
        (this->currentBoots == PLAYER_BOOTS_IRON)
    ) {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_4, this->modelAnimType);
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_2, this->modelAnimType);
    }
}

s32 Player_IsAimingReady_Boomerang(Player* this) {
    return Player_IsAiming_Boomerang(this) && (this->unk_834 != 0);
}

LinkAnimationHeader* Player_GetAnim_FightRight(Player* this) {
    if (Player_IsAimingReady_Boomerang(this)) {
        return &gPlayerAnim_link_boom_throw_waitR;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_6, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetAnim_FightLeft(Player* this) {
    if (Player_IsAimingReady_Boomerang(this)) {
        return &gPlayerAnim_link_boom_throw_waitL;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_5, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetAnim_SidewalkEnd(Player* this) {
    if (func_8002DD78(this)) {
        return &gPlayerAnim_link_bow_side_walk;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_23, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetAnim_SidewalkRight(Player* this) {
    if (Player_IsAimingReady_Boomerang(this)) {
        return &gPlayerAnim_link_boom_throw_side_walkR;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_25, this->modelAnimType);
    }
}

LinkAnimationHeader* Player_GetAnim_SidewalkLeft(Player* this) {
    if (Player_IsAimingReady_Boomerang(this)) {
        return &gPlayerAnim_link_boom_throw_side_walkL;
    } else {
        return GET_PLAYER_ANIM(PLAYER_ANIMGROUP_24, this->modelAnimType);
    }
}

void Player_SetUpperActionFunc(Player* this, PlayerUpperActionFunc arg1) {
    this->func_82C = arg1;
    this->unk_836 = 0;
    this->unk_830 = 0.0f;
    Player_StopInterruptableSfx(this);
}

void Player_SetupChangeItemAnim(PlayState* play, Player* this, s8 actionParam) {
    LinkAnimationHeader* current = this->skelAnime.animation;
    LinkAnimationHeader** iter = sAnims_AnimTypeGroup + this->modelAnimType;
    u32 i;
    
    this->stateFlags1 &= ~(PLAYER_STATE1_3 | PLAYER_STATE1_24);
    
    for (i = 0; i < PLAYER_ANIMGROUP_MAX; i++) {
        if (current == *iter) {
            break;
        }
        iter += PLAYER_ANIMTYPE_MAX;
    }
    
    Player_ChangeItem(play, this, actionParam);
    
    if (i < PLAYER_ANIMGROUP_MAX) {
        this->skelAnime.animation = GET_PLAYER_ANIM(i, this->modelAnimType);
    }
}

s8 Player_ItemToActionParam(s32 item) {
    if (item >= ITEM_NONE_FE) {
        return PLAYER_AP_NONE;
    } else if (item == ITEM_LAST_USED) {
        return PLAYER_AP_LAST_USED;
    } else if (item == ITEM_FISHING_POLE) {
        return PLAYER_AP_FISHING_POLE;
    } else {
        return sItemActionParams[item];
    }
}

void Player_DoNothing(PlayState* play, Player* this) {
}

void Player_SetupDekuStick(PlayState* play, Player* this) {
    this->unk_85C = 1.0f;
}

void Player_DoNothing2(PlayState* play, Player* this) {
}

void Player_SetupBowOrSlingshot(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_3;
    
    if (this->heldItemActionParam != PLAYER_AP_SLINGSHOT) {
        this->unk_860 = -1;
    } else {
        this->unk_860 = -2;
    }
}

void Player_SetupExplosive(PlayState* play, Player* this) {
    s32 explosiveType;
    ExplosiveInfo* explosiveInfo;
    Actor* spawnedActor;
    
    if (this->stateFlags1 & PLAYER_STATE1_11) {
        Player_UnequipItem(play, this);
        
        return;
    }
    
    explosiveType = Player_GetExplosiveHeld(this);
    explosiveInfo = &sExplosiveInfos[explosiveType];
    
    spawnedActor =
        Actor_SpawnAsChild(
        &play->actorCtx,
        &this->actor,
        play,
        explosiveInfo->actorId,
        this->actor.world.pos.x,
        this->actor.world.pos.y,
        this->actor.world.pos.z,
        0,
        this->actor.shape.rot.y,
        0,
        0
        );
    if (spawnedActor != NULL) {
        if ((explosiveType != 0) && (play->bombchuBowlingStatus != 0)) {
            play->bombchuBowlingStatus--;
            if (play->bombchuBowlingStatus == 0) {
                play->bombchuBowlingStatus = -1;
            }
        } else {
            Inventory_ChangeAmmo(explosiveInfo->itemId, -1);
        }
        
        this->interactRangeActor = spawnedActor;
        this->heldActor = spawnedActor;
        this->getItemId = GI_NONE;
        this->unk_3BC.y = spawnedActor->shape.rot.y - this->actor.shape.rot.y;
        this->stateFlags1 |= PLAYER_STATE1_11;
    }
}

void Player_SetupHookshot(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_3;
    this->unk_860 = -3;
    
    this->heldActor =
        Actor_SpawnAsChild(
        &play->actorCtx,
        &this->actor,
        play,
        ACTOR_ARMS_HOOK,
        this->actor.world.pos.x,
        this->actor.world.pos.y,
        this->actor.world.pos.z,
        0,
        this->actor.shape.rot.y,
        0,
        0
        );
}

void Player_SetupBoomerang(PlayState* play, Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_24;
}

void Player_ChangeItem(PlayState* play, Player* this, s8 actionParam) {
    this->unk_860 = 0;
    this->unk_85C = 0.0f;
    this->unk_858 = 0.0f;
    
    this->heldItemActionParam = this->itemActionParam = actionParam;
    this->modelGroup = this->nextModelGroup;
    
    this->stateFlags1 &= ~(PLAYER_STATE1_3 | PLAYER_STATE1_24);
    
    sItemChangeFuncs[actionParam](play, this);
    
    Player_SetModelGroup(this, this->modelGroup);
}

void Player_MeleeAttack(Player* this, s32 newMeleeWeaponState) {
    u16 itemSfx;
    u16 voiceSfx;
    
    if (this->meleeWeaponState == 0) {
        if ((this->heldItemActionParam == PLAYER_AP_SWORD_BGS) && (gSaveContext.swordHealth > 0.0f)) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else {
            itemSfx = NA_SE_IT_SWORD_SWING;
        }
        
        voiceSfx = NA_SE_VO_LI_SWORD_N;
        if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
            itemSfx = NA_SE_IT_HAMMER_SWING;
        } else if (this->meleeWeaponAnimation >= PLAYER_MWA_SPIN_ATTACK_1H) {
            itemSfx = 0;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        } else if (this->unk_845 >= 3) {
            itemSfx = NA_SE_IT_SWORD_SWING_HARD;
            voiceSfx = NA_SE_VO_LI_SWORD_L;
        }
        
        if (itemSfx != 0) {
            Player_PlayReactableSfx(this, itemSfx);
        }
        
        if (
            !((this->meleeWeaponAnimation >= PLAYER_MWA_FLIPSLASH_START) &&
            (this->meleeWeaponAnimation <= PLAYER_MWA_JUMPSLASH_FINISH))
        ) {
            Player_PlayVoiceSfxForAge(this, voiceSfx);
        }
    }
    
    this->meleeWeaponState = newMeleeWeaponState;
}

s32 Player_IsFriendlyZTargeting(Player* this) {
    if (this->stateFlags1 & (PLAYER_STATE1_16 | PLAYER_STATE1_17 | PLAYER_STATE1_30)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_SetupStartEnemyZTargeting(Player* this) {
    if ((this->unk_664 != NULL) && CHECK_FLAG_ALL(this->unk_664->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
        this->stateFlags1 |= PLAYER_STATE1_4;
        
        return 1;
    }
    
    if (this->stateFlags1 & PLAYER_STATE1_4) {
        this->stateFlags1 &= ~PLAYER_STATE1_4;
        if (this->linearVelocity == 0.0f) {
            this->currentYaw = this->actor.shape.rot.y;
        }
    }
    
    return 0;
}

s32 Player_IsZTargeting(Player* this) {
    return func_8008E9C4(this) || Player_IsFriendlyZTargeting(this);
}

s32 Player_IsZTargetingSetupStartEnemy(Player* this) {
    return Player_SetupStartEnemyZTargeting(this) || Player_IsFriendlyZTargeting(this);
}

void Player_ResetLeftRightBlendWeight(Player* this) {
    this->unk_870 = this->unk_874 = 0.0f;
}

s32 Player_IsItemValid(Player* this, s32 item) {
    if ((item < ITEM_NONE_FE) && (Player_ItemToActionParam(item) == this->itemActionParam)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_IsWearableMaskValid(s32 item1, s32 actionParam) {
    if ((item1 < ITEM_NONE_FE) && (Player_ItemToActionParam(item1) == actionParam)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_GetButtonItem(PlayState* play, s32 index) {
    if (index >= 4) {
        return ITEM_NONE;
    } else if (play->bombchuBowlingStatus != 0) {
        return (play->bombchuBowlingStatus > 0) ? ITEM_BOMBCHU : ITEM_NONE;
    } else if (index == 0) {
        return B_BTN_ITEM;
    } else if (index == 1) {
        return C_BTN_ITEM(0);
    } else if (index == 2) {
        return C_BTN_ITEM(1);
    } else {
        return C_BTN_ITEM(2);
    }
}

void Player_SetupUseItem(Player* this, PlayState* play) {
    s32 maskActionParam;
    s32 item;
    s32 i;
    
    if (this->currentMask != PLAYER_MASK_NONE) {
        maskActionParam = this->currentMask - 1 + PLAYER_AP_MASK_KEATON;
        if (
            !Player_IsWearableMaskValid(C_BTN_ITEM(0), maskActionParam) && !Player_IsWearableMaskValid(C_BTN_ITEM(1), maskActionParam) &&
            !Player_IsWearableMaskValid(C_BTN_ITEM(2), maskActionParam)
        ) {
            this->currentMask = PLAYER_MASK_NONE;
        }
    }
    
    if (!(this->stateFlags1 & (PLAYER_STATE1_11 | PLAYER_STATE1_29)) && !func_8008F128(this)) {
        if (this->itemActionParam >= PLAYER_AP_FISHING_POLE) {
            if (
                !Player_IsItemValid(this, B_BTN_ITEM) && !Player_IsItemValid(this, C_BTN_ITEM(0)) &&
                !Player_IsItemValid(this, C_BTN_ITEM(1)) && !Player_IsItemValid(this, C_BTN_ITEM(2))
            ) {
                Player_UseItem(play, this, ITEM_NONE);
                
                return;
            }
        }
        
        for (i = 0; i < ARRAY_COUNT(sItemButtonID); i++) {
            if (CHECK_BTN_ALL(sControlInput->press.button, sItemButtonID[i])) {
                break;
            }
        }
        
        item = Player_GetButtonItem(play, i);
        if (item >= ITEM_NONE_FE) {
            for (i = 0; i < ARRAY_COUNT(sItemButtonID); i++) {
                if (CHECK_BTN_ALL(sControlInput->cur.button, sItemButtonID[i])) {
                    break;
                }
            }
            
            item = Player_GetButtonItem(play, i);
            if ((item < ITEM_NONE_FE) && (Player_ItemToActionParam(item) == this->heldItemActionParam)) {
                sActiveItemUseFlag2 = true;
            }
        } else {
            this->heldItemButton = i;
            Player_UseItem(play, this, item);
        }
    }
}

void Player_SetupStartChangeItem(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 phi_f2;
    f32 phi_f12;
    f32 phi_f14;
    f32 phi_f0;
    s32 sp38;
    s8 sp37;
    s32 nextAnimType;
    
    sp37 = Player_ItemToActionParam(this->heldItemId);
    Player_SetUpperActionFunc(this, Player_StartChangeItem);
    
    nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];
    sp38 = sAnimTypeSwapTable[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]][nextAnimType];
    if (
        (sp37 == PLAYER_AP_BOTTLE) || (sp37 == PLAYER_AP_BOOMERANG) ||
        ((sp37 == PLAYER_AP_NONE) &&
        ((this->heldItemActionParam == PLAYER_AP_BOTTLE) || (this->heldItemActionParam == PLAYER_AP_BOOMERANG)))
    ) {
        sp38 = (sp37 == PLAYER_AP_NONE) ? -PLAYER_ITEM_CHANGE_LEFT_HAND : PLAYER_ITEM_CHANGE_LEFT_HAND;
    }
    
    this->unk_15A = ABS(sp38);
    
    anim = D_808540F4[this->unk_15A].anim;
    if ((anim == &gPlayerAnim_link_normal_fighter2free) && (this->currentShield == PLAYER_SHIELD_NONE)) {
        anim = &gPlayerAnim_link_normal_free2fighter_free;
    }
    
    phi_f2 = Animation_GetLastFrame(anim);
    phi_f14 = phi_f2;
    
    if (sp38 >= 0) {
        phi_f0 = 1.2f;
        phi_f12 = 0.0f;
    } else {
        phi_f14 = 0.0f;
        phi_f0 = -1.2f;
        phi_f12 = phi_f2;
    }
    
    if (sp37 != PLAYER_AP_NONE) {
        phi_f0 *= 2.0f;
    }
    
    LinkAnimation_Change(play, &this->skelAnime2, anim, phi_f0, phi_f12, phi_f14, ANIMMODE_ONCE, 0.0f);
    
    this->stateFlags1 &= ~PLAYER_STATE1_8;
}

void Player_SetupItem(Player* this, PlayState* play) {
    if (
        (this->actor.category == ACTORCAT_PLAYER) && !(this->stateFlags1 & PLAYER_STATE1_8) &&
        ((this->heldItemActionParam == this->itemActionParam) || (this->stateFlags1 & PLAYER_STATE1_22)) &&
        (gSaveContext.health != 0) && (play->csCtx.state == CS_STATE_IDLE) && (this->csMode == 0) &&
        (play->shootingGalleryStatus == 0) && (play->activeCamId == CAM_ID_MAIN) &&
        (play->transitionTrigger != TRANS_TRIGGER_START) && (gSaveContext.timer1State != 10)
    ) {
        Player_SetupUseItem(this, play);
    }
    
    if (this->stateFlags1 & PLAYER_STATE1_8) {
        Player_SetupStartChangeItem(this, play);
    }
}

s32 Player_GetFpsItemAmmo(PlayState* play, Player* this, s32* itemPtr, s32* typePtr) {
    if (LINK_IS_ADULT) {
        *itemPtr = ITEM_BOW;
        if (this->stateFlags1 & PLAYER_STATE1_23) {
            *typePtr = ARROW_NORMAL_HORSE;
        } else {
            *typePtr = ARROW_NORMAL + (this->heldItemActionParam - PLAYER_AP_BOW);
        }
    } else {
        *itemPtr = ITEM_SLINGSHOT;
        *typePtr = ARROW_SEED;
    }
    
    if (gSaveContext.minigameState == 1) {
        return play->interfaceCtx.hbaAmmo;
    } else if (play->shootingGalleryStatus != 0) {
        return play->shootingGalleryStatus;
    } else {
        return AMMO(*itemPtr);
    }
}

s32 Player_SetupReadyFpsItemToShoot(Player* this, PlayState* play) {
    s32 item;
    s32 arrowType;
    s32 magicArrowType;
    
    if (
        (this->heldItemActionParam >= PLAYER_AP_BOW_FIRE) && (this->heldItemActionParam <= PLAYER_AP_BOW_0E) &&
        (gSaveContext.magicState != MAGIC_STATE_IDLE)
    ) {
        func_80078884(NA_SE_SY_ERROR);
    } else {
        Player_SetUpperActionFunc(this, Player_ReadyFpsItemToShoot);
        
        this->stateFlags1 |= PLAYER_STATE1_9;
        this->unk_834 = 14;
        
        if (this->unk_860 >= 0) {
            func_8002F7DC(&this->actor, sSfxID_FpsItemStandby[ABS(this->unk_860) - 1]);
            
            if (!Player_HoldsHookshot(this) && (Player_GetFpsItemAmmo(play, this, &item, &arrowType) > 0)) {
                magicArrowType = arrowType - ARROW_FIRE;
                
                if (this->unk_860 >= 0) {
                    if (
                        (magicArrowType >= 0) && (magicArrowType <= 2) &&
                        !Magic_RequestChange(play, sMagicArrowCosts[magicArrowType], MAGIC_CONSUME_NOW)
                    ) {
                        arrowType = ARROW_NORMAL;
                    }
                    
                    this->heldActor = Actor_SpawnAsChild(
                        &play->actorCtx,
                        &this->actor,
                        play,
                        ACTOR_EN_ARROW,
                        this->actor.world.pos.x,
                        this->actor.world.pos.y,
                        this->actor.world.pos.z,
                        0,
                        this->actor.shape.rot.y,
                        0,
                        arrowType
                    );
                }
            }
        }
        
        return 1;
    }
    
    return 0;
}

void Player_ChangeItemWithSfx(PlayState* play, Player* this) {
    if (this->heldItemActionParam != PLAYER_AP_NONE) {
        if (func_8008F2BC(this, this->heldItemActionParam) >= 0) {
            Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PUTAWAY);
        } else {
            Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
        }
    }
    
    Player_UseItem(play, this, this->heldItemId);
    
    if (func_8008F2BC(this, this->heldItemActionParam) >= 0) {
        Player_PlayReactableSfx(this, NA_SE_IT_SWORD_PICKOUT);
    } else if (this->heldItemActionParam != PLAYER_AP_NONE) {
        Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
    }
}

void Player_SetupHeldItemUpperActionFunc(PlayState* play, Player* this) {
    if (Player_StartChangeItem == this->func_82C) {
        Player_ChangeItemWithSfx(play, this);
    }
    
    Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemActionParam]);
    this->unk_834 = 0;
    this->unk_6AC = 0;
    Player_DetatchHeldActor(play, this);
    this->stateFlags1 &= ~PLAYER_STATE1_8;
}

LinkAnimationHeader* Player_GetAnim_Defend(PlayState* play, Player* this) {
    Player_SetUpperActionFunc(this, Player_StandingDefend);
    Player_DetatchHeldActor(play, this);
    
    if (this->unk_870 < 0.5f) {
        return sAnims_DefendStanceRight[Player_HoldsTwoHandedWeapon(this)];
    } else {
        return sAnims_DefendStanceLeft[Player_HoldsTwoHandedWeapon(this)];
    }
}

s32 Player_StartZTargetDefend(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    f32 frame;
    
    if (
        !(this->stateFlags1 & (PLAYER_STATE1_22 | PLAYER_STATE1_23 | PLAYER_STATE1_29)) &&
        (play->shootingGalleryStatus == 0) && (this->heldItemActionParam == this->itemActionParam) &&
        (this->currentShield != PLAYER_SHIELD_NONE) && !Player_IsChildWithHylianShield(this) && Player_IsZTargeting(this) &&
        CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)
    ) {
        anim = Player_GetAnim_Defend(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->skelAnime2, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
        func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_POSTURE);
        
        return 1;
    } else {
        return 0;
    }
}

s32 Player_SetupStartZTargetDefend(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    } else {
        return 0;
    }
}

void Player_SetupEndDefend(Player* this) {
    Player_SetUpperActionFunc(this, Player_EndDefend);
    
    if (this->itemActionParam < 0) {
        func_8008EC70(this);
    }
    
    Animation_Reverse(&this->skelAnime2);
    func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_REMOVE);
}

void Player_SetupChangeItem(PlayState* play, Player* this) {
    struct_808540F4* ptr = &D_808540F4[this->unk_15A];
    f32 temp;
    
    temp = ptr->unk_04;
    temp = (this->skelAnime2.playSpeed < 0.0f) ? temp - 1.0f : temp;
    
    if (LinkAnimation_OnFrame(&this->skelAnime2, temp)) {
        Player_ChangeItemWithSfx(play, this);
    }
    
    Player_SetupStartEnemyZTargeting(this);
}

s32 func_8083499C(Player* this, PlayState* play) {
    if (this->stateFlags1 & PLAYER_STATE1_8) {
        Player_SetupStartChangeItem(this, play);
    } else {
        return 0;
    }
    
    return 1;
}

s32 Player_SetupStartZTargetDefend2(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this) || func_8083499C(this, play)) {
        return 1;
    } else {
        return 0;
    }
}

s32 Player_StartChangeItem(Player* this, PlayState* play) {
    if (
        LinkAnimation_Update(play, &this->skelAnime2) ||
        ((Player_ItemToActionParam(this->heldItemId) == this->heldItemActionParam) &&
        (sActiveItemUseFlag =
        (sActiveItemUseFlag || ((this->modelAnimType != PLAYER_ANIMTYPE_3) && (play->shootingGalleryStatus == 0)))))
    ) {
        Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemActionParam]);
        this->unk_834 = 0;
        this->unk_6AC = 0;
        sActiveItemUseFlag2 = sActiveItemUseFlag;
        
        return this->func_82C(this, play);
    }
    
    if (Player_IsPlayingIdleAnim(this) != 0) {
        Player_SetupChangeItem(play, this);
        Player_PlayAnimOnce(play, this, Player_GetAnim_StandingStill(this));
        this->unk_6AC = 0;
    } else {
        Player_SetupChangeItem(play, this);
    }
    
    return 1;
}

s32 Player_StandingDefend(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime2);
    
    if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
        Player_SetupEndDefend(this);
        
        return 1;
    } else {
        this->stateFlags1 |= PLAYER_STATE1_22;
        Player_SetModelsForHoldingShield(this);
        
        return 1;
    }
}

s32 Player_EndDeflectAttackStanding(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;
    
    if (LinkAnimation_Update(play, &this->skelAnime2)) {
        anim = Player_GetAnim_Defend(play, this);
        frame = Animation_GetLastFrame(anim);
        LinkAnimation_Change(play, &this->skelAnime2, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
    }
    
    this->stateFlags1 |= PLAYER_STATE1_22;
    Player_SetModelsForHoldingShield(this);
    
    return 1;
}

s32 Player_EndDefend(Player* this, PlayState* play) {
    sActiveItemUseFlag = sActiveItemUseFlag2;
    
    if (sActiveItemUseFlag || LinkAnimation_Update(play, &this->skelAnime2)) {
        Player_SetUpperActionFunc(this, sUpperBodyItemFuncs[this->heldItemActionParam]);
        LinkAnimation_PlayLoop(play, &this->skelAnime2, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_0, this->modelAnimType));
        this->unk_6AC = 0;
        this->func_82C(this, play);
        
        return 0;
    }
    
    return 1;
}

s32 Player_SetupUseFpsItem(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    
    if (this->heldItemActionParam != PLAYER_AP_BOOMERANG) {
        if (!Player_SetupReadyFpsItemToShoot(this, play)) {
            return 0;
        }
        
        if (!Player_HoldsHookshot(this)) {
            anim = &gPlayerAnim_link_bow_bow_ready;
        } else {
            anim = &gPlayerAnim_link_hook_shot_ready;
        }
        LinkAnimation_PlayOnce(play, &this->skelAnime2, anim);
    } else {
        Player_SetUpperActionFunc(this, Player_SetupAimBoomerang);
        this->unk_834 = 10;
        LinkAnimation_PlayOnce(play, &this->skelAnime2, &gPlayerAnim_link_boom_throw_wait2waitR);
    }
    
    if (this->stateFlags1 & PLAYER_STATE1_23) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_uma_anim_walk);
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !Player_SetupStartEnemyZTargeting(this)) {
        Player_PlayAnimLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_0, this->modelAnimType));
    }
    
    return 1;
}

s32 Player_CheckShootingGalleryShootInput(PlayState* play) {
    return (play->shootingGalleryStatus > 0) && CHECK_BTN_ALL(sControlInput->press.button, BTN_B);
}

s32 func_80834E7C(PlayState* play) {
    return (play->shootingGalleryStatus != 0) &&
           ((play->shootingGalleryStatus < 0) ||
           CHECK_BTN_ANY(sControlInput->cur.button, BTN_A | BTN_B | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN));
}

s32 Player_SetupAimAttention(Player* this, PlayState* play) {
    if ((this->unk_6AD == 0) || (this->unk_6AD == 2)) {
        if (Player_IsZTargeting(this) || (Camera_CheckValidMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_BOWARROW) == 0)) {
            return 1;
        }
        this->unk_6AD = 2;
    }
    
    return 0;
}

s32 Player_CanUseFpsItem(Player* this, PlayState* play) {
    if ((this->doorType == PLAYER_DOORTYPE_NONE) && !(this->stateFlags1 & PLAYER_STATE1_25)) {
        if (sActiveItemUseFlag || Player_CheckShootingGalleryShootInput(play)) {
            if (Player_SetupUseFpsItem(this, play)) {
                return Player_SetupAimAttention(this, play);
            }
        }
    }
    
    return 0;
}

s32 Player_EndHookshotMove(Player* this) {
    if (this->actor.child != NULL) {
        if (this->heldActor == NULL) {
            this->heldActor = this->actor.child;
            Player_SetRumble(this, 255, 10, 250, 0);
            func_8002F7DC(&this->actor, NA_SE_IT_HOOKSHOT_RECEIVE);
        }
        
        return 1;
    }
    
    return 0;
}

s32 Player_HoldFpsItem(Player* this, PlayState* play) {
    if (this->unk_860 >= 0) {
        this->unk_860 = -this->unk_860;
    }
    
    if (
        (!Player_HoldsHookshot(this) || Player_EndHookshotMove(this)) && !Player_StartZTargetDefend(play, this) &&
        !Player_CanUseFpsItem(this, play)
    ) {
        return 0;
    }
    
    return 1;
}

s32 Player_UpdateShotFpsItem(PlayState* play, Player* this) {
    s32 item;
    s32 arrowType;
    
    if (this->heldActor != NULL) {
        if (!Player_HoldsHookshot(this)) {
            Player_GetFpsItemAmmo(play, this, &item, &arrowType);
            
            if (gSaveContext.minigameState == 1) {
                play->interfaceCtx.hbaAmmo--;
            } else if (play->shootingGalleryStatus != 0) {
                play->shootingGalleryStatus--;
            } else {
                Inventory_ChangeAmmo(item, -1);
            }
            
            if (play->shootingGalleryStatus == 1) {
                play->shootingGalleryStatus = -10;
            }
            
            Player_SetRumble(this, 150, 10, 150, 0);
        } else {
            Player_SetRumble(this, 255, 20, 150, 0);
        }
        
        this->unk_A73 = 4;
        this->heldActor->parent = NULL;
        this->actor.child = NULL;
        this->heldActor = NULL;
        
        return 1;
    }
    
    return 0;
}

static u16 sSfxID_NoAmmo[] = { NA_SE_IT_BOW_FLICK, NA_SE_IT_SLING_FLICK };

s32 Player_ReadyFpsItemToShoot(Player* this, PlayState* play) {
    s32 sp2C;
    
    if (!Player_HoldsHookshot(this)) {
        sp2C = 0;
    } else {
        sp2C = 1;
    }
    
    Math_ScaledStepToS(&this->unk_6C0, 1200, 400);
    this->unk_6AE |= 0x100;
    
    if ((this->unk_836 == 0) && (Player_IsPlayingIdleAnim(this) == 0) && (this->skelAnime.animation == &gPlayerAnim_link_bow_side_walk)) {
        LinkAnimation_PlayOnce(play, &this->skelAnime2, sAnims_FpsItemStandy_Walking[sp2C]);
        this->unk_836 = -1;
    } else if (LinkAnimation_Update(play, &this->skelAnime2)) {
        LinkAnimation_PlayLoop(play, &this->skelAnime2, sAnims_FpsItemStandy[sp2C]);
        this->unk_836 = 1;
    } else if (this->unk_836 == 1) {
        this->unk_836 = 2;
    }
    
    if (this->unk_834 > 10) {
        this->unk_834--;
    }
    
    Player_SetupAimAttention(this, play);
    
    if ((this->unk_836 > 0) && ((this->unk_860 < 0) || (!sActiveItemUseFlag2 && !func_80834E7C(play)))) {
        Player_SetUpperActionFunc(this, Player_AimFpsItem);
        if (this->unk_860 >= 0) {
            if (sp2C == 0) {
                if (!Player_UpdateShotFpsItem(play, this)) {
                    func_8002F7DC(&this->actor, sSfxID_NoAmmo[ABS(this->unk_860) - 1]);
                }
            } else if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_UpdateShotFpsItem(play, this);
            }
        }
        this->unk_834 = 10;
        Player_StopMovement(this);
    } else {
        this->stateFlags1 |= PLAYER_STATE1_9;
    }
    
    return 1;
}

s32 Player_AimFpsItem(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime2);
    
    if (Player_HoldsHookshot(this) && !Player_EndHookshotMove(this)) {
        return 1;
    }
    
    if (!Player_StartZTargetDefend(play, this) && (sActiveItemUseFlag || ((this->unk_860 < 0) && sActiveItemUseFlag2) || Player_CheckShootingGalleryShootInput(play))) {
        this->unk_860 = ABS(this->unk_860);
        
        if (Player_SetupReadyFpsItemToShoot(this, play)) {
            if (Player_HoldsHookshot(this)) {
                this->unk_836 = 1;
            } else {
                LinkAnimation_PlayOnce(play, &this->skelAnime2, &gPlayerAnim_link_bow_bow_shoot_next);
            }
        }
    } else {
        if (this->unk_834 != 0) {
            this->unk_834--;
        }
        
        if (Player_IsZTargeting(this) || (this->unk_6AD != 0) || (this->stateFlags1 & PLAYER_STATE1_20)) {
            if (this->unk_834 == 0) {
                this->unk_834++;
            }
            
            return 1;
        }
        
        if (Player_HoldsHookshot(this)) {
            Player_SetUpperActionFunc(this, Player_HoldFpsItem);
        } else {
            Player_SetUpperActionFunc(this, Player_EndAimFpsItem);
            LinkAnimation_PlayOnce(play, &this->skelAnime2, &gPlayerAnim_link_bow_bow_shoot_end);
        }
        
        this->unk_834 = 0;
    }
    
    return 1;
}

s32 Player_EndAimFpsItem(Player* this, PlayState* play) {
    if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || LinkAnimation_Update(play, &this->skelAnime2)) {
        Player_SetUpperActionFunc(this, Player_HoldFpsItem);
    }
    
    return 1;
}

void Player_SetZTargetFriendlyYaw(Player* this) {
    this->stateFlags1 |= PLAYER_STATE1_17;
    
    if (
        !(this->skelAnime.moveFlags & 0x80) && (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        (sTouchedWallYaw < 0x2000)
    ) {
        this->currentYaw = this->actor.shape.rot.y = this->actor.wallYaw + 0x8000;
    }
    
    this->targetYaw = this->actor.shape.rot.y;
}

s32 Player_InterruptHoldingActor(PlayState* play, Player* this, Actor* arg2) {
    if (arg2 == NULL) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetupStandingStillType(this, play);
        
        return 1;
    }
    
    return 0;
}

void Player_SetupHoldActorUpperAction(Player* this, PlayState* play) {
    if (!Player_InterruptHoldingActor(play, this, this->heldActor)) {
        Player_SetUpperActionFunc(this, Player_HoldActor);
        LinkAnimation_PlayLoop(play, &this->skelAnime2, &gPlayerAnim_link_normal_carryB_wait);
    }
}

s32 Player_HoldActor(Player* this, PlayState* play) {
    Actor* heldActor = this->heldActor;
    
    if (heldActor == NULL) {
        Player_SetupHeldItemUpperActionFunc(play, this);
    }
    
    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    }
    
    if (this->stateFlags1 & PLAYER_STATE1_11) {
        if (LinkAnimation_Update(play, &this->skelAnime2)) {
            LinkAnimation_PlayLoop(play, &this->skelAnime2, &gPlayerAnim_link_normal_carryB_wait);
        }
        
        if ((heldActor->id == ACTOR_EN_NIW) && (this->actor.velocity.y <= 0.0f)) {
            this->actor.minVelocityY = -2.0f;
            this->actor.gravity = -0.5f;
            this->fallStartHeight = this->actor.world.pos.y;
        }
        
        return 1;
    }
    
    return Player_SetupStartZTargetDefend(this, play);
}

void Player_SetLeftHandDlists(Player* this, Gfx** dLists) {
    this->leftHandDLists = dLists + gSaveContext.linkAge;
}

s32 Player_HoldBoomerang(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    }
    
    if (this->stateFlags1 & PLAYER_STATE1_25) {
        Player_SetUpperActionFunc(this, Player_WaitForThrownBoomerang);
    } else if (Player_CanUseFpsItem(this, play)) {
        return 1;
    }
    
    return 0;
}

s32 Player_SetupAimBoomerang(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime2)) {
        Player_SetUpperActionFunc(this, Player_AimBoomerang);
        LinkAnimation_PlayLoop(play, &this->skelAnime2, &gPlayerAnim_link_boom_throw_waitR);
    }
    
    Player_SetupAimAttention(this, play);
    
    return 1;
}

s32 Player_AimBoomerang(Player* this, PlayState* play) {
    LinkAnimationHeader* animSeg = this->skelAnime.animation;
    
    if (
        (Player_GetAnim_FightRight(this) == animSeg) || (Player_GetAnim_FightLeft(this) == animSeg) || (Player_GetAnim_SidewalkRight(this) == animSeg) ||
        (Player_GetAnim_SidewalkLeft(this) == animSeg)
    ) {
        AnimationContext_SetCopyAll(
            play,
            this->skelAnime.limbCount,
            this->skelAnime2.jointTable,
            this->skelAnime.jointTable
        );
    } else {
        LinkAnimation_Update(play, &this->skelAnime2);
    }
    
    Player_SetupAimAttention(this, play);
    
    if (!sActiveItemUseFlag2) {
        Player_SetUpperActionFunc(this, Player_ThrowBoomerang);
        LinkAnimation_PlayOnce(
            play,
            &this->skelAnime2,
            (this->unk_870 < 0.5f) ? &gPlayerAnim_link_boom_throwR : &gPlayerAnim_link_boom_throwL
        );
    }
    
    return 1;
}

s32 Player_ThrowBoomerang(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime2)) {
        Player_SetUpperActionFunc(this, Player_WaitForThrownBoomerang);
        this->unk_834 = 0;
    } else if (LinkAnimation_OnFrame(&this->skelAnime2, 6.0f)) {
        f32 posX = (Math_SinS(this->actor.shape.rot.y) * 10.0f) + this->actor.world.pos.x;
        f32 posZ = (Math_CosS(this->actor.shape.rot.y) * 10.0f) + this->actor.world.pos.z;
        s32 yaw = (this->unk_664 != NULL) ? this->actor.shape.rot.y + 14000 : this->actor.shape.rot.y;
        EnBoom* boomerang =
            (EnBoom*)Actor_Spawn(
            &play->actorCtx,
            play,
            ACTOR_EN_BOOM,
            posX,
            this->actor.world.pos.y + 30.0f,
            posZ,
            this->actor.focus.rot.x,
            yaw,
            0,
            0
            );
        
        this->boomerangActor = &boomerang->actor;
        if (boomerang != NULL) {
            boomerang->moveTo = this->unk_664;
            boomerang->returnTimer = 20;
            this->stateFlags1 |= PLAYER_STATE1_25;
            if (!func_8008E9C4(this)) {
                Player_SetZTargetFriendlyYaw(this);
            }
            this->unk_A73 = 4;
            func_8002F7DC(&this->actor, NA_SE_IT_BOOMERANG_THROW);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
        }
    }
    
    return 1;
}

s32 Player_WaitForThrownBoomerang(Player* this, PlayState* play) {
    if (Player_StartZTargetDefend(play, this)) {
        return 1;
    }
    
    if (!(this->stateFlags1 & PLAYER_STATE1_25)) {
        Player_SetUpperActionFunc(this, Player_CatchBoomerang);
        LinkAnimation_PlayOnce(play, &this->skelAnime2, &gPlayerAnim_link_boom_catch);
        Player_SetLeftHandDlists(this, gPlayerLeftHandBoomerangDLs);
        func_8002F7DC(&this->actor, NA_SE_PL_CATCH_BOOMERANG);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
        
        return 1;
    }
    
    return 0;
}

s32 Player_CatchBoomerang(Player* this, PlayState* play) {
    if (!Player_HoldBoomerang(this, play) && LinkAnimation_Update(play, &this->skelAnime2)) {
        Player_SetUpperActionFunc(this, Player_HoldBoomerang);
    }
    
    return 1;
}

s32 __Player_SetActionFunc(PlayState* play, Player* this, PlayerActionFunc func, s32 flags) {
    if (func == this->func_674) {
        return 0;
    }
    
    if (Player_PlayOcarina == this->func_674) {
        AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
        this->stateFlags2 &= ~(PLAYER_STATE2_24 | PLAYER_STATE2_25);
    } else if (Player_UpdateMagicSpell == this->func_674) {
        Player_ResetSubCam(play, this);
    }
    
    this->func_674 = func;
    
    if (
        (this->itemActionParam != this->heldItemActionParam) &&
        (!(flags & 1) || !(this->stateFlags1 & PLAYER_STATE1_22))
    ) {
        func_8008EC70(this);
    }
    
    if (!(flags & 1) && !(this->stateFlags1 & PLAYER_STATE1_11)) {
        Player_SetupHeldItemUpperActionFunc(play, this);
        this->stateFlags1 &= ~PLAYER_STATE1_22;
    }
    
    Player_EndAnimMovement(this);
    this->stateFlags1 &= ~(PLAYER_STATE1_2 | PLAYER_STATE1_6 | PLAYER_STATE1_26 | PLAYER_STATE1_28 | PLAYER_STATE1_29 |
        PLAYER_STATE1_31);
    this->stateFlags2 &= ~(PLAYER_STATE2_19 | PLAYER_STATE2_27 | PLAYER_STATE2_28);
    this->stateFlags3 &= ~(PLAYER_STATE3_1 | PLAYER_STATE3_3 | PLAYER_STATE3_7);
    this->unk_84F = 0;
    this->unk_850 = 0;
    this->unk_6AC = 0;
    Player_StopInterruptableSfx(this);
    
    return 1;
}

void Player_SetActionFunc_KeepMoveFlags(PlayState* play, Player* this, PlayerActionFunc func, s32 flags) {
    s32 temp;
    
    temp = this->skelAnime.moveFlags;
    this->skelAnime.moveFlags = 0;
    Player_SetActionFunc(play, this, func, flags);
    this->skelAnime.moveFlags = temp;
}

void Player_SetActionFunc_KeepItemAP(PlayState* play, Player* this, PlayerActionFunc func, s32 flags) {
    s32 temp;
    
    if (this->itemActionParam >= 0) {
        temp = this->itemActionParam;
        this->itemActionParam = this->heldItemActionParam;
        Player_SetActionFunc(play, this, func, flags);
        this->itemActionParam = temp;
        Player_SetModels(this, Player_ActionToModelGroup(this, this->itemActionParam));
    }
}

void Player_ChangeCameraSetting(PlayState* play, s16 camSetting) {
    if (!Play_CamIsNotFixed(play)) {
        if (camSetting == CAM_SET_SCENE_TRANSITION) {
            Interface_ChangeAlpha(2);
        }
    } else {
        Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), camSetting);
    }
}

void Player_SetCameraTurnAround(PlayState* play, s32 arg1) {
    Player_ChangeCameraSetting(play, CAM_SET_TURN_AROUND);
    Camera_SetCameraData(Play_GetCamera(play, CAM_ID_MAIN), 4, 0, 0, arg1, 0, 0);
}

void Player_PutAwayHookshot(Player* this) {
    if (Player_HoldsHookshot(this)) {
        Actor* heldActor = this->heldActor;
        
        if (heldActor != NULL) {
            Actor_Kill(heldActor);
            this->actor.child = NULL;
            this->heldActor = NULL;
        }
    }
}

void Player_UseItem(PlayState* play, Player* this, s32 item) {
    s8 actionParam;
    s32 temp;
    s32 nextAnimType;
    
    actionParam = Player_ItemToActionParam(item);
    
    if (
        ((this->heldItemActionParam == this->itemActionParam) &&
        (!(this->stateFlags1 & PLAYER_STATE1_22) || (Player_ActionToMeleeWeapon(actionParam) != 0) ||
        (actionParam == PLAYER_AP_NONE))) ||
        ((this->itemActionParam < 0) &&
        ((Player_ActionToMeleeWeapon(actionParam) != 0) || (actionParam == PLAYER_AP_NONE)))
    ) {
        if (
            (actionParam == PLAYER_AP_NONE) || !(this->stateFlags1 & PLAYER_STATE1_27) ||
            ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
            ((actionParam == PLAYER_AP_HOOKSHOT) || (actionParam == PLAYER_AP_LONGSHOT)))
        ) {
            if (
                (play->bombchuBowlingStatus == 0) &&
                (((actionParam == PLAYER_AP_STICK) && (AMMO(ITEM_STICK) == 0)) ||
                ((actionParam == PLAYER_AP_BEAN) && (AMMO(ITEM_BEAN) == 0)) ||
                (temp = Player_ActionToExplosive(this, actionParam),
                ((temp >= 0) && ((AMMO(sExplosiveInfos[temp].itemId) == 0) ||
                (play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].length >= 3)))))
            ) {
                func_80078884(NA_SE_SY_ERROR);
            } else if (actionParam == PLAYER_AP_LENS) {
                if (Magic_RequestChange(play, 0, MAGIC_CONSUME_LENS)) {
                    if (play->actorCtx.lensActive) {
                        Actor_DisableLens(play);
                    } else {
                        play->actorCtx.lensActive = true;
                    }
                    
                    func_80078884((play->actorCtx.lensActive) ? NA_SE_SY_GLASSMODE_ON : NA_SE_SY_GLASSMODE_OFF);
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if (actionParam == PLAYER_AP_NUT) {
                if (AMMO(ITEM_NUT) != 0) {
                    Player_SetupThrowDekuNut(play, this);
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if ((temp = Player_ActionToMagicSpell(this, actionParam)) >= 0) {
                if (
                    ((actionParam == PLAYER_AP_FARORES_WIND) && (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 0)) ||
                    ((gSaveContext.magicCapacity != 0) && (gSaveContext.magicState == MAGIC_STATE_IDLE) &&
                    (gSaveContext.magic >= sMagicSpellCosts[temp]))
                ) {
                    this->itemActionParam = actionParam;
                    this->unk_6AD = 4;
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            } else if (actionParam >= PLAYER_AP_MASK_KEATON) {
                if (this->currentMask != PLAYER_MASK_NONE) {
                    this->currentMask = PLAYER_MASK_NONE;
                } else {
                    this->currentMask = actionParam - PLAYER_AP_MASK_KEATON + 1;
                }
                
                Player_PlayReactableSfx(this, NA_SE_PL_CHANGE_ARMS);
            } else if (
                ((actionParam >= PLAYER_AP_OCARINA_FAIRY) && (actionParam <= PLAYER_AP_OCARINA_TIME)) ||
                (actionParam >= PLAYER_AP_BOTTLE_FISH)
            ) {
                if (
                    !func_8008E9C4(this) ||
                    ((actionParam >= PLAYER_AP_BOTTLE_POTION_RED) && (actionParam <= PLAYER_AP_BOTTLE_FAIRY))
                ) {
                    TitleCard_Clear(play, &play->actorCtx.titleCtx);
                    this->unk_6AD = 4;
                    this->itemActionParam = actionParam;
                }
            } else if (
                (actionParam != this->heldItemActionParam) ||
                ((this->heldActor == 0) && (Player_ActionToExplosive(this, actionParam) >= 0))
            ) {
                this->nextModelGroup = Player_ActionToModelGroup(this, actionParam);
                nextAnimType = gPlayerModelTypes[this->nextModelGroup][PLAYER_MODELGROUPENTRY_ANIM];
                
                if (
                    (this->heldItemActionParam >= 0) && (Player_ActionToMagicSpell(this, actionParam) < 0) &&
                    (item != this->heldItemId) &&
                    (sAnimTypeSwapTable[gPlayerModelTypes[this->modelGroup][PLAYER_MODELGROUPENTRY_ANIM]][nextAnimType] !=
                    PLAYER_ITEM_CHANGE_DEFAULT)
                ) {
                    this->heldItemId = item;
                    this->stateFlags1 |= PLAYER_STATE1_8;
                } else {
                    Player_PutAwayHookshot(this);
                    Player_DetatchHeldActor(play, this);
                    Player_SetupChangeItemAnim(play, this, actionParam);
                }
            } else {
                sActiveItemUseFlag = sActiveItemUseFlag2 = true;
            }
        }
    }
}

void Player_SetupDie(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    s32 cond = Player_IsSwimming(this);
    
    Player_ResetAttributesAndHeldActor(play, this);
    
    Player_SetActionFunc(play, this, cond ? Player_Drown : Player_Die, 0);
    
    this->stateFlags1 |= PLAYER_STATE1_7;
    
    Player_PlayAnimOnce(play, this, anim);
    if (anim == &gPlayerAnim_link_derth_rebirth) {
        this->skelAnime.endFrame = 84.0f;
    }
    
    Player_ClearAttentionModeAndStopMoving(this);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DOWN);
    
    if (this->actor.category == ACTORCAT_PLAYER) {
        func_800F47BC();
        
        if (Inventory_ConsumeFairy(play)) {
            play->gameOverCtx.state = GAMEOVER_REVIVE_START;
            this->unk_84F = 1;
        } else {
            play->gameOverCtx.state = GAMEOVER_DEATH_START;
            func_800F6AB0(0);
            Audio_PlayFanfare(NA_BGM_GAME_OVER);
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
        }
        
        OnePointCutscene_Init(play, 9806, cond ? 120 : 60, &this->actor, CAM_ID_MAIN);
        Letterbox_SetSizeTarget(0x20);
    }
}

s32 Player_CanUseItem(Player* this) {
    return (!(Player_RunCutsceneFunc == this->func_674) ||
           ((this->stateFlags1 & PLAYER_STATE1_8) &&
           ((this->heldItemId == ITEM_LAST_USED) || (this->heldItemId == ITEM_NONE)))) &&
           (!(Player_StartChangeItem == this->func_82C) ||
           (Player_ItemToActionParam(this->heldItemId) == this->heldItemActionParam));
}

s32 Player_SetupCurrentUpperAction(Player* this, PlayState* play) {
    if (!(this->stateFlags1 & PLAYER_STATE1_23) && (this->actor.parent != NULL) && Player_HoldsHookshot(this)) {
        Player_SetActionFunc(play, this, Player_MoveAlongHookshotPath, 1);
        this->stateFlags3 |= PLAYER_STATE3_7;
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_hook_fly_start);
        Player_SetupAnimMovement(play, this, 0x9B);
        Player_ClearAttentionModeAndStopMoving(this);
        this->currentYaw = this->actor.shape.rot.y;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        this->hoverBootsTimer = 0;
        this->unk_6AE |= 0x43;
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
        
        return 1;
    }
    
    if (Player_CanUseItem(this)) {
        Player_SetupItem(this, play);
        if (Player_ThrowDekuNut == this->func_674) {
            return 1;
        }
    }
    
    if (!this->func_82C(this, play)) {
        return 0;
    }
    
    if (this->unk_830 != 0.0f) {
        if ((Player_IsPlayingIdleAnim(this) == 0) || (this->linearVelocity != 0.0f)) {
            AnimationContext_SetCopyFalse(
                play,
                this->skelAnime.limbCount,
                this->skelAnime2.jointTable,
                this->skelAnime.jointTable,
                D_80853410
            );
        }
        Math_StepToF(&this->unk_830, 0.0f, 0.25f);
        AnimationContext_SetInterp(
            play,
            this->skelAnime.limbCount,
            this->skelAnime.jointTable,
            this->skelAnime2.jointTable,
            1.0f - this->unk_830
        );
    } else if ((Player_IsPlayingIdleAnim(this) == 0) || (this->linearVelocity != 0.0f)) {
        AnimationContext_SetCopyTrue(
            play,
            this->skelAnime.limbCount,
            this->skelAnime.jointTable,
            this->skelAnime2.jointTable,
            D_80853410
        );
    } else {
        AnimationContext_SetCopyAll(
            play,
            this->skelAnime.limbCount,
            this->skelAnime.jointTable,
            this->skelAnime2.jointTable
        );
    }
    
    return 1;
}

s32 Player_SetupCsActionFunc(PlayState* play, Player* this, PlayerCutsceneFunc func) {
    this->func_A74 = func;
    Player_SetActionFunc(play, this, Player_RunCutsceneFunc, 0);
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    return Player_UnequipItem(play, this);
}

void Player_UpdateYaw(Player* this, PlayState* play) {
    s16 previousYaw = this->actor.shape.rot.y;
    
    if (!(this->stateFlags2 & (PLAYER_STATE2_5 | PLAYER_STATE2_6))) {
        if (
            (this->unk_664 != NULL) &&
            ((play->actorCtx.targetCtx.unk_4B != 0) || (this->actor.category != ACTORCAT_PLAYER))
        ) {
            Math_ScaledStepToS(
                &this->actor.shape.rot.y,
                Math_Vec3f_Yaw(&this->actor.world.pos, &this->unk_664->focus.pos),
                4000
            );
        } else if (
            (this->stateFlags1 & PLAYER_STATE1_17) &&
            !(this->stateFlags2 & (PLAYER_STATE2_5 | PLAYER_STATE2_6))
        ) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, this->targetYaw, 4000);
        }
    } else if (!(this->stateFlags2 & PLAYER_STATE2_6)) {
        Math_ScaledStepToS(&this->actor.shape.rot.y, this->currentYaw, 2000);
    }
    
    this->unk_87C = this->actor.shape.rot.y - previousYaw;
}

s32 Player_StepAngleOffset(s16* pValue, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5) {
    s16 temp1;
    s16 temp2;
    s16 temp3;
    
    temp1 = temp2 = arg4 - *pValue;
    temp2 = CLAMP(temp2, -arg5, arg5);
    *pValue += (s16)(temp1 - temp2);
    
    Math_ScaledStepToS(pValue, arg1, arg2);
    
    temp3 = *pValue;
    if (*pValue < -arg3) {
        *pValue = -arg3;
    } else if (*pValue > arg3) {
        *pValue = arg3;
    }
    
    return temp3 - *pValue;
}

s32 Player_UpdateLookRot(Player* this, s32 arg1) {
    s16 sp36;
    s16 var;
    
    var = this->actor.shape.rot.y;
    if (arg1 != 0) {
        var = this->actor.focus.rot.y;
        this->unk_6BC = this->actor.focus.rot.x;
        this->unk_6AE |= 0x41;
    } else {
        Player_StepAngleOffset(
            &this->unk_6BC,
            Player_StepAngleOffset(&this->unk_6B6, this->actor.focus.rot.x, 600, 10000, this->actor.focus.rot.x, 0),
            200,
            4000,
            this->unk_6B6,
            10000
        );
        sp36 = this->actor.focus.rot.y - var;
        Player_StepAngleOffset(&sp36, 0, 200, 24000, this->unk_6BE, 8000);
        var = this->actor.focus.rot.y - sp36;
        Player_StepAngleOffset(&this->unk_6B8, sp36 - this->unk_6BE, 200, 8000, sp36, 8000);
        Player_StepAngleOffset(&this->unk_6BE, sp36, 200, 8000, this->unk_6B8, 8000);
        this->unk_6AE |= 0xD9;
    }
    
    return var;
}

void Player_SetupZTargeting(Player* this, PlayState* play) {
    s32 sp1C = 0;
    s32 zTrigPressed = CHECK_BTN_ALL(sControlInput->cur.button, BTN_Z);
    Actor* actorToTarget;
    s32 holdTarget;
    s32 cond;
    
    if (!zTrigPressed) {
        this->stateFlags1 &= ~PLAYER_STATE1_30;
    }
    
    if (
        (play->csCtx.state != CS_STATE_IDLE) || (this->csMode != 0) ||
        (this->stateFlags1 & (PLAYER_STATE1_7 | PLAYER_STATE1_29)) || (this->stateFlags3 & PLAYER_STATE3_7)
    ) {
        this->unk_66C = 0;
    } else if (zTrigPressed || (this->stateFlags2 & PLAYER_STATE2_13) || (this->unk_684 != NULL)) {
        if (this->unk_66C <= 5) {
            this->unk_66C = 5;
        } else {
            this->unk_66C--;
        }
    } else if (this->stateFlags1 & PLAYER_STATE1_17) {
        this->unk_66C = 0;
    } else if (this->unk_66C != 0) {
        this->unk_66C--;
    }
    
    if (this->unk_66C >= 6) {
        sp1C = 1;
    }
    
    cond = Player_CheckActorTalkRequested(play);
    if (cond || (this->unk_66C != 0) || (this->stateFlags1 & (PLAYER_STATE1_12 | PLAYER_STATE1_25))) {
        if (!cond) {
            if (
                !(this->stateFlags1 & PLAYER_STATE1_25) &&
                ((this->heldItemActionParam != PLAYER_AP_FISHING_POLE) || (this->unk_860 == 0)) &&
                CHECK_BTN_ALL(sControlInput->press.button, BTN_Z)
            ) {
                if (this->actor.category == ACTORCAT_PLAYER) {
                    actorToTarget = play->actorCtx.targetCtx.arrowPointedActor;
                } else {
                    actorToTarget = &GET_PLAYER(play)->actor;
                }
                
                holdTarget = (gSaveContext.zTargetSetting != 0) || (this->actor.category != ACTORCAT_PLAYER);
                this->stateFlags1 |= PLAYER_STATE1_15;
                
                if ((actorToTarget != NULL) && !(actorToTarget->flags & ACTOR_FLAG_27)) {
                    if ((actorToTarget == this->unk_664) && (this->actor.category == ACTORCAT_PLAYER)) {
                        actorToTarget = play->actorCtx.targetCtx.unk_94;
                    }
                    
                    if (actorToTarget != this->unk_664) {
                        if (!holdTarget) {
                            this->stateFlags2 |= PLAYER_STATE2_13;
                        }
                        this->unk_664 = actorToTarget;
                        this->unk_66C = 15;
                        this->stateFlags2 &= ~(PLAYER_STATE2_1 | PLAYER_STATE2_21);
                    } else {
                        if (!holdTarget) {
                            func_8008EDF0(this);
                        }
                    }
                    
                    this->stateFlags1 &= ~PLAYER_STATE1_30;
                } else {
                    if (!(this->stateFlags1 & (PLAYER_STATE1_17 | PLAYER_STATE1_30))) {
                        Player_SetZTargetFriendlyYaw(this);
                    }
                }
            }
            
            if (this->unk_664 != NULL) {
                if (
                    (this->actor.category == ACTORCAT_PLAYER) && (this->unk_664 != this->unk_684) &&
                    func_8002F0C8(this->unk_664, this, sp1C)
                ) {
                    func_8008EDF0(this);
                    this->stateFlags1 |= PLAYER_STATE1_30;
                } else if (this->unk_664 != NULL) {
                    this->unk_664->targetPriority = 40;
                }
            } else if (this->unk_684 != NULL) {
                this->unk_664 = this->unk_684;
            }
        }
        
        if (this->unk_664 != NULL) {
            this->stateFlags1 &= ~(PLAYER_STATE1_16 | PLAYER_STATE1_17);
            if (
                (this->stateFlags1 & PLAYER_STATE1_11) ||
                !CHECK_FLAG_ALL(this->unk_664->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)
            ) {
                this->stateFlags1 |= PLAYER_STATE1_16;
            }
        } else {
            if (this->stateFlags1 & PLAYER_STATE1_17) {
                this->stateFlags2 &= ~PLAYER_STATE2_13;
            } else {
                func_8008EE08(this);
            }
        }
    } else {
        func_8008EE08(this);
    }
}

s32 Player_CalculateTargetVelAndYaw(PlayState* play, Player* this, f32* arg2, s16* arg3, f32 arg4) {
    f32 temp_f2;
    f32 temp_f0;
    f32 temp_f14;
    f32 temp_f12;
    
    if (
        (this->unk_6AD != 0) || (play->transitionTrigger == TRANS_TRIGGER_START) ||
        (this->stateFlags1 & PLAYER_STATE1_0)
    ) {
        *arg2 = 0.0f;
        *arg3 = this->actor.shape.rot.y;
    } else {
        *arg2 = sAnalogStickMod;
        *arg3 = sAnalogStickYaw;
        
        if (arg4 != 0.0f) {
            *arg2 -= 20.0f;
            if (*arg2 < 0.0f) {
                *arg2 = 0.0f;
            } else {
                temp_f2 = 1.0f - Math_CosS(*arg2 * 450.0f);
                *arg2 = ((temp_f2 * temp_f2) * 30.0f) + 7.0f;
            }
        } else {
            *arg2 *= 0.8f;
        }
        
        if (sAnalogStickMod != 0.0f) {
            temp_f0 = Math_SinS(this->unk_898);
            temp_f12 = this->unk_880;
            temp_f14 = CLAMP(temp_f0, 0.0f, 0.6f);
            
            if (this->unk_6C4 != 0.0f) {
                temp_f12 = temp_f12 - (this->unk_6C4 * 0.008f);
                if (temp_f12 < 2.0f) {
                    temp_f12 = 2.0f;
                }
            }
            
            *arg2 = (*arg2 * 0.14f) - (8.0f * temp_f14 * temp_f14);
            *arg2 = CLAMP(*arg2, 0.0f, temp_f12);
            
            return 1;
        }
    }
    
    return 0;
}

s32 Player_StepLinearVelToZero(Player* this) {
    return Math_StepToF(&this->linearVelocity, 0.0f, REG(43) / 100.0f);
}

s32 Player_GetTargetVelAndYaw(Player* this, f32* arg1, s16* arg2, f32 arg3, PlayState* play) {
    if (!Player_CalculateTargetVelAndYaw(play, this, arg1, arg2, arg3)) {
        *arg2 = this->actor.shape.rot.y;
        
        if (this->unk_664 != NULL) {
            if ((play->actorCtx.targetCtx.unk_4B != 0) && !(this->stateFlags2 & PLAYER_STATE2_6)) {
                *arg2 = Math_Vec3f_Yaw(&this->actor.world.pos, &this->unk_664->focus.pos);
                
                return 0;
            }
        } else if (Player_IsFriendlyZTargeting(this)) {
            *arg2 = this->targetYaw;
        }
        
        return 0;
    } else {
        *arg2 += Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        
        return 1;
    }
}

static s8 sAction_StandStill_TargetEnemy[] = { 13, 2, 4, 9, 10, 11, 8, -7 };
static s8 sAction_StandStill_TargetFriendly[] = { 13, 1, 2, 5, 3, 4, 9, 10, 11, 7, 8, -6 };
static s8 sAction_SidewalkStop[] = { 13, 1, 2, 3, 4, 9, 10, 11, 8, 7, -6 };
static s8 sAction_Backwalk_TargetFriendly[] = { 13, 2, 4, 9, 10, 11, 8, -7 };
static s8 sAction_Sidewalk[] = { 13, 2, 4, 9, 10, 11, 12, 8, -7 };
static s8 sAction_Turn[] = { -7 };
static s8 sAction_StandStill[] = { 0, 11, 1, 2, 3, 5, 4, 9, 8, 7, -6 };
static s8 sAction_Run[] = { 0, 11, 1, 2, 3, 12, 5, 4, 9, 8, 7, -6 };
static s8 sAction_Run_Targeting[] = { 13, 1, 2, 3, 12, 5, 4, 9, 10, 11, 8, 7, -6 };
static s8 sAction_BackwalkStop[] = { 10, 8, -7 };
static s8 sAction_Swim[] = { 0, 12, 5, -4 };

s32 Player_SetupAction(PlayState* play, Player* this, s8* arg2, s32 arg3) {
    static s32 (*sActions[])(Player* this, PlayState* play) = {
        [0] = Player_SetupCUpBehavior,
        [1] = Player_SetupOpenDoor,
        [2] = Player_SetupGetItemOrHoldBehavior,
        [3] = Player_SetupMountHorse,
        [4] = Player_SetupSpeakOrCheck,
        [5] = Player_SetupSpecialWallInteraction,
        [6] = Player_SetupRollOrPutAway,
        [7] = Player_SetupStartMeleeWeaponAttack,
        [8] = Player_SetupStartChargeSpinAttack,
        [9] = Player_SetupPutDownOrThrowActor,
        [10] = Player_SetupJumpSlashOrRoll,
        [11] = Player_SetupDefend,
        [12] = Player_SetupWallJumpBehavior,
        [13] = Player_SetupItemCsOrFirstPerson,
    };
    
    if (!(this->stateFlags1 & (PLAYER_STATE1_0 | PLAYER_STATE1_7 | PLAYER_STATE1_29))) {
        if (arg3 != 0) {
            D_808535E0 = Player_SetupCurrentUpperAction(this, play);
            if (Player_ThrowDekuNut == this->func_674) {
                return 1;
            }
        }
        
        if (func_8008F128(this)) {
            this->unk_6AE |= 0x41;
            
            return 1;
        }
        
        if (!(this->stateFlags1 & PLAYER_STATE1_8) && (Player_StartChangeItem != this->func_82C)) {
            while (*arg2 >= 0) {
                if (sActions[*arg2](this, play)) {
                    return 1;
                }
                arg2++;
            }
            
            if (sActions[-(*arg2)](this, play)) {
                return 1;
            }
        }
    }
    
    return 0;
}

s32 Player_IsActionInterrupted(PlayState* play, Player* this, SkelAnime* skelAnime, f32 arg3) {
    f32 sp24;
    s16 sp22;
    
    if ((skelAnime->endFrame - arg3) <= skelAnime->curFrame) {
        if (Player_SetupAction(play, this, sAction_StandStill, 1)) {
            return 0;
        }
        
        if (Player_GetTargetVelAndYaw(this, &sp24, &sp22, 0.018f, play)) {
            return 1;
        }
    }
    
    return -1;
}

void Player_SetupSpinAttackActor(PlayState* play, Player* this, s32 arg2) {
    if (arg2 != 0) {
        this->unk_858 = 0.0f;
    } else {
        this->unk_858 = 0.5f;
    }
    
    this->stateFlags1 |= PLAYER_STATE1_12;
    
    if (this->actor.category == ACTORCAT_PLAYER) {
        Actor_Spawn(
            &play->actorCtx,
            play,
            ACTOR_EN_M_THUNDER,
            this->bodyPartsPos[PLAYER_BODYPART_WAIST].x,
            this->bodyPartsPos[PLAYER_BODYPART_WAIST].y,
            this->bodyPartsPos[PLAYER_BODYPART_WAIST].z,
            0,
            0,
            0,
            Player_GetMeleeWeaponHeld(this) | arg2
        );
    }
}

s32 Player_CanQuickspin(Player* this) {
    s8 sp3C[4];
    s8* iter;
    s8* iter2;
    s8 temp1;
    s8 temp2;
    s32 i;
    
    if ((this->heldItemActionParam == PLAYER_AP_STICK) || Player_HoldsBrokenKnife(this)) {
        return 0;
    }
    
    iter = &this->unk_847[0];
    iter2 = &sp3C[0];
    for (i = 0; i < 4; i++, iter++, iter2++) {
        if ((*iter2 = *iter) < 0) {
            return 0;
        }
        *iter2 *= 2;
    }
    
    temp1 = sp3C[0] - sp3C[1];
    if (ABS(temp1) < 10) {
        return 0;
    }
    
    iter2 = &sp3C[1];
    for (i = 1; i < 3; i++, iter2++) {
        temp2 = *iter2 - *(iter2 + 1);
        if ((ABS(temp2) < 10) || (temp2 * temp1 < 0)) {
            return 0;
        }
    }
    
    return 1;
}

void Player_SetupSpinAttackAnims(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    
    if (
        (this->meleeWeaponAnimation >= PLAYER_MWA_RIGHT_SLASH_1H) &&
        (this->meleeWeaponAnimation <= PLAYER_MWA_RIGHT_COMBO_2H)
    ) {
        anim = sAnims_SpinAttack1[Player_HoldsTwoHandedWeapon(this)];
    } else {
        anim = sAnims_SpinAttack2[Player_HoldsTwoHandedWeapon(this)];
    }
    
    Player_InactivateMeleeWeapon(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, 8.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE, -9.0f);
    Player_SetupSpinAttackActor(play, this, 0x200);
}

void Player_StartChargeSpinAttack(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_ChargeSpinAttack, 1);
    Player_SetupSpinAttackAnims(play, this);
}

s32 Player_GetMeleeAttackAnimID(Player* this) {
    static s8 sSwordAttackType[] = {
        PLAYER_MWA_STAB_1H,
        PLAYER_MWA_RIGHT_SLASH_1H,
        PLAYER_MWA_RIGHT_SLASH_1H,
        PLAYER_MWA_LEFT_SLASH_1H,
    };
    static s8 sHammerAttackType[] = {
        PLAYER_MWA_HAMMER_FORWARD,
        PLAYER_MWA_HAMMER_SIDE,
        PLAYER_MWA_HAMMER_FORWARD,
        PLAYER_MWA_HAMMER_SIDE,
    };
    s32 sp1C = this->unk_84B[this->unk_846];
    s32 sp18;
    
    if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
        if (sp1C < 0) {
            sp1C = 0;
        }
        sp18 = sHammerAttackType[sp1C];
        this->unk_845 = 0;
    } else {
        if (Player_CanQuickspin(this)) {
            sp18 = PLAYER_MWA_SPIN_ATTACK_1H;
        } else {
            if (sp1C < 0) {
                if (Player_IsZTargeting(this)) {
                    sp18 = PLAYER_MWA_FORWARD_SLASH_1H;
                } else {
                    sp18 = PLAYER_MWA_RIGHT_SLASH_1H;
                }
            } else {
                sp18 = sSwordAttackType[sp1C];
                if (sp18 == PLAYER_MWA_STAB_1H) {
                    this->stateFlags2 |= PLAYER_STATE2_30;
                    if (!Player_IsZTargeting(this)) {
                        sp18 = PLAYER_MWA_FORWARD_SLASH_1H;
                    }
                }
            }
            if (this->heldItemActionParam == PLAYER_AP_STICK) {
                sp18 = PLAYER_MWA_FORWARD_SLASH_1H;
            }
        }
        if (Player_HoldsTwoHandedWeapon(this)) {
            sp18++;
        }
    }
    
    return sp18;
}

void Player_SetupMeleeWeaponToucherFlags(Player* this, s32 quadIndex, u32 dmgFlags) {
    this->meleeWeaponQuads[quadIndex].info.toucher.dmgFlags = dmgFlags;
    
    if (dmgFlags == DMG_DEKU_STICK) {
        this->meleeWeaponQuads[quadIndex].info.toucherFlags = TOUCH_ON | TOUCH_NEAREST | TOUCH_SFX_WOOD;
    } else {
        this->meleeWeaponQuads[quadIndex].info.toucherFlags = TOUCH_ON | TOUCH_NEAREST;
    }
}

void Player_StartMeleeWeaponAttack(PlayState* play, Player* this, s32 arg2) {
    static u32 sMeleeWeaponDamageFlag[][2] = {
        { DMG_SLASH_MASTER, DMG_JUMP_MASTER   },
        { DMG_SLASH_KOKIRI, DMG_JUMP_KOKIRI   },
        { DMG_SLASH_GIANT,  DMG_JUMP_GIANT    },
        { DMG_DEKU_STICK,   DMG_JUMP_MASTER   },
        { DMG_HAMMER_SWING, DMG_HAMMER_JUMP   },
    };
    u32 dmgFlags;
    s32 temp;
    
    Player_SetActionFunc(play, this, Player_MeleeWeaponAttack, 0);
    this->unk_844 = 8;
    if (!((arg2 >= PLAYER_MWA_FLIPSLASH_FINISH) && (arg2 <= PLAYER_MWA_JUMPSLASH_FINISH))) {
        Player_InactivateMeleeWeapon(this);
    }
    
    if ((arg2 != this->meleeWeaponAnimation) || !(this->unk_845 < 3)) {
        this->unk_845 = 0;
    }
    
    this->unk_845++;
    if (this->unk_845 >= 3) {
        arg2 += 2;
    }
    
    this->meleeWeaponAnimation = arg2;
    
    Player_PlayAnimOnceSlowed(play, this, sAnims_MeleeAttack[arg2].unk_00);
    if ((arg2 != PLAYER_MWA_FLIPSLASH_START) && (arg2 != PLAYER_MWA_JUMPSLASH_START)) {
        Player_SetupAnimMovement(play, this, 0x209);
    }
    
    this->currentYaw = this->actor.shape.rot.y;
    
    if (Player_HoldsBrokenKnife(this)) {
        temp = 1;
    } else {
        temp = Player_GetMeleeWeaponHeld(this) - 1;
    }
    
    if ((arg2 >= PLAYER_MWA_FLIPSLASH_START) && (arg2 <= PLAYER_MWA_JUMPSLASH_FINISH)) {
        dmgFlags = sMeleeWeaponDamageFlag[temp][1];
    } else {
        dmgFlags = sMeleeWeaponDamageFlag[temp][0];
    }
    
    Player_SetupMeleeWeaponToucherFlags(this, 0, dmgFlags);
    Player_SetupMeleeWeaponToucherFlags(this, 1, dmgFlags);
}

void Player_SetupInvincibility(Player* this, s32 timer) {
    if (this->invincibilityTimer >= 0) {
        this->invincibilityTimer = timer;
        this->unk_88F = 0;
    }
}

void Player_SetupInvincibility_NoDamageFlash(Player* this, s32 timer) {
    if (this->invincibilityTimer > timer) {
        this->invincibilityTimer = timer;
    }
    this->unk_88F = 0;
}

s32 Player_ApplyDamage(PlayState* play, Player* this, s32 damage) {
    if ((this->invincibilityTimer != 0) || (this->actor.category != ACTORCAT_PLAYER)) {
        return 1;
    }
    
    return Health_ChangeBy(play, damage);
}

void Player_SetLedgeGrabPosition(Player* this) {
    this->skelAnime.prevTransl = this->skelAnime.jointTable[0];
    Player_UpdateAnimMovement(this, 3);
}

void Player_SetupFallFromLedge(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UpdateMidair, 0);
    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
    this->unk_850 = 1;
    if (this->unk_6AD != 3) {
        this->unk_6AD = 0;
    }
}

void Player_SetupDamage(PlayState* play, Player* this, s32 arg2, f32 arg3, f32 arg4, s16 arg5, s32 arg6) {
    static LinkAnimationHeader* sAnims_Damage[] = {
        &gPlayerAnim_link_normal_front_shit, &gPlayerAnim_link_normal_front_shitR, &gPlayerAnim_link_normal_back_shit, &gPlayerAnim_link_normal_back_shitR,
        &gPlayerAnim_link_normal_front_hit,  &gPlayerAnim_link_anchor_front_hitR,  &gPlayerAnim_link_normal_back_hit,  &gPlayerAnim_link_anchor_back_hitR,
    };
    LinkAnimationHeader* sp2C = NULL;
    LinkAnimationHeader** sp28;
    
    if (this->stateFlags1 & PLAYER_STATE1_13) {
        Player_SetLedgeGrabPosition(this);
    }
    
    this->unk_890 = 0;
    
    func_8002F7DC(&this->actor, NA_SE_PL_DAMAGE);
    
    if (!Player_ApplyDamage(play, this, 0 - this->actor.colChkInfo.damage)) {
        this->stateFlags2 &= ~PLAYER_STATE2_7;
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !(this->stateFlags1 & PLAYER_STATE1_27)) {
            Player_SetupFallFromLedge(this, play);
        }
        
        return;
    }
    
    Player_SetupInvincibility(this, arg6);
    
    if (arg2 == 3) {
        Player_SetActionFunc(play, this, Player_FrozenInIce, 0);
        
        sp2C = &gPlayerAnim_link_normal_ice_down;
        
        Player_ClearAttentionModeAndStopMoving(this);
        Player_SetRumble(this, 255, 10, 40, 0);
        
        func_8002F7DC(&this->actor, NA_SE_PL_FREEZE_S);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
    } else if (arg2 == 4) {
        Player_SetActionFunc(play, this, Player_SetupElectricShock, 0);
        
        Player_SetRumble(this, 255, 80, 150, 0);
        
        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_normal_electric_shock);
        Player_ClearAttentionModeAndStopMoving(this);
        
        this->unk_850 = 20;
    } else {
        arg5 -= this->actor.shape.rot.y;
        if (this->stateFlags1 & PLAYER_STATE1_27) {
            Player_SetActionFunc(play, this, Player_DamagedSwim, 0);
            Player_SetRumble(this, 180, 20, 50, 0);
            
            this->linearVelocity = 4.0f;
            this->actor.velocity.y = 0.0f;
            
            sp2C = &gPlayerAnim_link_swimer_swim_hit;
            
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        } else if (
            (arg2 == 1) || (arg2 == 2) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
            (this->stateFlags1 & (PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_21))
        ) {
            Player_SetActionFunc(play, this, Player_StartKnockback, 0);
            
            this->stateFlags3 |= PLAYER_STATE3_1;
            
            Player_SetRumble(this, 255, 20, 150, 0);
            Player_ClearAttentionModeAndStopMoving(this);
            
            if (arg2 == 2) {
                this->unk_850 = 4;
                
                this->actor.speedXZ = 3.0f;
                this->linearVelocity = 3.0f;
                this->actor.velocity.y = 6.0f;
                
                Player_ChangeAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_3, this->modelAnimType));
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
            } else {
                this->actor.speedXZ = arg3;
                this->linearVelocity = arg3;
                this->actor.velocity.y = arg4;
                
                if (ABS(arg5) > 0x4000) {
                    sp2C = &gPlayerAnim_link_normal_front_downA;
                } else {
                    sp2C = &gPlayerAnim_link_normal_back_downA;
                }
                
                if ((this->actor.category != ACTORCAT_PLAYER) && (this->actor.colChkInfo.health == 0)) {
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_BL_DOWN);
                } else {
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
                }
            }
            
            this->hoverBootsTimer = 0;
            this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        } else {
            if ((this->linearVelocity > 4.0f) && !func_8008E9C4(this)) {
                this->unk_890 = 20;
                Player_SetRumble(this, 120, 20, 10, 0);
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
                
                return;
            }
            
            sp28 = sAnims_Damage;
            
            Player_SetActionFunc(play, this, func_8084370C, 0);
            Player_ResetLeftRightBlendWeight(this);
            
            if (this->actor.colChkInfo.damage < 5) {
                Player_SetRumble(this, 120, 20, 10, 0);
            } else {
                Player_SetRumble(this, 180, 20, 100, 0);
                this->linearVelocity = 23.0f;
                sp28 += 4;
            }
            
            if (ABS(arg5) <= 0x4000) {
                sp28 += 2;
            }
            
            if (func_8008E9C4(this)) {
                sp28 += 1;
            }
            
            sp2C = *sp28;
            
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        }
        
        this->actor.shape.rot.y += arg5;
        this->currentYaw = this->actor.shape.rot.y;
        this->actor.world.rot.y = this->actor.shape.rot.y;
        if (ABS(arg5) > 0x4000) {
            this->actor.shape.rot.y += 0x8000;
        }
    }
    
    Player_ResetAttributesAndHeldActor(play, this);
    
    this->stateFlags1 |= PLAYER_STATE1_26;
    
    if (sp2C != NULL) {
        Player_PlayAnimOnceSlowed(play, this, sp2C);
    }
}

typedef enum {
    PLAYER_FLOORDAMAGE_NONE = -1,
    PLAYER_FLOODRAMAGE_SPIKE,
    PLAYER_FLOORDAMAGE_FIRE,
    PLAYER_FLOORDAMAGE_MAX,
} FloorDamageType;

s32 Player_GetFloorDamageType(s32 arg0) {
    s32 temp = arg0 - 2;
    
    if ((temp >= PLAYER_FLOODRAMAGE_SPIKE) && (temp < PLAYER_FLOORDAMAGE_MAX)) {
        return temp;
    } else {
        return PLAYER_FLOORDAMAGE_NONE;
    }
}

s32 Player_IsFloorSinkingSand(s32 arg0) {
    return (arg0 == SURFACE_BEHAVIOUR_SAND) ||
           (arg0 == SURFACE_BEHAVIOUR_QUICKSAND) ||
           (arg0 == SURFACE_BEHAVIOUR_QUICKSAND_EPONA);
}

void Player_BurnDekuShield(Player* this, PlayState* play) {
    if (this->currentShield == PLAYER_SHIELD_DEKU) {
        Actor_Spawn(
            &play->actorCtx,
            play,
            ACTOR_ITEM_SHIELD,
            this->actor.world.pos.x,
            this->actor.world.pos.y,
            this->actor.world.pos.z,
            0,
            0,
            0,
            1
        );
        Inventory_DeleteEquipment(play, EQUIP_TYPE_SHIELD);
        Message_StartTextbox(play, 0x305F, NULL);
    }
}

void Player_SetLinkBurning(Player* this) {
    s32 i;
    
    for (i = 0; i < PLAYER_BODYPART_MAX; i++)
        this->flameTimers[i] = Rand_S16Offset(0, 200);
    
    this->isBurning = true;
}

void Player_PlayFallSfx(Player* this) {
    if (this->actor.colChkInfo.acHitEffect == 1) {
        Player_SetLinkBurning(this);
    }
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

void Player_RoundUpInvincibilityTimer(Player* this) {
    if ((this->invincibilityTimer >= 0) && (this->invincibilityTimer < 20)) {
        this->invincibilityTimer = 20;
    }
}

s32 Player_UpdateDamage(Player* this, PlayState* play) {
    s32 sp68 = false;
    s32 sp64;
    
    if (this->unk_A86 != 0) {
        if (!Player_InBlockingCsMode(play, this)) {
            Player_InflictDamage(play, -16);
            this->unk_A86 = 0;
        }
    } else {
        sp68 = ((Player_GetHeight(this) - 8.0f) < (this->unk_6C4 * this->actor.scale.y));
        
        if (
            sp68 || (this->actor.bgCheckFlags & BGCHECKFLAG_CRUSHED) || (sFloorSpecialProperty == 9) ||
            (this->stateFlags2 & PLAYER_STATE2_31)
        ) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
            
            if (sp68) {
                Play_TriggerRespawn(play);
                Play_SetFadeOut(play);
            } else {
                // Special case for getting crushed in Forest Temple's Checkboard Ceiling Hall or Shadow Temple's
                // Falling Spike Trap Room, to respawn the player in a specific place
                if (
                    ((play->sceneId == SCENE_BMORI1) && (play->roomCtx.curRoom.num == 15)) ||
                    ((play->sceneId == SCENE_HAKADAN) && (play->roomCtx.curRoom.num == 10))
                ) {
                    static SpecialRespawnInfo checkboardCeilingRespawn = { { 1992.0f, 403.0f, -3432.0f }, 0 };
                    static SpecialRespawnInfo fallingSpikeTrapRespawn = { { 1200.0f, -1343.0f, 3850.0f }, 0 };
                    SpecialRespawnInfo* respawnInfo;
                    
                    if (play->sceneId == SCENE_BMORI1) {
                        respawnInfo = &checkboardCeilingRespawn;
                    } else {
                        respawnInfo = &fallingSpikeTrapRespawn;
                    }
                    
                    Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = respawnInfo->pos;
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = respawnInfo->yaw;
                }
                
                Play_TriggerVoidOut(play);
            }
            
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_TAKEN_AWAY);
            play->unk_11DE9 = true;
            func_80078884(NA_SE_OC_ABYSS);
        } else if ((this->unk_8A1 != 0) && ((this->unk_8A1 >= 2) || (this->invincibilityTimer == 0))) {
            u8 sp5C[] = { 2, 1, 1 };
            
            Player_PlayFallSfx(this);
            
            if (this->unk_8A1 == 3) {
                this->shockTimer = 40;
            }
            
            this->actor.colChkInfo.damage += this->unk_8A0;
            Player_SetupDamage(play, this, sp5C[this->unk_8A1 - 1], this->unk_8A4, this->unk_8A8, this->unk_8A2, 20);
        } else {
            sp64 = (this->shieldQuad.base.acFlags & AC_BOUNCED) != 0;
            
            //! @bug The second set of conditions here seems intended as a way for Link to "block" hits by rolling.
            // However, `Collider.atFlags` is a byte so the flag check at the end is incorrect and cannot work.
            // Additionally, `Collider.atHit` can never be set while already colliding as AC, so it's also bugged.
            // This behavior was later fixed in MM, most likely by removing both the `atHit` and `atFlags` checks.
            if (
                sp64 || ((this->invincibilityTimer < 0) && (this->cylinder.base.acFlags & AC_HIT) &&
                (this->cylinder.info.atHit != NULL) && (this->cylinder.info.atHit->atFlags & 0x20000000))
            ) {
                Player_SetRumble(this, 180, 20, 100, 0);
                
                if (!Player_IsChildWithHylianShield(this)) {
                    if (this->invincibilityTimer >= 0) {
                        LinkAnimationHeader* anim;
                        s32 sp54 = Player_AimShieldCrouched == this->func_674;
                        
                        if (!Player_IsSwimming(this)) {
                            Player_SetActionFunc(play, this, Player_DeflectAttackWithShield, 0);
                        }
                        
                        if (!(this->unk_84F = sp54)) {
                            Player_SetUpperActionFunc(this, Player_EndDeflectAttackStanding);
                            
                            if (this->unk_870 < 0.5f) {
                                anim = sAnims_DefendStanceRight_Deflect[Player_HoldsTwoHandedWeapon(this)];
                            } else {
                                anim = sAnims_DefendStanceLeft_Deflect[Player_HoldsTwoHandedWeapon(this)];
                            }
                            LinkAnimation_PlayOnce(play, &this->skelAnime2, anim);
                        } else {
                            Player_PlayAnimOnce(play, this, sAnims_Deflect[Player_HoldsTwoHandedWeapon(this)]);
                        }
                    }
                    
                    if (!(this->stateFlags1 & (PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_21))) {
                        this->linearVelocity = -18.0f;
                        this->currentYaw = this->actor.shape.rot.y;
                    }
                }
                
                if (sp64 && (this->shieldQuad.info.acHitInfo->toucher.effect == 1)) {
                    Player_BurnDekuShield(this, play);
                }
                
                return 0;
            }
            
            if (
                (this->unk_A87 != 0) || (this->invincibilityTimer > 0) || (this->stateFlags1 & PLAYER_STATE1_26) ||
                (this->csMode != 0) || (this->meleeWeaponQuads[0].base.atFlags & AT_HIT) ||
                (this->meleeWeaponQuads[1].base.atFlags & AT_HIT)
            ) {
                return 0;
            }
            
            if (this->cylinder.base.acFlags & AC_HIT) {
                Actor* ac = this->cylinder.base.ac;
                s32 sp4C;
                
                if (ac->flags & ACTOR_FLAG_24) {
                    func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
                }
                
                if (this->stateFlags1 & PLAYER_STATE1_27) {
                    sp4C = 0;
                } else if (this->actor.colChkInfo.acHitEffect == 2) {
                    sp4C = 3;
                } else if (this->actor.colChkInfo.acHitEffect == 3) {
                    sp4C = 4;
                } else if (this->actor.colChkInfo.acHitEffect == 4) {
                    sp4C = 1;
                } else {
                    Player_PlayFallSfx(this);
                    sp4C = 0;
                }
                
                Player_SetupDamage(play, this, sp4C, 4.0f, 5.0f, Actor_WorldYawTowardActor(ac, &this->actor), 20);
            } else if (this->invincibilityTimer != 0) {
                return 0;
            } else {
                static u8 D_808544F4[] = { 120, 60 };
                s32 sp48 = Player_GetFloorDamageType(sFloorSpecialProperty);
                
                if (
                    ((this->actor.wallPoly != NULL) &&
                    func_80042108(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) ||
                    ((sp48 >= PLAYER_FLOODRAMAGE_SPIKE) &&
                    func_80042108(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) &&
                    (this->unk_A79 >= D_808544F4[sp48])) ||
                    ((sp48 >= PLAYER_FLOODRAMAGE_SPIKE) &&
                    ((this->currentTunic != PLAYER_TUNIC_GORON) || (this->unk_A79 >= D_808544F4[sp48])))
                ) {
                    this->unk_A79 = 0;
                    this->actor.colChkInfo.damage = 4;
                    Player_SetupDamage(play, this, 0, 4.0f, 5.0f, this->actor.shape.rot.y, 20);
                } else {
                    return 0;
                }
            }
        }
    }
    
    return 1;
}

void Player_SetupJumpWithSfx(Player* this, LinkAnimationHeader* anim, f32 arg2, PlayState* play, u16 sfxId) {
    Player_SetActionFunc(play, this, Player_UpdateMidair, 1);
    
    if (anim != NULL) {
        Player_PlayAnimOnceSlowed(play, this, anim);
    }
    
    this->actor.velocity.y = arg2 * sWaterSpeedScale;
    this->hoverBootsTimer = 0;
    this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    
    Player_PlayJumpSfx(this);
    Player_PlayVoiceSfxForAge(this, sfxId);
    
    this->stateFlags1 |= PLAYER_STATE1_18;
}

void Player_SetupJump(Player* this, LinkAnimationHeader* anim, f32 arg2, PlayState* play) {
    Player_SetupJumpWithSfx(this, anim, arg2, play, NA_SE_VO_LI_SWORD_N);
}

s32 Player_SetupWallJumpBehavior(Player* this, PlayState* play) {
    s32 sp3C;
    LinkAnimationHeader* sp38;
    f32 sp34;
    f32 temp;
    f32 wallPolyNormalX;
    f32 wallPolyNormalZ;
    f32 sp24;
    
    if (
        !(this->stateFlags1 & PLAYER_STATE1_11) && (this->unk_88C >= 2) &&
        (!(this->stateFlags1 & PLAYER_STATE1_27) || (this->ageProperties->unk_14 > this->wallHeight))
    ) {
        sp3C = 0;
        
        if (Player_IsSwimming(this)) {
            if (this->actor.yDistToWater < 50.0f) {
                if ((this->unk_88C < 2) || (this->wallHeight > this->ageProperties->unk_10)) {
                    return 0;
                }
            } else if ((this->currentBoots != PLAYER_BOOTS_IRON) || (this->unk_88C > 2)) {
                return 0;
            }
        } else if (
            !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
            ((this->ageProperties->unk_14 <= this->wallHeight) && (this->stateFlags1 & PLAYER_STATE1_27))
        ) {
            return 0;
        }
        
        if ((this->actor.wallBgId != BGCHECK_SCENE) && (sTouchedWallFlags & 0x40)) {
            if (this->unk_88D >= 6) {
                this->stateFlags2 |= PLAYER_STATE2_2;
                if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
                    sp3C = 1;
                }
            }
        } else if ((this->unk_88D >= 6) || CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
            sp3C = 1;
        }
        
        if (sp3C != 0) {
            Player_SetActionFunc(play, this, Player_JumpUpToLedge, 0);
            
            this->stateFlags1 |= PLAYER_STATE1_18;
            
            sp34 = this->wallHeight;
            
            if (this->ageProperties->unk_14 <= sp34) {
                sp38 = &gPlayerAnim_link_normal_250jump_start;
                this->linearVelocity = 1.0f;
            } else {
                wallPolyNormalX = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.x);
                wallPolyNormalZ = COLPOLY_GET_NORMAL(this->actor.wallPoly->normal.z);
                sp24 = this->wallDistance + 0.5f;
                
                this->stateFlags1 |= PLAYER_STATE1_14;
                
                if (Player_IsSwimming(this)) {
                    sp38 = &gPlayerAnim_link_swimer_swim_15step_up;
                    sp34 -= (60.0f * this->ageProperties->unk_08);
                    this->stateFlags1 &= ~PLAYER_STATE1_27;
                } else if (this->ageProperties->unk_18 <= sp34) {
                    sp38 = &gPlayerAnim_link_normal_150step_up;
                    sp34 -= (59.0f * this->ageProperties->unk_08);
                } else {
                    sp38 = &gPlayerAnim_link_normal_100step_up;
                    sp34 -= (41.0f * this->ageProperties->unk_08);
                }
                
                this->actor.shape.yOffset -= sp34 * 100.0f;
                
                this->actor.world.pos.x -= sp24 * wallPolyNormalX;
                this->actor.world.pos.y += this->wallHeight;
                this->actor.world.pos.z -= sp24 * wallPolyNormalZ;
                
                Player_ClearAttentionModeAndStopMoving(this);
            }
            
            this->actor.bgCheckFlags |= BGCHECKFLAG_GROUND;
            
            LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sp38, 1.3f);
            AnimationContext_DisableQueue(play);
            
            this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
            
            return 1;
        }
    } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->unk_88C == 1) && (this->unk_88D >= 3)) {
        temp = (this->wallHeight * 0.08f) + 5.5f;
        Player_SetupJump(this, &gPlayerAnim_link_normal_jump, temp, play);
        this->linearVelocity = 2.5f;
        
        return 1;
    }
    
    return 0;
}

void Player_SetupCutsceneMovement(PlayState* play, Player* this, f32 arg2, s16 arg3) {
    Player_SetActionFunc(play, this, Player_CutsceneMovement, 0);
    Player_ResetAttributes(play, this);
    
    this->unk_84F = 1;
    this->unk_850 = 1;
    
    this->unk_450.x = (Math_SinS(arg3) * arg2) + this->actor.world.pos.x;
    this->unk_450.z = (Math_CosS(arg3) * arg2) + this->actor.world.pos.z;
    
    Player_PlayAnimOnce(play, this, Player_GetAnim_StandingStill(this));
}

void Player_SetupSwimIdle(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_UpdateSwimIdle, 0);
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
}

void Player_SetupEnterGrotto(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_EnterGrotto, 0);
    
    this->stateFlags1 |= PLAYER_STATE1_29 | PLAYER_STATE1_31;
    
    Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_FREE0);
}

s32 Player_ShouldEnterGrotto(PlayState* play, Player* this) {
    if ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (this->stateFlags1 & PLAYER_STATE1_31)) {
        Player_SetupEnterGrotto(play, this);
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_S);
        func_800788CC(NA_SE_OC_SECRET_WARP_IN);
        
        return 1;
    }
    
    return 0;
}

/**
 * The actual entrances each "return entrance" value can map to.
 * This is used by scenes that are shared between locations, like child/adult Shooting Gallery or Great Fairy Fountains.
 *
 * This 1D array is split into groups of entrances.
 * The start of each group is indexed by `sReturnEntranceGroupIndices` values.
 * The resulting groups are then indexed by the spawn value.
 *
 * The spawn value (`PlayState.curSpawn`) is set to a different value depending on the entrance used to enter the
 * scene, which allows these dynamic "return entrances" to link back to the previous scene.
 *
 * Note: grottos and normal fairy fountains use `ENTR_RETURN_GROTTO`
 */
s16 sReturnEntranceGroupData[] = {
    // ENTR_RETURN_DAIYOUSEI_IZUMI
    /*  0 */ ENTR_SPOT16_4, // DMT from Magic Fairy Fountain
    /*  1 */ ENTR_SPOT17_3, // DMC from Double Defense Fairy Fountain
    /*  2 */ ENTR_SPOT15_2, // Hyrule Castle from Dins Fire Fairy Fountain
    
    // ENTR_RETURN_2
    /*  3 */ ENTR_SPOT01_9, // Kakariko from Potion Shop
    /*  4 */ ENTR_MARKET_DAY_5, // Market (child day) from Potion Shop
    
    // ENTR_RETURN_SHOP1
    /*  5 */ ENTR_SPOT01_3, // Kakariko from Bazaar
    /*  6 */ ENTR_MARKET_DAY_6, // Market (child day) from Bazaar
    
    // ENTR_RETURN_4
    /*  7 */ ENTR_SPOT01_11, // Kakariko from House of Skulltulas
    /*  8 */ ENTR_MARKET_ALLEY_2, // Back Alley (day) from Bombchu Shop
    
    // ENTR_RETURN_SYATEKIJYOU
    /*  9 */ ENTR_SPOT01_10, // Kakariko from Shooting Gallery
    /* 10 */ ENTR_MARKET_DAY_8, // Market (child day) from Shooting Gallery
    
    // ENTR_RETURN_YOUSEI_IZUMI_YOKO
    /* 11 */ ENTR_SPOT08_5, // Zoras Fountain from Farores Wind Fairy Fountain
    /* 12 */ ENTR_SPOT15_2, // Hyrule Castle from Dins Fire Fairy Fountain
    /* 13 */ ENTR_SPOT11_7, // Desert Colossus from Nayrus Love Fairy Fountain
};

/**
 * The values are indices into `sReturnEntranceGroupData` marking the start of each group
 */
u8 sReturnEntranceGroupIndices[] = {
    11, // ENTR_RETURN_YOUSEI_IZUMI_YOKO
    9, // ENTR_RETURN_SYATEKIJYOU
    3, // ENTR_RETURN_2
    5, // ENTR_RETURN_SHOP1
    7, // ENTR_RETURN_4
    0, // ENTR_RETURN_DAIYOUSEI_IZUMI
};

s32 Player_SetupExit(PlayState* play, Player* this, CollisionPoly* poly, u32 bgId) {
    s32 exitIndex;
    s32 temp;
    s32 sp34;
    f32 linearVel;
    s32 yaw;
    
    if (this->actor.category == ACTORCAT_PLAYER) {
        exitIndex = 0;
        
        if (
            !(this->stateFlags1 & PLAYER_STATE1_7) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
            (this->csMode == 0) && !(this->stateFlags1 & PLAYER_STATE1_0) &&
            (((poly != NULL) &&
            (exitIndex = SurfaceType_GetExitIndex(&play->colCtx, poly, bgId), exitIndex != 0)) ||
            (Player_IsFloorSinkingSand(sFloorSpecialProperty) && (this->unk_A7A == 12)))
        ) {
            sp34 = this->unk_A84 - (s32)this->actor.world.pos.y;
            
            if (
                !(this->stateFlags1 & (PLAYER_STATE1_23 | PLAYER_STATE1_27 | PLAYER_STATE1_29)) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (sp34 < 100) && (sFloorDistY > 100.0f)
            ) {
                return 0;
            }
            
            if (exitIndex == 0) {
                Play_TriggerVoidOut(play);
                Play_SetFadeOut(play);
            } else {
                // z64rom
                u16* setupList = (u16*)play->setupExitList;
                
                for (s32 i = 0; i < exitIndex - 1; i++) {
                    if ((*setupList & 0x8000) == 0x8000)
                        setupList += 2;
                    
                    else
                        setupList++;
                }
                
                gExitParam.nextEntranceIndex = setupList[0];
                
                if ((*setupList & 0x8000) == 0x8000) {
                    gExitParam.exit.upper = setupList[0];
                    gExitParam.exit.lower = setupList[1];
                }
                
                if (gExitParam.nextEntranceIndex <= ENTR_RETURN_GROTTO && gExitParam.nextEntranceIndex >= ENTR_RETURN_YOUSEI_IZUMI_YOKO) {
                    if (gExitParam.nextEntranceIndex == ENTR_RETURN_GROTTO) {
                        gSaveContext.respawnFlag = 2;
                        gExitParam.nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex;
                        play->transitionType = TRANS_TYPE_FADE_WHITE;
                        gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
                    } else if (gExitParam.nextEntranceIndex >= ENTR_RETURN_YOUSEI_IZUMI_YOKO) {
                        gExitParam.nextEntranceIndex =
                            sReturnEntranceGroupData[sReturnEntranceGroupIndices[gExitParam.nextEntranceIndex -
                                ENTR_RETURN_YOUSEI_IZUMI_YOKO] +
                                play->curSpawn];
                        Play_SetFadeOut(play);
                    }
                } else {
                    if (SurfaceType_GetFloorEffect(&play->colCtx, poly, bgId) == 2) {
                        gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = gExitParam.nextEntranceIndex;
                        gExitParam.respawn[RESPAWN_MODE_DOWN] = gExitParam.exit;
                        Play_TriggerVoidOut(play);
                        gSaveContext.respawnFlag = -2;
                    }
                    
                    gSaveContext.retainWeatherMode = true;
                    Play_SetFadeOut(play);
                }
                
                play->transitionTrigger = TRANS_TRIGGER_START;
            }
            
            if (
                !(this->stateFlags1 & (PLAYER_STATE1_23 | PLAYER_STATE1_29)) &&
                !(this->stateFlags2 & PLAYER_STATE2_18) && !Player_IsSwimming(this) &&
                (temp = SurfaceType_GetFloorType(&play->colCtx, poly, bgId), (temp != 10)) &&
                ((sp34 < 100) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))
            ) {
                if (temp == 11) {
                    func_800788CC(NA_SE_OC_SECRET_HOLE_OUT);
                    func_800F6964(5);
                    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
                    gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
                } else {
                    linearVel = this->linearVelocity;
                    
                    if (linearVel < 0.0f) {
                        this->actor.world.rot.y += 0x8000;
                        linearVel = -linearVel;
                    }
                    
                    if (linearVel > R_RUN_SPEED_LIMIT / 100.0f) {
                        gSaveContext.entranceSpeed = R_RUN_SPEED_LIMIT / 100.0f;
                    } else {
                        gSaveContext.entranceSpeed = linearVel;
                    }
                    
                    if (sConveyorSpeedIndex != 0) {
                        yaw = sConveyorYaw;
                    } else {
                        yaw = this->actor.world.rot.y;
                    }
                    Player_SetupCutsceneMovement(play, this, 400.0f, yaw);
                }
            } else {
                if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                    Player_StopMovement(this);
                }
            }
            
            this->stateFlags1 |= PLAYER_STATE1_0 | PLAYER_STATE1_29;
            
            Player_ChangeCameraSetting(play, CAM_SET_SCENE_TRANSITION);
            
            return 1;
        } else {
            if (play->transitionTrigger == TRANS_TRIGGER_OFF) {
                if (
                    (this->actor.world.pos.y < -4000.0f) ||
                    (((this->unk_A7A == 5) || (this->unk_A7A == 12)) &&
                    ((sFloorDistY < 100.0f) || (this->fallDistance > 400.0f) ||
                    ((play->sceneId != SCENE_HAKADAN) && (this->fallDistance > 200.0f)))) ||
                    ((play->sceneId == SCENE_GANON_FINAL) && (this->fallDistance > 320.0f))
                ) {
                    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                        if (this->unk_A7A == 5) {
                            Play_TriggerRespawn(play);
                        } else {
                            Play_TriggerVoidOut(play);
                        }
                        play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
                        func_80078884(NA_SE_OC_ABYSS);
                    } else {
                        Player_SetupEnterGrotto(play, this);
                        this->unk_850 = 9999;
                        if (this->unk_A7A == 5) {
                            this->unk_84F = -1;
                        } else {
                            this->unk_84F = 1;
                        }
                    }
                }
                
                this->unk_A84 = this->actor.world.pos.y;
            }
        }
    }
    
    return 0;
}

void Player_PosRelativeToPlayerYaw(Player* this, Vec3f* origin, Vec3f* pos, Vec3f* dest) {
    f32 cos = Math_CosS(this->actor.shape.rot.y);
    f32 sin = Math_SinS(this->actor.shape.rot.y);
    
    dest->x = origin->x + ((pos->x * cos) + (pos->z * sin));
    dest->y = origin->y + pos->y;
    dest->z = origin->z + ((pos->z * cos) - (pos->x * sin));
}

Actor* Player_SpawnFairy(PlayState* play, Player* this, Vec3f* arg2, Vec3f* arg3, s32 type) {
    Vec3f pos;
    
    Player_PosRelativeToPlayerYaw(this, arg2, arg3, &pos);
    
    return Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ELF, pos.x, pos.y, pos.z, 0, 0, 0, type);
}

f32 Player_RelativeFloorRaycast_PolyInfo(PlayState* play, Player* this, Vec3f* pos, Vec3f* result, CollisionPoly** poly, s32* bgId) {
    Player_PosRelativeToPlayerYaw(this, &this->actor.world.pos, pos, result);
    
    return BgCheck_EntityRaycastDown3(&play->colCtx, poly, bgId, result);
}

f32 Player_RelativeFloorRaycast(PlayState* play, Player* this, Vec3f* arg2, Vec3f* arg3) {
    CollisionPoly* sp24;
    s32 sp20;
    
    return Player_RelativeFloorRaycast_PolyInfo(play, this, arg2, arg3, &sp24, &sp20);
}

s32 Player_RelativeLineRaycast(PlayState* play, Player* this, Vec3f* pos, CollisionPoly** poly, s32* bgId, Vec3f* result) {
    Vec3f sp44;
    Vec3f sp38;
    
    sp44.x = this->actor.world.pos.x;
    sp44.y = this->actor.world.pos.y + pos->y;
    sp44.z = this->actor.world.pos.z;
    
    Player_PosRelativeToPlayerYaw(this, &this->actor.world.pos, pos, &sp38);
    
    return BgCheck_EntityLineTest1(&play->colCtx, &sp44, &sp38, result, poly, true, false, false, true, bgId);
}

s32 Player_SetupOpenDoor(Player* this, PlayState* play) {
    DoorShutter* doorShutter;
    EnDoor* door; // Can also be DoorKiller*
    s32 doorDirection;
    f32 sp78;
    f32 sp74;
    Actor* doorActor;
    f32 sp6C;
    s32 frontRoom;
    Actor* attachedActor;
    LinkAnimationHeader* sp5C;
    CollisionPoly* sp58;
    Vec3f sp4C;
    
    if (
        (this->doorType != PLAYER_DOORTYPE_NONE) &&
        (!(this->stateFlags1 & PLAYER_STATE1_11) ||
        ((this->heldActor != NULL) && (this->heldActor->id == ACTOR_EN_RU1)))
    ) {
        if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) || (Player_SetupOpenDoorFromSpawn == this->func_674)) {
            doorActor = this->doorActor;
            
            if (this->doorType <= PLAYER_DOORTYPE_AJAR) {
                doorActor->textId = 0xD0;
                Player_StartTalkingWithActor(play, doorActor);
                
                return 0;
            }
            
            doorDirection = this->doorDirection;
            sp78 = Math_CosS(doorActor->shape.rot.y);
            sp74 = Math_SinS(doorActor->shape.rot.y);
            
            if (this->doorType == PLAYER_DOORTYPE_SLIDING) {
                doorShutter = (DoorShutter*)doorActor;
                
                this->currentYaw = doorShutter->dyna.actor.home.rot.y;
                if (doorDirection > 0) {
                    this->currentYaw -= 0x8000;
                }
                this->actor.shape.rot.y = this->currentYaw;
                
                if (this->linearVelocity <= 0.0f) {
                    this->linearVelocity = 0.1f;
                }
                
                Player_SetupCutsceneMovement(play, this, 50.0f, this->actor.shape.rot.y);
                
                this->unk_84F = 0;
                this->unk_447 = this->doorType;
                this->stateFlags1 |= PLAYER_STATE1_29;
                
                this->unk_450.x = this->actor.world.pos.x + ((doorDirection * 20.0f) * sp74);
                this->unk_450.z = this->actor.world.pos.z + ((doorDirection * 20.0f) * sp78);
                this->unk_45C.x = this->actor.world.pos.x + ((doorDirection * -120.0f) * sp74);
                this->unk_45C.z = this->actor.world.pos.z + ((doorDirection * -120.0f) * sp78);
                
                doorShutter->unk_164 = 1;
                Player_ClearAttentionModeAndStopMoving(this);
                
                if (this->doorTimer != 0) {
                    this->unk_850 = 0;
                    Player_ChangeAnimMorphToLastFrame(play, this, Player_GetAnim_StandingStill(this));
                    this->skelAnime.endFrame = 0.0f;
                } else {
                    this->linearVelocity = 0.1f;
                }
                
                if (doorShutter->dyna.actor.category == ACTORCAT_DOOR) {
                    this->doorBgCamIndex = play->transiActorCtx.list[(u16)doorShutter->dyna.actor.params >> 10]
                        .sides[(doorDirection > 0) ? 0 : 1]
                        .bgCamIndex;
                    
                    Actor_DisableLens(play);
                }
            } else {
                // This actor can be either EnDoor or DoorKiller.
                // Don't try to access any struct vars other than `animStyle` and `playerIsOpening`! These two variables
                // are common across the two actors' structs however most other variables are not!
                door = (EnDoor*)doorActor;
                
                door->openAnim = (doorDirection < 0.0f) ? (LINK_IS_ADULT ? KNOB_ANIM_ADULT_L : KNOB_ANIM_CHILD_L)
                             : (LINK_IS_ADULT ? KNOB_ANIM_ADULT_R : KNOB_ANIM_CHILD_R);
                
                if (door->openAnim == KNOB_ANIM_ADULT_L) {
                    sp5C = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_9, this->modelAnimType);
                } else if (door->openAnim == KNOB_ANIM_CHILD_L) {
                    sp5C = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_10, this->modelAnimType);
                } else if (door->openAnim == KNOB_ANIM_ADULT_R) {
                    sp5C = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_11, this->modelAnimType);
                } else {
                    sp5C = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_12, this->modelAnimType);
                }
                
                Player_SetActionFunc(play, this, Player_OpenDoor, 0);
                Player_UnequipItem(play, this);
                
                if (doorDirection < 0) {
                    this->actor.shape.rot.y = doorActor->shape.rot.y;
                } else {
                    this->actor.shape.rot.y = doorActor->shape.rot.y - 0x8000;
                }
                
                this->currentYaw = this->actor.shape.rot.y;
                
                sp6C = (doorDirection * 22.0f);
                this->actor.world.pos.x = doorActor->world.pos.x + sp6C * sp74;
                this->actor.world.pos.z = doorActor->world.pos.z + sp6C * sp78;
                
                Player_PlayAnimOnce_WaterSpeedScale(play, this, sp5C);
                
                if (this->doorTimer != 0) {
                    this->skelAnime.endFrame = 0.0f;
                }
                
                Player_ClearAttentionModeAndStopMoving(this);
                Player_SetupAnimMovement(play, this, 0x28F);
                
                if (doorActor->parent != NULL) {
                    doorDirection = -doorDirection;
                }
                
                door->playerIsOpening = 1;
                
                if (this->doorType != PLAYER_DOORTYPE_FAKE) {
                    this->stateFlags1 |= PLAYER_STATE1_29;
                    Actor_DisableLens(play);
                    
                    if (((doorActor->params >> 7) & 7) == 3) {
                        sp4C.x = doorActor->world.pos.x - (sp6C * sp74);
                        sp4C.y = doorActor->world.pos.y + 10.0f;
                        sp4C.z = doorActor->world.pos.z - (sp6C * sp78);
                        
                        BgCheck_EntityRaycastDown1(&play->colCtx, &sp58, &sp4C);
                        
                        if (Player_SetupExit(play, this, sp58, BGCHECK_SCENE)) {
                            gSaveContext.entranceSpeed = 2.0f;
                            gSaveContext.entranceSound = NA_SE_OC_DOOR_OPEN;
                        }
                    } else {
                        Camera_ChangeDoorCam(
                            Play_GetCamera(play, CAM_ID_MAIN),
                            doorActor,
                            play->transiActorCtx.list[(u16)doorActor->params >> 10]
                            .sides[(doorDirection > 0) ? 0 : 1]
                            .bgCamIndex,
                            0,
                            38.0f * sWaterSpeedInvScale,
                            26.0f * sWaterSpeedInvScale,
                            10.0f * sWaterSpeedInvScale
                        );
                    }
                }
            }
            
            if ((this->doorType != PLAYER_DOORTYPE_FAKE) && (doorActor->category == ACTORCAT_DOOR)) {
                frontRoom =
                    play->transiActorCtx.list[(u16)doorActor->params >> 10].sides[(doorDirection > 0) ? 0 : 1].room;
                
                if ((frontRoom >= 0) && (frontRoom != play->roomCtx.curRoom.num)) {
                    func_8009728C(play, &play->roomCtx, frontRoom);
                }
            }
            
            doorActor->room = play->roomCtx.curRoom.num;
            
            if (((attachedActor = doorActor->child) != NULL) || ((attachedActor = doorActor->parent) != NULL)) {
                attachedActor->room = play->roomCtx.curRoom.num;
            }
            
            return 1;
        }
    }
    
    return 0;
}

void Player_SetupUnfriendlyZTargetStandStill(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    
    Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);
    
    if (this->unk_870 < 0.5f) {
        anim = Player_GetAnim_FightRight(this);
        this->unk_870 = 0.0f;
    } else {
        anim = Player_GetAnim_FightLeft(this);
        this->unk_870 = 1.0f;
    }
    
    this->unk_874 = this->unk_870;
    Player_PlayAnimLoop(play, this, anim);
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupFriendlyZTargetingStandStill(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_FriendlyZTargetStandingStill, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, Player_GetAnim_StandingStill(this));
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupStandingStillType(Player* this, PlayState* play) {
    if (func_8008E9C4(this)) {
        Player_SetupUnfriendlyZTargetStandStill(this, play);
    } else if (Player_IsFriendlyZTargeting(this)) {
        Player_SetupFriendlyZTargetingStandStill(this, play);
    } else {
        Player_SetupStandingStillMorph(this, play);
    }
}

void Player_ReturnToStandStill(Player* this, PlayState* play) {
    PlayerActionFunc func;
    
    if (func_8008E9C4(this)) {
        func = Player_UnfriendlyZTargetStandingStill;
    } else if (Player_IsFriendlyZTargeting(this)) {
        func = Player_FriendlyZTargetStandingStill;
    } else {
        func = Player_StandingStill;
    }
    
    Player_SetActionFunc(play, this, func, 1);
}

void Player_SetupReturnToStandStill(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    if (func_8008E9C4(this)) {
        this->unk_850 = 1;
    }
}

void Player_SetupReturnToStandStillSetAnim(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetupReturnToStandStill(this, play);
    Player_PlayAnimOnce_WaterSpeedScale(play, this, anim);
}

s32 Player_CanHoldActor(Player* this) {
    return (this->interactRangeActor != NULL) && (this->heldActor == NULL);
}

void Player_SetupHoldActor(PlayState* play, Player* this) {
    if (Player_CanHoldActor(this)) {
        Actor* interactRangeActor = this->interactRangeActor;
        s32 interactActorId = interactRangeActor->id;
        
        if (interactActorId == ACTOR_BG_TOKI_SWD) {
            this->interactRangeActor->parent = &this->actor;
            Player_SetActionFunc(play, this, Player_SetDrawAndStartCutsceneAfterTimer, 0);
            this->stateFlags1 |= PLAYER_STATE1_29;
        } else {
            LinkAnimationHeader* anim;
            
            if (interactActorId == ACTOR_BG_HEAVY_BLOCK) {
                Player_SetActionFunc(play, this, Player_ThrowStonePillar, 0);
                this->stateFlags1 |= PLAYER_STATE1_29;
                anim = &gPlayerAnim_link_normal_heavy_carry;
            } else if ((interactActorId == ACTOR_EN_ISHI) && ((interactRangeActor->params & 0xF) == 1)) {
                Player_SetActionFunc(play, this, Player_LiftSilverBoulder, 0);
                anim = &gPlayerAnim_link_silver_carry;
            } else if (
                ((interactActorId == ACTOR_EN_BOMBF) || (interactActorId == ACTOR_EN_KUSA)) &&
                (Player_GetStrength() <= PLAYER_STR_NONE)
            ) {
                Player_SetActionFunc(play, this, Player_FailToLiftActor, 0);
                this->actor.world.pos.x =
                    (Math_SinS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.x;
                this->actor.world.pos.z =
                    (Math_CosS(interactRangeActor->yawTowardsPlayer) * 20.0f) + interactRangeActor->world.pos.z;
                this->currentYaw = this->actor.shape.rot.y = interactRangeActor->yawTowardsPlayer + 0x8000;
                anim = &gPlayerAnim_link_normal_nocarry_free;
            } else {
                Player_SetActionFunc(play, this, Player_LiftActor, 0);
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_13, this->modelAnimType);
            }
            
            Player_PlayAnimOnce(play, this, anim);
        }
    } else {
        Player_SetupStandingStillType(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_11;
    }
}

void Player_SetupTalkWithActor(PlayState* play, Player* this) {
    Player_SetActionFunc_KeepMoveFlags(play, this, Player_TalkWithActor, 0);
    
    this->stateFlags1 |= PLAYER_STATE1_6 | PLAYER_STATE1_29;
    
    if (this->actor.textId != 0) {
        Message_StartTextbox(play, this->actor.textId, this->targetActor);
        this->unk_664 = this->targetActor;
    }
}

void Player_SetupRideHorse(PlayState* play, Player* this) {
    Player_SetActionFunc_KeepMoveFlags(play, this, Player_RideHorse, 0);
}

void Player_SetupGrabPullableObject(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_GrabPushPullWall, 0);
}

void Player_SetupClimbingWallOrDownLedge(PlayState* play, Player* this) {
    s32 sp1C = this->unk_850;
    s32 sp18 = this->unk_84F;
    
    Player_SetActionFunc_KeepMoveFlags(play, this, Player_ClimbingWallOrDownLedge, 0);
    this->actor.velocity.y = 0.0f;
    
    this->unk_850 = sp1C;
    this->unk_84F = sp18;
}

void Player_SetupInsideCrawlspace(PlayState* play, Player* this) {
    Player_SetActionFunc_KeepMoveFlags(play, this, Player_InsideCrawlspace, 0);
}

void Player_SetupGetItem(PlayState* play, Player* this) {
    Player_SetActionFunc_KeepMoveFlags(play, this, Player_GetItem, 0);
    
    this->stateFlags1 |= PLAYER_STATE1_10 | PLAYER_STATE1_29;
    
    if (this->getItemId == GI_HEART_CONTAINER_2) {
        this->unk_850 = 20;
    } else if (this->getItemId >= 0) {
        this->unk_850 = 1;
    } else {
        this->getItemId = -this->getItemId;
    }
}

s32 Player_StartJump(Player* this, PlayState* play) {
    s16 yawDiff;
    LinkAnimationHeader* anim;
    f32 temp;
    
    yawDiff = this->currentYaw - this->actor.shape.rot.y;
    
    if ((ABS(yawDiff) < 0x1000) && (this->linearVelocity > 4.0f)) {
        anim = &gPlayerAnim_link_normal_run_jump;
    } else {
        anim = &gPlayerAnim_link_normal_jump;
    }
    
    if (this->linearVelocity > (IREG(66) / 100.0f)) {
        temp = IREG(67) / 100.0f;
    } else {
        temp = (IREG(68) / 100.0f) + ((IREG(69) * this->linearVelocity) / 1000.0f);
    }
    
    Player_SetupJumpWithSfx(this, anim, temp, play, NA_SE_VO_LI_AUTO_JUMP);
    this->unk_850 = 1;
    
    return 1;
}

void Player_SetupGrabLedge(PlayState* play, Player* this, CollisionPoly* arg2, f32 arg3, LinkAnimationHeader* arg4) {
    f32 nx = COLPOLY_GET_NORMAL(arg2->normal.x);
    f32 nz = COLPOLY_GET_NORMAL(arg2->normal.z);
    
    Player_SetActionFunc(play, this, Player_GrabLedge, 0);
    Player_ResetAttributesAndHeldActor(play, this);
    Player_PlayAnimOnce(play, this, arg4);
    
    this->actor.world.pos.x -= (arg3 + 1.0f) * nx;
    this->actor.world.pos.z -= (arg3 + 1.0f) * nz;
    this->actor.shape.rot.y = this->currentYaw = Math_Atan2S(nz, nx);
    
    Player_ClearAttentionModeAndStopMoving(this);
    Player_AnimUpdatePrevTranslRot(this);
}

s32 Player_SetupGrabLedgeInsteadOfFalling(Player* this, PlayState* play) {
    CollisionPoly* sp84;
    s32 sp80;
    Vec3f sp74;
    Vec3f sp68;
    f32 temp1;
    
    if ((this->actor.yDistToWater < -80.0f) && (ABS(this->unk_898) < 2730) && (ABS(this->unk_89A) < 2730)) {
        sp74.x = this->actor.prevPos.x - this->actor.world.pos.x;
        sp74.z = this->actor.prevPos.z - this->actor.world.pos.z;
        
        temp1 = sqrtf(SQ(sp74.x) + SQ(sp74.z));
        if (temp1 != 0.0f) {
            temp1 = 5.0f / temp1;
        } else {
            temp1 = 0.0f;
        }
        
        sp74.x = this->actor.prevPos.x + (sp74.x * temp1);
        sp74.y = this->actor.world.pos.y;
        sp74.z = this->actor.prevPos.z + (sp74.z * temp1);
        
        if (
            BgCheck_EntityLineTest1(
                &play->colCtx,
                &this->actor.world.pos,
                &sp74,
                &sp68,
                &sp84,
                true,
                false,
                false,
                true,
                &sp80
            ) &&
            (ABS(sp84->normal.y) < 600)
        ) {
            f32 nx = COLPOLY_GET_NORMAL(sp84->normal.x);
            f32 ny = COLPOLY_GET_NORMAL(sp84->normal.y);
            f32 nz = COLPOLY_GET_NORMAL(sp84->normal.z);
            f32 sp54;
            s32 sp50;
            
            sp54 = Math3D_UDistPlaneToPos(nx, ny, nz, sp84->dist, &this->actor.world.pos);
            
            sp50 = sFloorProperty == 6;
            if (!sp50 && (SurfaceType_GetWallFlags(&play->colCtx, sp84, sp80) & 8)) {
                sp50 = 1;
            }
            
            Player_SetupGrabLedge(play, this, sp84, sp54, sp50 ? &gPlayerAnim_link_normal_Fclimb_startB : &gPlayerAnim_link_normal_fall);
            
            if (sp50) {
                Player_SetupCsActionFunc(play, this, Player_SetupClimbingWallOrDownLedge);
                
                this->currentYaw += 0x8000;
                this->actor.shape.rot.y = this->currentYaw;
                
                this->stateFlags1 |= PLAYER_STATE1_21;
                Player_SetupAnimMovement(play, this, 0x9F);
                
                this->unk_850 = -1;
                this->unk_84F = sp50;
            } else {
                this->stateFlags1 |= PLAYER_STATE1_13;
                this->stateFlags1 &= ~PLAYER_STATE1_17;
            }
            
            func_8002F7DC(&this->actor, NA_SE_PL_SLIPDOWN);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HANG);
            
            return 1;
        }
    }
    
    return 0;
}

void Player_SetupClimbOntoLedge(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetActionFunc(play, this, Player_ClimbOntoLedge, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, 1.3f);
}

void Player_SetupMidairBehavior(Player* this, PlayState* play) {
    static Vec3f sWaterRaycastPos = { 0.0f, 0.0f, 100.0f };
    s32 sp5C;
    CollisionPoly* sp58;
    s32 sp54;
    WaterBox* sp50;
    Vec3f sp44;
    f32 sp40;
    f32 sp3C;
    
    this->fallDistance = this->fallStartHeight - (s32)this->actor.world.pos.y;
    
    if (
        !(this->stateFlags1 & (PLAYER_STATE1_27 | PLAYER_STATE1_29)) &&
        !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
    ) {
        if (!Player_ShouldEnterGrotto(play, this)) {
            if (sFloorProperty == 8) {
                this->actor.world.pos.x = this->actor.prevPos.x;
                this->actor.world.pos.z = this->actor.prevPos.z;
                
                return;
            }
            
            if (
                !(this->stateFlags3 & PLAYER_STATE3_1) && !(this->skelAnime.moveFlags & 0x80) &&
                (Player_UpdateMidair != this->func_674) && (Player_FallingDive != this->func_674)
            ) {
                if ((sFloorProperty == 7) || (this->meleeWeaponState != 0)) {
                    Math_Vec3f_Copy(&this->actor.world.pos, &this->actor.prevPos);
                    Player_StopMovement(this);
                    
                    return;
                }
                
                if (this->hoverBootsTimer != 0) {
                    this->actor.velocity.y = 1.0f;
                    sFloorProperty = 9;
                    
                    return;
                }
                
                sp5C = (s16)(this->currentYaw - this->actor.shape.rot.y);
                
                Player_SetActionFunc(play, this, Player_UpdateMidair, 1);
                Player_ResetAttributes(play, this);
                
                this->unk_89E = this->unk_A82;
                
                if (
                    (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_LEAVE) && !(this->stateFlags1 & PLAYER_STATE1_27) &&
                    (sFloorProperty != 6) && (sFloorProperty != 9) && (sFloorDistY > 20.0f) && (this->meleeWeaponState == 0) &&
                    (ABS(sp5C) < 0x2000) && (this->linearVelocity > 3.0f)
                ) {
                    if ((sFloorProperty == 11) && !(this->stateFlags1 & PLAYER_STATE1_11)) {
                        sp40 = Player_RelativeFloorRaycast_PolyInfo(play, this, &sWaterRaycastPos, &sp44, &sp58, &sp54);
                        sp3C = this->actor.world.pos.y;
                        
                        if (
                            WaterBox_GetSurface1(play, &play->colCtx, sp44.x, sp44.z, &sp3C, &sp50) &&
                            ((sp3C - sp40) > 50.0f)
                        ) {
                            Player_SetupJump(this, &gPlayerAnim_link_normal_run_jump_water_fall, 6.0f, play);
                            Player_SetActionFunc(play, this, Player_FallingDive, 0);
                            
                            return;
                        }
                    }
                    
                    Player_StartJump(this, play);
                    
                    return;
                }
                
                if ((sFloorProperty == 9) || (sFloorDistY <= this->ageProperties->unk_34) || !Player_SetupGrabLedgeInsteadOfFalling(this, play)) {
                    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_landing_wait);
                    
                    return;
                }
            }
        }
    } else {
        this->fallStartHeight = this->actor.world.pos.y;
    }
}

s32 Player_SetupCameraMode(PlayState* play, Player* this) {
    s32 cameraMode;
    
    if (this->unk_6AD == 2) {
        if (func_8002DD6C(this)) {
            if (LINK_IS_ADULT) {
                cameraMode = CAM_MODE_BOWARROW;
            } else {
                cameraMode = CAM_MODE_SLINGSHOT;
            }
        } else {
            cameraMode = CAM_MODE_BOOMERANG;
        }
    } else {
        cameraMode = CAM_MODE_FIRSTPERSON;
    }
    
    return Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), cameraMode);
}

s32 Player_SetupCutscene(PlayState* play, Player* this) {
    if (this->unk_6AD == 3) {
        Player_SetActionFunc(play, this, Player_StartCutscene, 0);
        if (this->doorBgCamIndex != 0) {
            this->stateFlags1 |= PLAYER_STATE1_29;
        }
        Player_InactivateMeleeWeapon(this);
        
        return 1;
    } else {
        return 0;
    }
}

void Player_LoadGetItemObject(Player* this, s16 objectId) {
    u32 size;
    
    if (objectId != OBJECT_INVALID) {
        this->giObjectLoading = true;
        osCreateMesgQueue(&this->giObjectLoadQueue, &this->giObjectLoadMsg, 1);
        
        size = gObjectTable[objectId].vromEnd - gObjectTable[objectId].vromStart;
        
        LOG_HEX("size", size, "../z_player.c", 9090);
        ASSERT(size <= 1024 * 8, "size <= 1024 * 8", "../z_player.c", 9091);
        
        DmaMgr_SendRequest2(
            &this->giObjectDmaRequest,
            (u32)this->giObjectSegment,
            gObjectTable[objectId].vromStart,
            size,
            0,
            &this->giObjectLoadQueue,
            NULL,
            "../z_player.c",
            9099
        );
    }
}

void Player_SetupMagicSpell(PlayState* play, Player* this, s32 magicSpell) {
    Player_SetActionFunc_KeepItemAP(play, this, Player_UpdateMagicSpell, 0);
    
    this->unk_84F = magicSpell - 3;
    Magic_RequestChange(play, sMagicSpellCosts[magicSpell], MAGIC_CONSUME_WAIT_PREVIEW);
    
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_link_magic_tame, 0.83f);
    
    if (magicSpell == 5) {
        this->subCamId = OnePointCutscene_Init(play, 1100, -101, NULL, CAM_ID_MAIN);
    } else {
        Player_SetCameraTurnAround(play, 10);
    }
}

void Player_ResetLookAngles(Player* this) {
    this->actor.focus.rot.x = this->actor.focus.rot.z = this->unk_6B6 = this->unk_6B8 = this->unk_6BA = this->unk_6BC =
        this->unk_6BE = this->unk_6C0 = 0;
    
    this->actor.focus.rot.y = this->actor.shape.rot.y;
}

static u8 sGetItemID_ExchangeItem[] = {
    GI_LETTER_ZELDA, GI_WEIRD_EGG,          GI_CHICKEN,               GI_BEAN,            GI_POCKET_EGG,           GI_POCKET_CUCCO,
    GI_COJIRO,       GI_ODD_MUSHROOM,       GI_ODD_POTION,            GI_SAW,             GI_SWORD_BROKEN,         GI_PRESCRIPTION,
    GI_FROG,         GI_EYEDROPS,           GI_CLAIM_CHECK,           GI_MASK_SKULL,      GI_MASK_SPOOKY,          GI_MASK_KEATON,
    GI_MASK_BUNNY,   GI_MASK_TRUTH,         GI_MASK_GORON,            GI_MASK_ZORA,       GI_MASK_GERUDO,          GI_LETTER_RUTO,
    GI_LETTER_RUTO,  GI_LETTER_RUTO,        GI_LETTER_RUTO,           GI_LETTER_RUTO,     GI_LETTER_RUTO,
};

static LinkAnimationHeader* sAnims_ExchangeItem[] = {
    &gPlayerAnim_link_normal_give_other,
    &gPlayerAnim_link_bottle_read,
    &gPlayerAnim_link_normal_take_out,
};

s32 Player_SetupItemCsOrFirstPerson(Player* this, PlayState* play) {
    s32 sp2C;
    s32 sp28;
    GetItemEntry* giEntry;
    Actor* targetActor;
    
    if (
        (this->unk_6AD != 0) && (Player_IsSwimming(this) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
        (this->stateFlags1 & PLAYER_STATE1_23))
    ) {
        if (!Player_SetupCutscene(play, this)) {
            if (this->unk_6AD == 4) {
                sp2C = Player_ActionToMagicSpell(this, this->itemActionParam);
                if (sp2C >= 0) {
                    if ((sp2C != 3) || (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
                        Player_SetupMagicSpell(play, this, sp2C);
                    } else {
                        Player_SetActionFunc(play, this, Player_ChooseFaroresWindOption, 1);
                        this->stateFlags1 |= PLAYER_STATE1_28 | PLAYER_STATE1_29;
                        Player_PlayAnimOnce(play, this, Player_GetAnim_StandingStill(this));
                        Player_SetCameraTurnAround(play, 4);
                    }
                    
                    Player_ClearAttentionModeAndStopMoving(this);
                    
                    return 1;
                }
                
                sp2C = this->itemActionParam - PLAYER_AP_LETTER_ZELDA;
                if (
                    (sp2C >= 0) ||
                    (sp28 = Player_ActionToBottle(this, this->itemActionParam) - 1,
                    ((sp28 >= 0) && (sp28 < 6) &&
                    ((this->itemActionParam > PLAYER_AP_BOTTLE_POE) ||
                    ((this->targetActor != NULL) &&
                    (((this->itemActionParam == PLAYER_AP_BOTTLE_POE) && (this->exchangeItemId == EXCH_ITEM_POE)) ||
                    (this->exchangeItemId == EXCH_ITEM_BLUE_FIRE))))))
                ) {
                    if ((play->actorCtx.titleCtx.delayTimer == 0) && (play->actorCtx.titleCtx.alpha == 0)) {
                        Player_SetActionFunc_KeepItemAP(play, this, Player_PresentExchangeItem, 0);
                        
                        if (sp2C >= 0) {
                            giEntry = &sGetItemTable[sGetItemID_ExchangeItem[sp2C] - 1];
                            Player_LoadGetItemObject(this, giEntry->objectId);
                        }
                        
                        this->stateFlags1 |= PLAYER_STATE1_6 | PLAYER_STATE1_28 | PLAYER_STATE1_29;
                        
                        if (sp2C >= 0) {
                            sp2C = sp2C + 1;
                        } else {
                            sp2C = sp28 + 0x18;
                        }
                        
                        targetActor = this->targetActor;
                        
                        if (
                            (targetActor != NULL) &&
                            ((this->exchangeItemId == sp2C) || (this->exchangeItemId == EXCH_ITEM_BLUE_FIRE) ||
                            ((this->exchangeItemId == EXCH_ITEM_POE) &&
                            (this->itemActionParam == PLAYER_AP_BOTTLE_BIG_POE)) ||
                            ((this->exchangeItemId == EXCH_ITEM_BEAN) &&
                            (this->itemActionParam == PLAYER_AP_BOTTLE_BUG))) &&
                            ((this->exchangeItemId != EXCH_ITEM_BEAN) || (this->itemActionParam == PLAYER_AP_BEAN))
                        ) {
                            if (this->exchangeItemId == EXCH_ITEM_BEAN) {
                                Inventory_ChangeAmmo(ITEM_BEAN, -1);
                                Player_SetActionFunc_KeepItemAP(play, this, func_8084279C, 0);
                                this->stateFlags1 |= PLAYER_STATE1_29;
                                this->unk_850 = 0x50;
                                this->unk_84F = -1;
                            }
                            targetActor->flags |= ACTOR_FLAG_8;
                            this->unk_664 = this->targetActor;
                        } else if (sp2C == EXCH_ITEM_LETTER_RUTO) {
                            this->unk_84F = 1;
                            this->actor.textId = 0x4005;
                            Player_SetCameraTurnAround(play, 1);
                        } else {
                            this->unk_84F = 2;
                            this->actor.textId = 0xCF;
                            Player_SetCameraTurnAround(play, 4);
                        }
                        
                        this->actor.flags |= ACTOR_FLAG_8;
                        this->exchangeItemId = sp2C;
                        
                        if (this->unk_84F < 0) {
                            Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_32, this->modelAnimType));
                        } else {
                            Player_PlayAnimOnce(play, this, sAnims_ExchangeItem[this->unk_84F]);
                        }
                        
                        Player_ClearAttentionModeAndStopMoving(this);
                    }
                    
                    return 1;
                }
                
                sp2C = Player_ActionToBottle(this, this->itemActionParam);
                if (sp2C >= 0) {
                    if (sp2C == 0xC) {
                        Player_SetActionFunc_KeepItemAP(play, this, Player_HealWithFairy, 0);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_bottle_bug_out);
                        Player_SetCameraTurnAround(play, 3);
                    } else if ((sp2C > 0) && (sp2C < 4)) {
                        Player_SetActionFunc_KeepItemAP(play, this, Player_DropItemFromBottle, 0);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_bottle_fish_out);
                        Player_SetCameraTurnAround(play, (sp2C == 1) ? 1 : 5);
                    } else {
                        Player_SetActionFunc_KeepItemAP(play, this, Player_DrinkFromBottle, 0);
                        Player_ChangeAnimSlowedMorphToLastFrame(play, this, &gPlayerAnim_link_bottle_drink_demo_start);
                        Player_SetCameraTurnAround(play, 2);
                    }
                } else {
                    Player_SetActionFunc_KeepItemAP(play, this, Player_PlayOcarina, 0);
                    Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_normal_okarina_start);
                    this->stateFlags2 |= PLAYER_STATE2_27;
                    Player_SetCameraTurnAround(play, (this->unk_6A8 != NULL) ? 0x5B : 0x5A);
                    if (this->unk_6A8 != NULL) {
                        this->stateFlags2 |= PLAYER_STATE2_25;
                        Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, this->unk_6A8);
                    }
                }
            } else if (Player_SetupCameraMode(play, this)) {
                if (!(this->stateFlags1 & PLAYER_STATE1_23)) {
                    Player_SetActionFunc(play, this, Player_FirstPersonAiming, 1);
                    this->unk_850 = 13;
                    Player_ResetLookAngles(this);
                }
                this->stateFlags1 |= PLAYER_STATE1_20;
                func_80078884(NA_SE_SY_CAMERA_ZOOM_UP);
                Player_StopMovement(this);
                
                return 1;
            } else {
                this->unk_6AD = 0;
                func_80078884(NA_SE_SY_ERROR);
                
                return 0;
            }
            
            this->stateFlags1 |= PLAYER_STATE1_28 | PLAYER_STATE1_29;
        }
        
        Player_ClearAttentionModeAndStopMoving(this);
        
        return 1;
    }
    
    return 0;
}

s32 Player_SetupSpeakOrCheck(Player* this, PlayState* play) {
    Actor* sp34 = this->targetActor;
    Actor* sp30 = this->unk_664;
    Actor* sp2C = NULL;
    s32 sp28 = 0;
    s32 sp24;
    
    sp24 = (sp30 != NULL) &&
        (CHECK_FLAG_ALL(sp30->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) || (sp30->naviEnemyId != NAVI_ENEMY_NONE));
    
    if (sp24 || (this->naviTextId != 0)) {
        sp28 = (this->naviTextId < 0) && ((ABS(this->naviTextId) & 0xFF00) != 0x200);
        if (sp28 || !sp24) {
            sp2C = this->naviActor;
            if (sp28) {
                sp30 = NULL;
                sp34 = NULL;
            }
        } else {
            sp2C = sp30;
        }
    }
    
    if ((sp34 != NULL) || (sp2C != NULL)) {
        if ((sp30 == NULL) || (sp30 == sp34) || (sp30 == sp2C)) {
            if (
                !(this->stateFlags1 & PLAYER_STATE1_11) ||
                ((this->heldActor != NULL) && (sp28 || (sp34 == this->heldActor) || (sp2C == this->heldActor) ||
                ((sp34 != NULL) && (sp34->flags & ACTOR_FLAG_16))))
            ) {
                if (
                    (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (this->stateFlags1 & PLAYER_STATE1_23) ||
                    (Player_IsSwimming(this) && !(this->stateFlags2 & PLAYER_STATE2_10))
                ) {
                    if (sp34 != NULL) {
                        this->stateFlags2 |= PLAYER_STATE2_1;
                        if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A) || (sp34->flags & ACTOR_FLAG_16)) {
                            sp2C = NULL;
                        } else if (sp2C == NULL) {
                            return 0;
                        }
                    }
                    
                    if (sp2C != NULL) {
                        if (!sp28) {
                            this->stateFlags2 |= PLAYER_STATE2_21;
                        }
                        
                        if (!CHECK_BTN_ALL(sControlInput->press.button, BTN_CUP) && !sp28) {
                            return 0;
                        }
                        
                        sp34 = sp2C;
                        this->targetActor = NULL;
                        
                        if (sp28 || !sp24) {
                            if (this->naviTextId >= 0) {
                                sp2C->textId = this->naviTextId;
                            } else {
                                sp2C->textId = -this->naviTextId;
                            }
                        } else {
                            if (sp2C->naviEnemyId != NAVI_ENEMY_NONE) {
                                sp2C->textId = sp2C->naviEnemyId + 0x600;
                            }
                        }
                    }
                    
                    this->currentMask = sCurrentMask;
                    Player_StartTalkingWithActor(play, sp34);
                    
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

s32 Player_ForceFirstPerson(Player* this, PlayState* play) {
    if (
        !(this->stateFlags1 & (PLAYER_STATE1_11 | PLAYER_STATE1_23)) &&
        Camera_CheckValidMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_FIRSTPERSON)
    ) {
        if (
            (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
            (Player_IsSwimming(this) && (this->actor.yDistToWater < this->ageProperties->unk_2C))
        ) {
            this->unk_6AD = 1;
            
            return 1;
        }
    }
    
    return 0;
}

s32 Player_SetupCUpBehavior(Player* this, PlayState* play) {
    if (this->unk_6AD != 0) {
        Player_SetupItemCsOrFirstPerson(this, play);
        
        return 1;
    }
    
    if (
        (this->unk_664 != NULL) && (CHECK_FLAG_ALL(this->unk_664->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18) ||
        (this->unk_664->naviEnemyId != NAVI_ENEMY_NONE))
    ) {
        this->stateFlags2 |= PLAYER_STATE2_21;
    } else if (
        (this->naviTextId == 0) && !func_8008E9C4(this) && CHECK_BTN_ALL(sControlInput->press.button, BTN_CUP) &&
        (YREG(15) != 0x10) && (YREG(15) != 0x20) && !Player_ForceFirstPerson(this, play)
    ) {
        func_80078884(NA_SE_SY_ERROR);
    }
    
    return 0;
}

void Player_SetupJumpSlash(PlayState* play, Player* this, s32 arg2, f32 xzVelocity, f32 yVelocity) {
    Player_StartMeleeWeaponAttack(play, this, arg2);
    Player_SetActionFunc(play, this, Player_JumpSlash, 0);
    
    this->stateFlags3 |= PLAYER_STATE3_1;
    
    this->currentYaw = this->actor.shape.rot.y;
    this->linearVelocity = xzVelocity;
    this->actor.velocity.y = yVelocity;
    
    this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    this->hoverBootsTimer = 0;
    
    Player_PlayJumpSfx(this);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
}

s32 Player_CanJumpSlash(Player* this) {
    if (!(this->stateFlags1 & PLAYER_STATE1_22) && (Player_GetMeleeWeaponHeld(this) != 0)) {
        if (
            sActiveItemUseFlag ||
            ((this->actor.category != ACTORCAT_PLAYER) && CHECK_BTN_ALL(sControlInput->press.button, BTN_B))
        ) {
            return 1;
        }
    }
    
    return 0;
}

s32 Player_SetupMidairJumpSlash(Player* this, PlayState* play) {
    if (Player_CanJumpSlash(this) && (sFloorSpecialProperty != 7)) {
        Player_SetupJumpSlash(play, this, PLAYER_MWA_JUMPSLASH_START, 3.0f, 4.5f);
        
        return 1;
    }
    
    return 0;
}

void Player_SetupRolling(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_Rolling, 0);
    LinkAnimation_PlayOnceSetSpeed(
        play,
        &this->skelAnime,
        GET_PLAYER_ANIM(PLAYER_ANIMGROUP_16, this->modelAnimType),
        1.25f * sWaterSpeedScale
    );
}

s32 Player_CanRoll(Player* this, PlayState* play) {
    if ((this->unk_84B[this->unk_846] == 0) && (sFloorSpecialProperty != 7)) {
        Player_SetupRolling(this, play);
        
        return 1;
    }
    
    return 0;
}

void Player_SetupBackflipOrSidehop(Player* this, PlayState* play, s32 arg2) {
    Player_SetupJumpWithSfx(this, sAnims_ManualJump[arg2][0], !(arg2 & 1) ? 5.8f : 3.5f, play, NA_SE_VO_LI_SWORD_N);
    
    if (arg2) {
    }
    
    this->unk_850 = 1;
    this->unk_84F = arg2;
    
    this->currentYaw = this->actor.shape.rot.y + (arg2 << 0xE);
    this->linearVelocity = !(arg2 & 1) ? 6.0f : 8.5f;
    
    this->stateFlags2 |= PLAYER_STATE2_19;
    
    func_8002F7DC(&this->actor, ((arg2 << 0xE) == 0x8000) ? NA_SE_PL_ROLL : NA_SE_PL_SKIP);
}

s32 Player_SetupJumpSlashOrRoll(Player* this, PlayState* play) {
    s32 sp2C;
    
    if (
        CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
        (play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) && (sFloorSpecialProperty != 7) &&
        (SurfaceType_GetFloorEffect(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId) != 1)
    ) {
        sp2C = this->unk_84B[this->unk_846];
        
        if (sp2C <= 0) {
            if (Player_IsZTargeting(this)) {
                if (this->actor.category != ACTORCAT_PLAYER) {
                    if (sp2C < 0) {
                        Player_SetupJump(this, &gPlayerAnim_link_normal_jump, REG(69) / 100.0f, play);
                    } else {
                        Player_SetupRolling(this, play);
                    }
                } else {
                    if ((Player_GetMeleeWeaponHeld(this) != 0) && Player_CanUseItem(this)) {
                        Player_SetupJumpSlash(play, this, PLAYER_MWA_JUMPSLASH_START, 5.0f, 5.0f);
                    } else {
                        Player_SetupRolling(this, play);
                    }
                }
                
                return 1;
            }
        } else {
            Player_SetupBackflipOrSidehop(this, play, sp2C);
            
            return 1;
        }
    }
    
    return 0;
}

void Player_EndRun(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 sp30;
    
    sp30 = this->unk_868 - 3.0f;
    if (sp30 < 0.0f) {
        sp30 += 29.0f;
    }
    
    if (sp30 < 14.0f) {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_18, this->modelAnimType);
        sp30 = 11.0f - sp30;
        if (sp30 < 0.0f) {
            sp30 = 1.375f * -sp30;
        }
        sp30 /= 11.0f;
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_19, this->modelAnimType);
        sp30 = 26.0f - sp30;
        if (sp30 < 0.0f) {
            sp30 = 2 * -sp30;
        }
        sp30 /= 12.0f;
    }
    
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        anim,
        1.0f,
        0.0f,
        Animation_GetLastFrame(anim),
        ANIMMODE_ONCE,
        4.0f * sp30
    );
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupEndRun(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    Player_EndRun(this, play);
}

void Player_SetupStandingStillNoMorph(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_StandingStill, 1);
    Player_PlayAnimOnce(play, this, Player_GetAnim_StandingStill(this));
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_ClearLookAndAttention(Player* this, PlayState* play) {
    if (!(this->stateFlags3 & PLAYER_STATE3_7)) {
        Player_ResetLookAngles(this);
        if (this->stateFlags1 & PLAYER_STATE1_27) {
            Player_SetupSwimIdle(play, this);
        } else {
            Player_SetupStandingStillType(this, play);
        }
        if (this->unk_6AD < 4) {
            this->unk_6AD = 0;
        }
    }
    
    this->stateFlags1 &= ~(PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_20);
}

s32 Player_SetupRollOrPutAway(Player* this, PlayState* play) {
    if (
        !Player_SetupStartEnemyZTargeting(this) && (D_808535E0 == 0) && !(this->stateFlags1 & PLAYER_STATE1_23) &&
        CHECK_BTN_ALL(sControlInput->press.button, BTN_A)
    ) {
        if (Player_CanRoll(this, play)) {
            return 1;
        }
        if ((this->unk_837 == 0) && (this->heldItemActionParam >= PLAYER_AP_SWORD_MASTER)) {
            Player_UseItem(play, this, ITEM_NONE);
        } else {
            this->stateFlags2 ^= PLAYER_STATE2_20;
        }
    }
    
    return 0;
}

s32 Player_SetupDefend(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 frame;
    
    if (
        (play->shootingGalleryStatus == 0) && (this->currentShield != PLAYER_SHIELD_NONE) &&
        CHECK_BTN_ALL(sControlInput->cur.button, BTN_R) &&
        (Player_IsChildWithHylianShield(this) || (!Player_IsFriendlyZTargeting(this) && (this->unk_664 == NULL)))
    ) {
        Player_InactivateMeleeWeapon(this);
        Player_DetatchHeldActor(play, this);
        
        if (Player_SetActionFunc(play, this, Player_AimShieldCrouched, 0)) {
            this->stateFlags1 |= PLAYER_STATE1_22;
            
            if (!Player_IsChildWithHylianShield(this)) {
                Player_SetModelsForHoldingShield(this);
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_20, this->modelAnimType);
            } else {
                anim = &gPlayerAnim_clink_normal_defense_ALL;
            }
            
            if (anim != this->skelAnime.animation) {
                if (func_8008E9C4(this)) {
                    this->unk_86C = 1.0f;
                } else {
                    this->unk_86C = 0.0f;
                    Player_ResetLeftRightBlendWeight(this);
                }
                this->unk_6BC = this->unk_6BE = this->unk_6C0 = 0;
            }
            
            frame = Animation_GetLastFrame(anim);
            LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, frame, frame, ANIMMODE_ONCE, 0.0f);
            
            if (Player_IsChildWithHylianShield(this)) {
                Player_SetupAnimMovement(play, this, 4);
            }
            
            func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_POSTURE);
        }
        
        return 1;
    }
    
    return 0;
}

s32 Player_SetupTurnAroundRunning(Player* this, f32* arg1, s16* arg2) {
    s16 yaw = this->currentYaw - *arg2;
    
    if (ABS(yaw) > 0x6000) {
        if (Player_StepLinearVelToZero(this)) {
            *arg1 = 0.0f;
            *arg2 = this->currentYaw;
        } else {
            return 1;
        }
    }
    
    return 0;
}

void Player_DeactivateComboTimer(Player* this) {
    if ((this->unk_844 > 0) && !CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        this->unk_844 = -this->unk_844;
    }
}

s32 Player_SetupStartChargeSpinAttack(Player* this, PlayState* play) {
    if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
        if (
            !(this->stateFlags1 & PLAYER_STATE1_22) && (Player_GetMeleeWeaponHeld(this) != 0) && (this->unk_844 == 1) &&
            (this->heldItemActionParam != PLAYER_AP_STICK)
        ) {
            if ((this->heldItemActionParam != PLAYER_AP_SWORD_BGS) || (gSaveContext.swordHealth > 0.0f)) {
                Player_StartChargeSpinAttack(play, this);
                
                return 1;
            }
        }
    } else {
        Player_DeactivateComboTimer(this);
    }
    
    return 0;
}

s32 Player_SetupThrowDekuNut(PlayState* play, Player* this) {
    if (
        (play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (AMMO(ITEM_NUT) != 0)
    ) {
        Player_SetActionFunc(play, this, Player_ThrowDekuNut, 0);
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_light_bom);
        this->unk_6AD = 0;
        
        return 1;
    }
    
    return 0;
}

static struct_80854554 sAnims_BottleSwing[] = {
    { &gPlayerAnim_link_bottle_bug_miss,  &gPlayerAnim_link_bottle_bug_in,  2, 3 },
    { &gPlayerAnim_link_bottle_fish_miss, &gPlayerAnim_link_bottle_fish_in, 5, 3 },
};

s32 Player_CanSwingBottleOrCastFishingRod(PlayState* play, Player* this) {
    Vec3f sp24;
    
    if (sActiveItemUseFlag) {
        if (Player_GetBottleHeld(this) >= 0) {
            Player_SetActionFunc(play, this, Player_SwingBottle, 0);
            
            if (this->actor.yDistToWater > 12.0f) {
                this->unk_850 = 1;
            }
            
            Player_PlayAnimOnceSlowed(play, this, sAnims_BottleSwing[this->unk_850].unk_00);
            
            func_8002F7DC(&this->actor, NA_SE_IT_SWORD_SWING);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_AUTO_JUMP);
            
            return 1;
        }
        
        if (this->heldItemActionParam == PLAYER_AP_FISHING_POLE) {
            sp24 = this->actor.world.pos;
            sp24.y += 50.0f;
            
            if (
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (this->actor.world.pos.z > 1300.0f) ||
                BgCheck_SphVsFirstPoly(&play->colCtx, &sp24, 20.0f)
            ) {
                func_80078884(NA_SE_SY_ERROR);
                
                return 0;
            }
            
            Player_SetActionFunc(play, this, Player_CastFishingRod, 0);
            this->unk_860 = 1;
            Player_StopMovement(this);
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_fishing_throw);
            
            return 1;
        } else {
            return 0;
        }
    }
    
    return 0;
}

void Player_SetupRun(Player* this, PlayState* play) {
    PlayerActionFunc func;
    
    if (Player_IsZTargeting(this)) {
        func = Player_ZTargetingRun;
    } else {
        func = Player_Run;
    }
    
    Player_SetActionFunc(play, this, func, 1);
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_2, this->modelAnimType));
    
    this->unk_89C = 0;
    this->unk_864 = this->unk_868 = 0.0f;
}

void Player_SetupZTargetRunning(Player* this, PlayState* play, s16 arg2) {
    this->actor.shape.rot.y = this->currentYaw = arg2;
    Player_SetupRun(this, play);
}

s32 Player_SetupDefaultSpawnBehavior(PlayState* play, Player* this, f32 arg2) {
    WaterBox* sp2C;
    f32 sp28;
    
    sp28 = this->actor.world.pos.y;
    if (
        WaterBox_GetSurface1(play, &play->colCtx, this->actor.world.pos.x, this->actor.world.pos.z, &sp28, &sp2C) !=
        0
    ) {
        sp28 -= this->actor.world.pos.y;
        if (this->ageProperties->unk_24 <= sp28) {
            Player_SetActionFunc(play, this, Player_SpawnSwimming, 0);
            Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
            this->stateFlags1 |= PLAYER_STATE1_27 | PLAYER_STATE1_29;
            this->unk_850 = 20;
            this->linearVelocity = 2.0f;
            Player_SetBootData(play, this);
            
            return 0;
        }
    }
    
    Player_SetupCutsceneMovement(play, this, arg2, this->actor.shape.rot.y);
    this->stateFlags1 |= PLAYER_STATE1_29;
    
    return 1;
}

void Player_SpawnWalking_Default(PlayState* play, Player* this) {
    if (Player_SetupDefaultSpawnBehavior(play, this, 180.0f)) {
        this->unk_850 = -20;
    }
}

void Player_SpawnWalking_Slow(PlayState* play, Player* this) {
    this->linearVelocity = 2.0f;
    gSaveContext.entranceSpeed = 2.0f;
    if (Player_SetupDefaultSpawnBehavior(play, this, 120.0f)) {
        this->unk_850 = -15;
    }
}

void Player_SpawnWalking_PrevSpeed(PlayState* play, Player* this) {
    if (gSaveContext.entranceSpeed < 0.1f) {
        gSaveContext.entranceSpeed = 0.1f;
    }
    
    this->linearVelocity = gSaveContext.entranceSpeed;
    
    if (Player_SetupDefaultSpawnBehavior(play, this, 800.0f)) {
        this->unk_850 = -80 / this->linearVelocity;
        if (this->unk_850 < -20) {
            this->unk_850 = -20;
        }
    }
}

void Player_SetupFriendlyBackwalk(Player* this, s16 yaw, PlayState* play) {
    Player_SetActionFunc(play, this, Player_FriendlyBackwalk, 1);
    LinkAnimation_CopyJointToMorph(play, &this->skelAnime);
    this->unk_864 = this->unk_868 = 0.0f;
    this->currentYaw = yaw;
}

void Player_SetupFriendlySidewalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_EndSidewalk, 1);
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_1, this->modelAnimType));
}

void Player_SetupUnfriendlyBackwalk(Player* this, s16 yaw, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UnfriendlyBackwalk, 1);
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        &gPlayerAnim_link_anchor_back_walk,
        2.2f,
        0.0f,
        Animation_GetLastFrame(&gPlayerAnim_link_anchor_back_walk),
        ANIMMODE_ONCE,
        -6.0f
    );
    this->linearVelocity = 8.0f;
    this->currentYaw = yaw;
}

void Player_SetupSidewalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_Sidewalk, 1);
    Player_ChangeAnimShortMorphLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_25, this->modelAnimType));
    this->unk_868 = 0.0f;
}

void Player_SetupEndUnfriendlyBackwalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_EndUnfriendlyBackwalk, 1);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, &gPlayerAnim_link_anchor_back_brake, 2.0f);
}

void Player_SetupTurn(PlayState* play, Player* this, s16 yaw) {
    this->currentYaw = yaw;
    Player_SetActionFunc(play, this, Player_Turn, 1);
    this->unk_87E = 1200;
    this->unk_87E *= sWaterSpeedScale;
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        GET_PLAYER_ANIM(PLAYER_ANIMGROUP_26, this->modelAnimType),
        1.0f,
        0.0f,
        0.0f,
        ANIMMODE_LOOP,
        -6.0f
    );
}

void Player_EndUnfriendlyZTarget(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    
    Player_SetActionFunc(play, this, Player_StandingStill, 1);
    
    if (this->unk_870 < 0.5f) {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_28, this->modelAnimType);
    } else {
        anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_27, this->modelAnimType);
    }
    Player_PlayAnimOnce(play, this, anim);
    
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupUnfriendlyZTarget(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_7, this->modelAnimType));
    this->unk_850 = 1;
}

void Player_SetupEndUnfriendlyZTarget(Player* this, PlayState* play) {
    if (this->linearVelocity != 0.0f) {
        Player_SetupRun(this, play);
    } else {
        Player_EndUnfriendlyZTarget(this, play);
    }
}

void Player_EndCutsceneMovement(Player* this, PlayState* play) {
    if (this->linearVelocity != 0.0f) {
        Player_SetupRun(this, play);
    } else {
        Player_SetupStandingStillType(this, play);
    }
}

s32 Player_SetupSpawnSplash(PlayState* play, Player* this, f32 arg2, s32 splashScale) {
    f32 sp3C = fabsf(arg2);
    WaterBox* sp38;
    f32 sp34;
    Vec3f splashPos;
    s32 splashType;
    
    if (sp3C > 2.0f) {
        splashPos.x = this->bodyPartsPos[PLAYER_BODYPART_WAIST].x;
        splashPos.z = this->bodyPartsPos[PLAYER_BODYPART_WAIST].z;
        sp34 = this->actor.world.pos.y;
        if (WaterBox_GetSurface1(play, &play->colCtx, splashPos.x, splashPos.z, &sp34, &sp38)) {
            if ((sp34 - this->actor.world.pos.y) < 100.0f) {
                splashType = (sp3C <= 10.0f) ? 0 : 1;
                splashPos.y = sp34;
                EffectSsGSplash_Spawn(play, &splashPos, NULL, NULL, splashType, splashScale);
                
                return 1;
            }
        }
    }
    
    return 0;
}

void Player_StartJumpOutOfWater(PlayState* play, Player* this, f32 arg2) {
    this->stateFlags1 |= PLAYER_STATE1_18;
    this->stateFlags1 &= ~PLAYER_STATE1_27;
    
    Player_ResetSubCam(play, this);
    if (Player_SetupSpawnSplash(play, this, arg2, 500)) {
        func_8002F7DC(&this->actor, NA_SE_EV_JUMP_OUT_WATER);
    }
    
    Player_SetBootData(play, this);
}

s32 Player_SetupDive(PlayState* play, Player* this, Input* arg2) {
    if (!(this->stateFlags1 & PLAYER_STATE1_10) && !(this->stateFlags2 & PLAYER_STATE2_10)) {
        if (
            (arg2 == NULL) || (CHECK_BTN_ALL(arg2->press.button, BTN_A) && (ABS(this->unk_6C2) < 12000) &&
            (this->currentBoots != PLAYER_BOOTS_IRON))
        ) {
            Player_SetActionFunc(play, this, Player_Dive, 0);
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_swimer_swim_deep_start);
            
            this->unk_6C2 = 0;
            this->stateFlags2 |= PLAYER_STATE2_10;
            this->actor.velocity.y = 0.0f;
            
            if (arg2 != NULL) {
                this->stateFlags2 |= PLAYER_STATE2_11;
                func_8002F7DC(&this->actor, NA_SE_PL_DIVE_BUBBLE);
            }
            
            return 1;
        }
    }
    
    if ((this->stateFlags1 & PLAYER_STATE1_10) || (this->stateFlags2 & PLAYER_STATE2_10)) {
        if (this->actor.velocity.y > 0.0f) {
            if (this->actor.yDistToWater < this->ageProperties->unk_30) {
                this->stateFlags2 &= ~PLAYER_STATE2_10;
                
                if (arg2 != NULL) {
                    Player_SetActionFunc(play, this, Player_GetItemInWater, 1);
                    
                    if (this->stateFlags1 & PLAYER_STATE1_10) {
                        this->stateFlags1 |= PLAYER_STATE1_10 | PLAYER_STATE1_11 | PLAYER_STATE1_29;
                    }
                    
                    this->unk_850 = 2;
                }
                
                Player_ResetSubCam(play, this);
                Player_ChangeAnimMorphToLastFrame(
                    play,
                    this,
                    (this->stateFlags1 & PLAYER_STATE1_11) ? &gPlayerAnim_link_swimer_swim_get : &gPlayerAnim_link_swimer_swim_deep_end
                );
                
                if (Player_SetupSpawnSplash(play, this, this->actor.velocity.y, 500)) {
                    func_8002F7DC(&this->actor, NA_SE_PL_FACE_UP);
                }
                
                return 1;
            }
        }
    }
    
    return 0;
}

void Player_RiseFromDive(PlayState* play, Player* this) {
    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_swimer_swim);
    this->unk_6C2 = 16000;
    this->unk_850 = 1;
}

void func_8083D36C(PlayState* play, Player* this) {
    if ((this->currentBoots != PLAYER_BOOTS_IRON) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        Player_ResetAttributesAndHeldActor(play, this);
        
        if ((this->currentBoots != PLAYER_BOOTS_IRON) && (this->stateFlags2 & PLAYER_STATE2_10)) {
            this->stateFlags2 &= ~PLAYER_STATE2_10;
            Player_SetupDive(play, this, 0);
            this->unk_84F = 1;
        } else if (Player_FallingDive == this->func_674) {
            Player_SetActionFunc(play, this, Player_Dive, 0);
            Player_RiseFromDive(play, this);
        } else {
            Player_SetActionFunc(play, this, Player_UpdateSwimIdle, 1);
            Player_ChangeAnimMorphToLastFrame(
                play,
                this,
                (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ? &gPlayerAnim_link_swimer_wait2swim_wait : &gPlayerAnim_link_swimer_land2swim_wait
            );
        }
    }
    
    if (!(this->stateFlags1 & PLAYER_STATE1_27) || (this->actor.yDistToWater < this->ageProperties->unk_2C)) {
        if (Player_SetupSpawnSplash(play, this, this->actor.velocity.y, 500)) {
            func_8002F7DC(&this->actor, NA_SE_EV_DIVE_INTO_WATER);
            
            if (this->fallDistance > 800.0f) {
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
            }
        }
    }
    
    this->stateFlags1 |= PLAYER_STATE1_27;
    this->stateFlags2 |= PLAYER_STATE2_10;
    this->stateFlags1 &= ~(PLAYER_STATE1_18 | PLAYER_STATE1_19);
    this->unk_854 = 0.0f;
    
    Player_SetBootData(play, this);
}

void func_8083D53C(PlayState* play, Player* this) {
    if (this->actor.yDistToWater < this->ageProperties->unk_2C) {
        Audio_SetBaseFilter(0);
        this->unk_840 = 0;
    } else {
        Audio_SetBaseFilter(0x20);
        if (this->unk_840 < 300) {
            this->unk_840++;
        }
    }
    
    if ((Player_JumpUpToLedge != this->func_674) && (Player_ClimbOntoLedge != this->func_674)) {
        if (this->ageProperties->unk_2C < this->actor.yDistToWater) {
            if (
                !(this->stateFlags1 & PLAYER_STATE1_27) ||
                (!((this->currentBoots == PLAYER_BOOTS_IRON) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) &&
                (Player_DamagedSwim != this->func_674) && (Player_Drown != this->func_674) &&
                (Player_UpdateSwimIdle != this->func_674) && (Player_Swim != this->func_674) &&
                (Player_ZTargetSwimming != this->func_674) && (Player_Dive != this->func_674) &&
                (Player_GetItemInWater != this->func_674) && (Player_SpawnSwimming != this->func_674))
            ) {
                func_8083D36C(play, this);
                
                return;
            }
        } else if ((this->stateFlags1 & PLAYER_STATE1_27) && (this->actor.yDistToWater < this->ageProperties->unk_24)) {
            if ((this->skelAnime.moveFlags == 0) && (this->currentBoots != PLAYER_BOOTS_IRON)) {
                Player_SetupTurn(play, this, this->actor.shape.rot.y);
            }
            Player_StartJumpOutOfWater(play, this, this->actor.velocity.y);
        }
    }
}

void func_8083D6EC(PlayState* play, Player* this) {
    Vec3f ripplePos;
    f32 temp1;
    f32 temp2;
    f32 temp3;
    f32 temp4;
    
    this->actor.minVelocityY = -20.0f;
    this->actor.gravity = REG(68) / 100.0f;
    
    if (Player_IsFloorSinkingSand(sFloorSpecialProperty)) {
        temp1 = fabsf(this->linearVelocity) * 20.0f;
        temp3 = 0.0f;
        
        if (sFloorSpecialProperty == 4) {
            if (this->unk_6C4 > 1300.0f) {
                temp2 = this->unk_6C4;
            } else {
                temp2 = 1300.0f;
            }
            if (this->currentBoots == PLAYER_BOOTS_HOVER) {
                temp1 += temp1;
            } else if (this->currentBoots == PLAYER_BOOTS_IRON) {
                temp1 *= 0.3f;
            }
        } else {
            temp2 = 20000.0f;
            if (this->currentBoots != PLAYER_BOOTS_HOVER) {
                temp1 += temp1;
            } else if ((sFloorSpecialProperty == 7) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                temp1 = 0;
            }
        }
        
        if (this->currentBoots != PLAYER_BOOTS_HOVER) {
            temp3 = (temp2 - this->unk_6C4) * 0.02f;
            temp3 = CLAMP(temp3, 0.0f, 300.0f);
            if (this->currentBoots == PLAYER_BOOTS_IRON) {
                temp3 += temp3;
            }
        }
        
        this->unk_6C4 += temp3 - temp1;
        this->unk_6C4 = CLAMP(this->unk_6C4, 0.0f, temp2);
        
        this->actor.gravity -= this->unk_6C4 * 0.004f;
    } else {
        this->unk_6C4 = 0.0f;
    }
    
    if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
        if (this->actor.yDistToWater < 50.0f) {
            temp4 = fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].x - this->unk_A88.x) +
                fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].y - this->unk_A88.y) +
                fabsf(this->bodyPartsPos[PLAYER_BODYPART_WAIST].z - this->unk_A88.z);
            if (temp4 > 4.0f) {
                temp4 = 4.0f;
            }
            this->unk_854 += temp4;
            
            if (this->unk_854 > 15.0f) {
                this->unk_854 = 0.0f;
                
                ripplePos.x = (Rand_ZeroOne() * 10.0f) + this->actor.world.pos.x;
                ripplePos.y = this->actor.world.pos.y + this->actor.yDistToWater;
                ripplePos.z = (Rand_ZeroOne() * 10.0f) + this->actor.world.pos.z;
                EffectSsGRipple_Spawn(play, &ripplePos, 100, 500, 0);
                
                if (
                    (this->linearVelocity > 4.0f) && !Player_IsSwimming(this) &&
                    ((this->actor.world.pos.y + this->actor.yDistToWater) <
                    this->bodyPartsPos[PLAYER_BODYPART_WAIST].y)
                ) {
                    Player_SetupSpawnSplash(
                        play,
                        this,
                        20.0f,
                        (fabsf(this->linearVelocity) * 50.0f) + (this->actor.yDistToWater * 5.0f)
                    );
                }
            }
        }
        
        if (this->actor.yDistToWater > 40.0f) {
            s32 numBubbles = 0;
            s32 i;
            
            if ((this->actor.velocity.y > -1.0f) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                if (Rand_ZeroOne() < 0.2f) {
                    numBubbles = 1;
                }
            } else {
                numBubbles = this->actor.velocity.y * -2.0f;
            }
            
            for (i = 0; i < numBubbles; i++) {
                EffectSsBubble_Spawn(play, &this->actor.world.pos, 20.0f, 10.0f, 20.0f, 0.13f);
            }
        }
    }
}

s32 Player_LookAtTargetActor(Player* this, s32 arg1) {
    Actor* unk_664 = this->unk_664;
    Vec3f sp30;
    s16 sp2E;
    s16 sp2C;
    
    sp30.x = this->actor.world.pos.x;
    sp30.y = this->bodyPartsPos[PLAYER_BODYPART_HEAD].y + 3.0f;
    sp30.z = this->actor.world.pos.z;
    sp2E = Math_Vec3f_Pitch(&sp30, &unk_664->focus.pos);
    sp2C = Math_Vec3f_Yaw(&sp30, &unk_664->focus.pos);
    Math_SmoothStepToS(&this->actor.focus.rot.y, sp2C, 4, 10000, 0);
    Math_SmoothStepToS(&this->actor.focus.rot.x, sp2E, 4, 10000, 0);
    this->unk_6AE |= 2;
    
    return Player_UpdateLookRot(this, arg1);
}

void Player_SetLookAngle(Player* this, PlayState* play) {
    static Vec3f D_8085456C = { 0.0f, 100.0f, 40.0f };
    s16 sp46;
    s16 temp2;
    f32 temp1;
    Vec3f sp34;
    
    if (this->unk_664 != NULL) {
        if (func_8002DD78(this) || Player_IsAimingReady_Boomerang(this)) {
            Player_LookAtTargetActor(this, 1);
        } else {
            Player_LookAtTargetActor(this, 0);
        }
        
        return;
    }
    
    if (sFloorSpecialProperty == SURFACE_BEHAVIOUR_LINK_LOOK_UP) {
        Math_SmoothStepToS(&this->actor.focus.rot.x, -20000, 10, 4000, 800);
    } else {
        sp46 = 0;
        temp1 = Player_RelativeFloorRaycast(play, this, &D_8085456C, &sp34);
        if (temp1 > BGCHECK_Y_MIN) {
            temp2 = Math_Atan2S(40.0f, this->actor.world.pos.y - temp1);
            sp46 = CLAMP(temp2, -4000, 4000);
        }
        this->actor.focus.rot.y = this->actor.shape.rot.y;
        Math_SmoothStepToS(&this->actor.focus.rot.x, sp46, 14, 4000, 30);
    }
    
    Player_UpdateLookRot(this, func_8002DD78(this) || Player_IsAimingReady_Boomerang(this));
}

void func_8083DDC8(Player* this, PlayState* play) {
    s16 temp1;
    s16 temp2;
    
    if (!func_8002DD78(this) && !Player_IsAimingReady_Boomerang(this) && (this->linearVelocity > 5.0f)) {
        temp1 = this->linearVelocity * 200.0f;
        temp2 = (s16)(this->currentYaw - this->actor.shape.rot.y) * this->linearVelocity * 0.1f;
        temp1 = CLAMP(temp1, -4000, 4000);
        temp2 = CLAMP(-temp2, -4000, 4000);
        Math_ScaledStepToS(&this->unk_6BC, temp1, 900);
        this->unk_6B6 = -(f32)this->unk_6BC * 0.5f;
        Math_ScaledStepToS(&this->unk_6BA, temp2, 300);
        Math_ScaledStepToS(&this->unk_6C0, temp2, 200);
        this->unk_6AE |= 0x168;
    } else {
        Player_SetLookAngle(this, play);
    }
}

void Player_SetRunVelAndYaw(Player* this, f32 arg1, s16 arg2) {
    Math_AsymStepToF(&this->linearVelocity, arg1, REG(19) / 100.0f, 1.5f);
    Math_ScaledStepToS(&this->currentYaw, arg2, REG(27));
}

void func_8083DFE0(Player* this, f32* arg1, s16* arg2) {
    s16 yawDiff = this->currentYaw - *arg2;
    
    if (this->meleeWeaponState == 0) {
        this->linearVelocity = CLAMP(this->linearVelocity, -(R_RUN_SPEED_LIMIT / 100.0f), (R_RUN_SPEED_LIMIT / 100.0f));
    }
    
    if (ABS(yawDiff) > 0x6000) {
        if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
            this->currentYaw = *arg2;
        }
    } else {
        Math_AsymStepToF(&this->linearVelocity, *arg1, 0.05f, 0.1f);
        Math_ScaledStepToS(&this->currentYaw, *arg2, 200);
    }
}

static struct_80854578 sAnims_HorseMount[] = {
    { &gPlayerAnim_link_uma_left_up,  35.17f,  6.6099997f  },
    { &gPlayerAnim_link_uma_right_up, -34.16f, 7.91f       },
};

s32 Player_SetupMountHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 unk_04;
    f32 unk_08;
    f32 sp38;
    f32 sp34;
    s32 temp;
    
    if ((rideActor != NULL) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
        sp38 = Math_CosS(rideActor->actor.shape.rot.y);
        sp34 = Math_SinS(rideActor->actor.shape.rot.y);
        
        Player_SetupCsActionFunc(play, this, Player_SetupRideHorse);
        
        this->stateFlags1 |= PLAYER_STATE1_23;
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_WATER;
        
        if (this->mountSide < 0) {
            temp = 0;
        } else {
            temp = 1;
        }
        
        unk_04 = sAnims_HorseMount[temp].unk_04;
        unk_08 = sAnims_HorseMount[temp].unk_08;
        this->actor.world.pos.x =
            rideActor->actor.world.pos.x + rideActor->riderPos.x + ((unk_04 * sp38) + (unk_08 * sp34));
        this->actor.world.pos.z =
            rideActor->actor.world.pos.z + rideActor->riderPos.z + ((unk_08 * sp38) - (unk_04 * sp34));
        
        this->unk_878 = rideActor->actor.world.pos.y - this->actor.world.pos.y;
        this->currentYaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;
        
        Actor_MountHorse(play, this, &rideActor->actor);
        Player_PlayAnimOnce(play, this, sAnims_HorseMount[temp].anim);
        Player_SetupAnimMovement(play, this, 0x9B);
        this->actor.parent = this->rideActor;
        Player_ClearAttentionModeAndStopMoving(this);
        Actor_DisableLens(play);
        
        return 1;
    }
    
    return 0;
}

void Player_GetSlopeDirection(CollisionPoly* floorPoly, Vec3f* slopeNormal, s16* downwardSlopeYaw) {
    slopeNormal->x = COLPOLY_GET_NORMAL(floorPoly->normal.x);
    slopeNormal->y = COLPOLY_GET_NORMAL(floorPoly->normal.y);
    slopeNormal->z = COLPOLY_GET_NORMAL(floorPoly->normal.z);
    
    *downwardSlopeYaw = Math_Atan2S(slopeNormal->z, slopeNormal->x);
}

static LinkAnimationHeader* sAnims_SlopeSlip[] = {
    &gPlayerAnim_link_normal_down_slope_slip,
    &gPlayerAnim_link_normal_up_slope_slip,
};

s32 Player_WalkOnSlope(PlayState* play, Player* this, CollisionPoly* floorPoly) {
    s16 playerVelYaw;
    Vec3f slopeNormal;
    s16 downwardSlopeYaw;
    f32 slopeSlowdownSpeed;
    f32 slopeSlowdownSpeedStep;
    s16 velYawToDownwardSlope;
    
    if (
        !Player_InBlockingCsMode(play, this) && (Player_SlipOnSlope != this->func_674) &&
        (SurfaceType_GetFloorEffect(&play->colCtx, floorPoly, this->actor.floorBgId) == 1)
    ) {
        // Get direction of movement relative to the downward direction of the slope
        playerVelYaw = Math_Atan2S(this->actor.velocity.z, this->actor.velocity.x);
        Player_GetSlopeDirection(floorPoly, &slopeNormal, &downwardSlopeYaw);
        velYawToDownwardSlope = downwardSlopeYaw - playerVelYaw;
        
        if (ABS(velYawToDownwardSlope) > 0x3E80) { // 87.9 degrees
            // moving parallel or upwards on the slope, player does not slip but does slow down
            slopeSlowdownSpeed = (1.0f - slopeNormal.y) * 40.0f;
            slopeSlowdownSpeedStep = SQ(slopeSlowdownSpeed) * 0.015f;
            if (slopeSlowdownSpeedStep < 1.2f) {
                slopeSlowdownSpeedStep = 1.2f;
            }
            
            // slows down speed as player is climbing a slope
            this->pushedYaw = downwardSlopeYaw;
            Math_StepToF(&this->pushedSpeed, slopeSlowdownSpeed, slopeSlowdownSpeedStep);
        } else {
            // moving downward on the slope, causing player to slip
            Player_SetActionFunc(play, this, Player_SlipOnSlope, 0);
            Player_ResetAttributesAndHeldActor(play, this);
            if (sFloorPitch >= 0) {
                this->unk_84F = 1;
            }
            Player_ChangeAnimShortMorphLoop(play, this, sAnims_SlopeSlip[this->unk_84F]);
            this->linearVelocity = sqrtf(SQ(this->actor.velocity.x) + SQ(this->actor.velocity.z));
            this->currentYaw = playerVelYaw;
            
            return true;
        }
    }
    
    return false;
}

void Player_PickupItemDrop(PlayState* play, Player* this, GetItemEntry* giEntry) {
    s32 dropType = giEntry->field & 0x1F;
    
    if (!(giEntry->field & 0x80)) {
        Item_DropCollectible(play, &this->actor.world.pos, dropType | 0x8000);
        if (
            (dropType != ITEM00_BOMBS_A) && (dropType != ITEM00_ARROWS_SMALL) && (dropType != ITEM00_ARROWS_MEDIUM) &&
            (dropType != ITEM00_ARROWS_LARGE) && (dropType != ITEM00_RUPEE_GREEN) && (dropType != ITEM00_RUPEE_BLUE) &&
            (dropType != ITEM00_RUPEE_RED) && (dropType != ITEM00_RUPEE_PURPLE) && (dropType != ITEM00_RUPEE_ORANGE)
        ) {
            Item_Give(play, giEntry->itemId);
        }
    } else {
        Item_Give(play, giEntry->itemId);
    }
    
    func_80078884((this->getItemId < 0) ? NA_SE_SY_GET_BOXITEM : NA_SE_SY_GET_ITEM);
}

s32 Player_SetupGetItemOrHoldBehavior(Player* this, PlayState* play) {
    Actor* interactedActor;
    
    if (
        iREG(67) ||
        (((interactedActor = this->interactRangeActor) != NULL) && TitleCard_Clear(play, &play->actorCtx.titleCtx))
    ) {
        if (iREG(67) || (this->getItemId > GI_NONE)) {
            if (iREG(67)) {
                this->getItemId = iREG(68);
            }
            
            if (this->getItemId < GI_MAX) {
                GetItemEntry* giEntry = &sGetItemTable[this->getItemId - 1];
                
                if ((interactedActor != &this->actor) && !iREG(67)) {
                    interactedActor->parent = &this->actor;
                }
                
                iREG(67) = false;
                
                if ((Item_CheckObtainability(giEntry->itemId) == ITEM_NONE) || (play->sceneId == SCENE_BOWLING)) {
                    Player_DetatchHeldActor(play, this);
                    Player_LoadGetItemObject(this, giEntry->objectId);
                    
                    if (!(this->stateFlags2 & PLAYER_STATE2_10) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
                        Player_SetupCsActionFunc(play, this, Player_SetupGetItem);
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_get_itemB);
                        Player_SetCameraTurnAround(play, 9);
                    }
                    
                    this->stateFlags1 |= PLAYER_STATE1_10 | PLAYER_STATE1_11 | PLAYER_STATE1_29;
                    Player_ClearAttentionModeAndStopMoving(this);
                    
                    return 1;
                }
                
                Player_PickupItemDrop(play, this, giEntry);
                this->getItemId = GI_NONE;
            }
        } else if (
            CHECK_BTN_ALL(sControlInput->press.button, BTN_A) && !(this->stateFlags1 & PLAYER_STATE1_11) &&
            !(this->stateFlags2 & PLAYER_STATE2_10)
        ) {
            if (this->getItemId != GI_NONE) {
                GetItemEntry* giEntry = &sGetItemTable[-this->getItemId - 1];
                EnBox* chest = (EnBox*)interactedActor;
                
                if (giEntry->itemId != ITEM_NONE) {
                    if (
                        ((Item_CheckObtainability(giEntry->itemId) == ITEM_NONE) && (giEntry->field & 0x40)) ||
                        ((Item_CheckObtainability(giEntry->itemId) != ITEM_NONE) && (giEntry->field & 0x20))
                    ) {
                        this->getItemId = -GI_RUPEE_BLUE;
                        giEntry = &sGetItemTable[GI_RUPEE_BLUE - 1];
                    }
                }
                
                Player_SetupCsActionFunc(play, this, Player_SetupGetItem);
                this->stateFlags1 |= PLAYER_STATE1_10 | PLAYER_STATE1_11 | PLAYER_STATE1_29;
                Player_LoadGetItemObject(this, giEntry->objectId);
                this->actor.world.pos.x =
                    chest->dyna.actor.world.pos.x - (Math_SinS(chest->dyna.actor.shape.rot.y) * 29.4343f);
                this->actor.world.pos.z =
                    chest->dyna.actor.world.pos.z - (Math_CosS(chest->dyna.actor.shape.rot.y) * 29.4343f);
                this->currentYaw = this->actor.shape.rot.y = chest->dyna.actor.shape.rot.y;
                Player_ClearAttentionModeAndStopMoving(this);
                
                if (
                    (giEntry->itemId != ITEM_NONE) && (giEntry->gi >= 0) &&
                    (Item_CheckObtainability(giEntry->itemId) == ITEM_NONE)
                ) {
                    Player_PlayAnimOnceSlowed(play, this, this->ageProperties->unk_98);
                    Player_SetupAnimMovement(play, this, 0x28F);
                    chest->unk_1F4 = 1;
                    Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_SLOW_CHEST_CS);
                } else {
                    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_box_kick);
                    chest->unk_1F4 = -1;
                }
                
                return 1;
            }
            
            if ((this->heldActor == NULL) || Player_HoldsHookshot(this)) {
                if ((interactedActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                    s32 sp24 = this->itemActionParam;
                    
                    this->itemActionParam = PLAYER_AP_NONE;
                    this->modelAnimType = PLAYER_ANIMTYPE_0;
                    this->heldItemActionParam = this->itemActionParam;
                    Player_SetupCsActionFunc(play, this, Player_SetupHoldActor);
                    
                    if (sp24 == PLAYER_AP_SWORD_MASTER) {
                        this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_AP_LAST_USED);
                        Player_ChangeItem(play, this, PLAYER_AP_LAST_USED);
                    } else {
                        Player_UseItem(play, this, ITEM_LAST_USED);
                    }
                } else {
                    s32 strength = Player_GetStrength();
                    
                    if (
                        (interactedActor->id == ACTOR_EN_ISHI) && ((interactedActor->params & 0xF) == 1) &&
                        (strength < PLAYER_STR_SILVER_G)
                    ) {
                        return 0;
                    }
                    
                    Player_SetupCsActionFunc(play, this, Player_SetupHoldActor);
                }
                
                Player_ClearAttentionModeAndStopMoving(this);
                this->stateFlags1 |= PLAYER_STATE1_11;
                
                return 1;
            }
        }
    }
    
    return 0;
}

void Player_SetupStartThrowActor(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_StartThrowActor, 1);
    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_29, this->modelAnimType));
}

s32 Player_CanThrowActor(Player* this, Actor* actor) {
    if (
        (actor != NULL) && !(actor->flags & ACTOR_FLAG_23) &&
        ((this->linearVelocity < 1.1f) || (actor->id == ACTOR_EN_BOM_CHU))
    ) {
        return 0;
    }
    
    return 1;
}

s32 Player_SetupPutDownOrThrowActor(Player* this, PlayState* play) {
    if (
        (this->stateFlags1 & PLAYER_STATE1_11) && (this->heldActor != NULL) &&
        CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)
    ) {
        if (!Player_InterruptHoldingActor(play, this, this->heldActor)) {
            if (!Player_CanThrowActor(this, this->heldActor)) {
                Player_SetActionFunc(play, this, Player_SetupPutDownActor, 1);
                Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_30, this->modelAnimType));
            } else {
                Player_SetupStartThrowActor(this, play);
            }
        }
        
        return 1;
    }
    
    return 0;
}

s32 Player_SetupClimbWallOrLadder(Player* this, PlayState* play, u32 arg2) {
    if (this->wallHeight >= 79.0f) {
        if (
            !(this->stateFlags1 & PLAYER_STATE1_27) || (this->currentBoots == PLAYER_BOOTS_IRON) ||
            (this->actor.yDistToWater < this->ageProperties->unk_2C)
        ) {
            s32 sp8C = (arg2 & 8) ? 2 : 0;
            
            if ((sp8C != 0) || (arg2 & 2) || SurfaceType_CheckWallFlag2(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId)) {
                f32 phi_f20;
                CollisionPoly* wallPoly = this->actor.wallPoly;
                f32 sp80;
                f32 sp7C;
                f32 phi_f12;
                f32 phi_f14;
                
                phi_f20 = phi_f12 = 0.0f;
                
                if (sp8C != 0) {
                    sp80 = this->actor.world.pos.x;
                    sp7C = this->actor.world.pos.z;
                } else {
                    Vec3f sp50[3];
                    s32 i;
                    f32 sp48;
                    Vec3f* sp44 = &sp50[0];
                    
                    CollisionPoly_GetVerticesByBgId(wallPoly, this->actor.wallBgId, &play->colCtx, sp50);
                    
                    sp80 = phi_f12 = sp44->x;
                    sp7C = phi_f14 = sp44->z;
                    phi_f20 = sp44->y;
                    for (i = 1; i < 3; i++) {
                        sp44++;
                        if (sp80 > sp44->x) {
                            sp80 = sp44->x;
                        } else if (phi_f12 < sp44->x) {
                            phi_f12 = sp44->x;
                        }
                        
                        if (sp7C > sp44->z) {
                            sp7C = sp44->z;
                        } else if (phi_f14 < sp44->z) {
                            phi_f14 = sp44->z;
                        }
                        
                        if (phi_f20 > sp44->y) {
                            phi_f20 = sp44->y;
                        }
                    }
                    
                    sp80 = (sp80 + phi_f12) * 0.5f;
                    sp7C = (sp7C + phi_f14) * 0.5f;
                    
                    phi_f12 = ((this->actor.world.pos.x - sp80) * COLPOLY_GET_NORMAL(wallPoly->normal.z)) -
                        ((this->actor.world.pos.z - sp7C) * COLPOLY_GET_NORMAL(wallPoly->normal.x));
                    sp48 = this->actor.world.pos.y - phi_f20;
                    
                    phi_f20 = ((f32)(s32)((sp48 / 15.000000223517418) + 0.5) * 15.000000223517418) - sp48;
                    phi_f12 = fabsf(phi_f12);
                }
                
                if (phi_f12 < 8.0f) {
                    f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                    f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                    f32 sp34 = this->wallDistance;
                    LinkAnimationHeader* sp30;
                    
                    Player_SetupCsActionFunc(play, this, Player_SetupClimbingWallOrDownLedge);
                    this->stateFlags1 |= PLAYER_STATE1_21;
                    this->stateFlags1 &= ~PLAYER_STATE1_27;
                    
                    if ((sp8C != 0) || (arg2 & 2)) {
                        if ((this->unk_84F = sp8C) != 0) {
                            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                                sp30 = &gPlayerAnim_link_normal_Fclimb_startA;
                            } else {
                                sp30 = &gPlayerAnim_link_normal_Fclimb_hold2upL;
                            }
                            sp34 = (this->ageProperties->unk_38 - 1.0f) - sp34;
                        } else {
                            sp30 = this->ageProperties->unk_A4;
                            sp34 = sp34 - 1.0f;
                        }
                        this->unk_850 = -2;
                        this->actor.world.pos.y += phi_f20;
                        this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
                    } else {
                        sp30 = this->ageProperties->unk_A8;
                        this->unk_850 = -4;
                        this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw;
                    }
                    
                    this->actor.world.pos.x = (sp34 * wallPolyNormalX) + sp80;
                    this->actor.world.pos.z = (sp34 * wallPolyNormalZ) + sp7C;
                    Player_ClearAttentionModeAndStopMoving(this);
                    Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.world.pos);
                    Player_PlayAnimOnce(play, this, sp30);
                    Player_SetupAnimMovement(play, this, 0x9F);
                    
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

void Player_SetupEndClimb(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    Player_SetActionFunc_KeepMoveFlags(play, this, Player_EndClimb, 0);
    LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, anim, (4.0f / 3.0f));
}

s32 Player_SetupEnterCrawlspace(Player* this, PlayState* play, u32 arg2) {
    CollisionPoly* wallPoly;
    Vec3f sp50[3];
    f32 sp4C;
    f32 phi_f2;
    f32 sp44;
    f32 phi_f12;
    s32 i;
    
    if (!LINK_IS_ADULT && !(this->stateFlags1 & PLAYER_STATE1_27) && (arg2 & 0x30)) {
        wallPoly = this->actor.wallPoly;
        CollisionPoly_GetVerticesByBgId(wallPoly, this->actor.wallBgId, &play->colCtx, sp50);
        
        sp4C = phi_f2 = sp50[0].x;
        sp44 = phi_f12 = sp50[0].z;
        for (i = 1; i < 3; i++) {
            if (sp4C > sp50[i].x) {
                sp4C = sp50[i].x;
            } else if (phi_f2 < sp50[i].x) {
                phi_f2 = sp50[i].x;
            }
            
            if (sp44 > sp50[i].z) {
                sp44 = sp50[i].z;
            } else if (phi_f12 < sp50[i].z) {
                phi_f12 = sp50[i].z;
            }
        }
        
        sp4C = (sp4C + phi_f2) * 0.5f;
        sp44 = (sp44 + phi_f12) * 0.5f;
        
        phi_f2 = ((this->actor.world.pos.x - sp4C) * COLPOLY_GET_NORMAL(wallPoly->normal.z)) -
            ((this->actor.world.pos.z - sp44) * COLPOLY_GET_NORMAL(wallPoly->normal.x));
        
        if (fabsf(phi_f2) < 8.0f) {
            this->stateFlags2 |= PLAYER_STATE2_16;
            
            if (CHECK_BTN_ALL(sControlInput->press.button, BTN_A)) {
                f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                f32 sp30 = this->wallDistance;
                
                Player_SetupCsActionFunc(play, this, Player_SetupInsideCrawlspace);
                this->stateFlags2 |= PLAYER_STATE2_18;
                this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
                this->actor.world.pos.x = sp4C + (sp30 * wallPolyNormalX);
                this->actor.world.pos.z = sp44 + (sp30 * wallPolyNormalZ);
                Player_ClearAttentionModeAndStopMoving(this);
                this->actor.prevPos = this->actor.world.pos;
                Player_PlayAnimOnce(play, this, &gPlayerAnim_link_child_tunnel_start);
                Player_SetupAnimMovement(play, this, 0x9D);
                
                return 1;
            }
        }
    }
    
    return 0;
}

s32 Player_SetPosYaw_WallClimb(PlayState* play, Player* this, f32 arg1, f32 arg2, f32 arg3, f32 arg4) {
    CollisionPoly* wallPoly;
    s32 sp78;
    Vec3f sp6C;
    Vec3f sp60;
    Vec3f sp54;
    f32 yawCos;
    f32 yawSin;
    s32 temp;
    f32 wallPolyNormalX;
    f32 wallPolyNormalZ;
    
    yawCos = Math_CosS(this->actor.shape.rot.y);
    yawSin = Math_SinS(this->actor.shape.rot.y);
    
    sp6C.x = this->actor.world.pos.x + (arg4 * yawSin);
    sp6C.z = this->actor.world.pos.z + (arg4 * yawCos);
    sp60.x = this->actor.world.pos.x + (arg3 * yawSin);
    sp60.z = this->actor.world.pos.z + (arg3 * yawCos);
    sp60.y = sp6C.y = this->actor.world.pos.y + arg1;
    
    if (
        BgCheck_EntityLineTest1(
            &play->colCtx,
            &sp6C,
            &sp60,
            &sp54,
            &this->actor.wallPoly,
            true,
            false,
            false,
            true,
            &sp78
        )
    ) {
        wallPoly = this->actor.wallPoly;
        
        this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;
        this->actor.wallBgId = sp78;
        
        sTouchedWallFlags = SurfaceType_GetWallFlags(&play->colCtx, wallPoly, sp78);
        
        wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
        wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
        temp = Math_Atan2S(-wallPolyNormalZ, -wallPolyNormalX);
        Math_ScaledStepToS(&this->actor.shape.rot.y, temp, 800);
        
        this->currentYaw = this->actor.shape.rot.y;
        this->actor.world.pos.x = sp54.x - (Math_SinS(this->actor.shape.rot.y) * arg2);
        this->actor.world.pos.z = sp54.z - (Math_CosS(this->actor.shape.rot.y) * arg2);
        
        return 1;
    }
    
    this->actor.bgCheckFlags &= ~BGCHECKFLAG_PLAYER_WALL_INTERACT;
    
    return 0;
}

s32 Player_SetPosYaw_PushPull(PlayState* play, Player* this) {
    return Player_SetPosYaw_WallClimb(play, this, 26.0f, this->ageProperties->unk_38 + 5.0f, 30.0f, 0.0f);
}

s32 Player_SetupExitCrawlspace(Player* this, PlayState* play) {
    s16 temp;
    
    if ((this->linearVelocity != 0.0f) && (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) && (sTouchedWallFlags & 0x30)) {
        temp = this->actor.shape.rot.y - this->actor.wallYaw;
        if (this->linearVelocity < 0.0f) {
            temp += 0x8000;
        }
        
        if (ABS(temp) > 0x4000) {
            Player_SetActionFunc(play, this, Player_ExitCrawlspace, 0);
            
            if (this->linearVelocity > 0.0f) {
                this->actor.shape.rot.y = this->actor.wallYaw + 0x8000;
                Player_PlayAnimOnce(play, this, &gPlayerAnim_link_child_tunnel_end);
                Player_SetupAnimMovement(play, this, 0x9D);
                OnePointCutscene_Init(play, 9601, 999, NULL, CAM_ID_MAIN);
            } else {
                this->actor.shape.rot.y = this->actor.wallYaw;
                LinkAnimation_Change(
                    play,
                    &this->skelAnime,
                    &gPlayerAnim_link_child_tunnel_start,
                    -1.0f,
                    Animation_GetLastFrame(&gPlayerAnim_link_child_tunnel_start),
                    0.0f,
                    ANIMMODE_ONCE,
                    0.0f
                );
                Player_SetupAnimMovement(play, this, 0x9D);
                OnePointCutscene_Init(play, 9602, 999, NULL, CAM_ID_MAIN);
            }
            
            this->currentYaw = this->actor.shape.rot.y;
            Player_StopMovement(this);
            
            return 1;
        }
    }
    
    return 0;
}

void Player_StartGrabPushPullWall(Player* this, LinkAnimationHeader* anim, PlayState* play) {
    if (!Player_SetupCsActionFunc(play, this, Player_SetupGrabPullableObject)) {
        Player_SetActionFunc(play, this, Player_GrabPushPullWall, 0);
    }
    
    Player_PlayAnimOnce(play, this, anim);
    Player_ClearAttentionModeAndStopMoving(this);
    
    this->actor.shape.rot.y = this->currentYaw = this->actor.wallYaw + 0x8000;
}

s32 Player_SetupSpecialWallInteraction(Player* this, PlayState* play) {
    DynaPolyActor* wallPolyActor;
    
    if (
        !(this->stateFlags1 & PLAYER_STATE1_11) && (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        (sTouchedWallYaw < 0x3000)
    ) {
        if (
            ((this->linearVelocity > 0.0f) && Player_SetupClimbWallOrLadder(this, play, sTouchedWallFlags)) ||
            Player_SetupEnterCrawlspace(this, play, sTouchedWallFlags)
        ) {
            return 1;
        }
        
        if (
            !Player_IsSwimming(this) && ((this->linearVelocity == 0.0f) || !(this->stateFlags2 & PLAYER_STATE2_2)) &&
            (sTouchedWallFlags & 0x40) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->wallHeight >= 39.0f)
        ) {
            this->stateFlags2 |= PLAYER_STATE2_0;
            
            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) {
                if (
                    (this->actor.wallBgId != BGCHECK_SCENE) &&
                    ((wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId)) != NULL)
                ) {
                    if (wallPolyActor->actor.id == ACTOR_BG_HEAVY_BLOCK) {
                        if (Player_GetStrength() < PLAYER_STR_GOLD_G) {
                            return 0;
                        }
                        
                        Player_SetupCsActionFunc(play, this, Player_SetupHoldActor);
                        this->stateFlags1 |= PLAYER_STATE1_11;
                        this->interactRangeActor = &wallPolyActor->actor;
                        this->getItemId = GI_NONE;
                        this->currentYaw = this->actor.wallYaw + 0x8000;
                        Player_ClearAttentionModeAndStopMoving(this);
                        
                        return 1;
                    }
                    
                    this->unk_3C4 = &wallPolyActor->actor;
                } else {
                    this->unk_3C4 = NULL;
                }
                
                Player_StartGrabPushPullWall(this, &gPlayerAnim_link_normal_push_wait, play);
                
                return 1;
            }
        }
    }
    
    return 0;
}

s32 Player_SetupPushPullWallIdle(PlayState* play, Player* this) {
    if (
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((this->stateFlags2 & PLAYER_STATE2_4) || CHECK_BTN_ALL(sControlInput->cur.button, BTN_A))
    ) {
        DynaPolyActor* wallPolyActor = NULL;
        
        if (this->actor.wallBgId != BGCHECK_SCENE) {
            wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
        }
        
        if (&wallPolyActor->actor == this->unk_3C4) {
            if (this->stateFlags2 & PLAYER_STATE2_4) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    
    Player_ReturnToStandStill(this, play);
    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_push_wait_end);
    this->stateFlags2 &= ~PLAYER_STATE2_4;
    
    return 1;
}

void Player_SetupPushWall(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_PushWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_4;
    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_push_start);
}

void Player_SetupPullWall(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_PullWall, 0);
    this->stateFlags2 |= PLAYER_STATE2_4;
    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_35, this->modelAnimType));
}

void Player_ClimbingLetGo(Player* this, PlayState* play) {
    this->stateFlags1 &= ~(PLAYER_STATE1_21 | PLAYER_STATE1_27);
    Player_SetupFallFromLedge(this, play);
    this->linearVelocity = -0.4f;
}

s32 Player_SetupClimbingLetGo(Player* this, PlayState* play) {
    if (
        !CHECK_BTN_ALL(sControlInput->press.button, BTN_A) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
        ((sTouchedWallFlags & 8) || (sTouchedWallFlags & 2) ||
        SurfaceType_CheckWallFlag2(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId))
    ) {
        return 0;
    }
    
    Player_ClimbingLetGo(this, play);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_AUTO_JUMP);
    
    return 1;
}

s32 Player_GetUnfriendlyZTargetMoveDirection(Player* this, f32 arg1, s16 arg2) {
    f32 sp1C = (s16)(arg2 - this->actor.shape.rot.y);
    f32 temp;
    
    if (this->unk_664 != NULL) {
        Player_LookAtTargetActor(this, func_8002DD78(this) || Player_IsAimingReady_Boomerang(this));
    }
    
    temp = fabsf(sp1C) / 32768.0f;
    
    if (arg1 > (((temp * temp) * 50.0f) + 6.0f)) {
        return 1;
    } else if (arg1 > (((1.0f - temp) * 10.0f) + 6.8f)) {
        return -1;
    }
    
    return 0;
}

s32 Player_GetFriendlyZTargetingMoveDirection(Player* this, f32* arg1, s16* arg2, PlayState* play) {
    s16 sp2E = *arg2 - this->targetYaw;
    u16 sp2C = ABS(sp2E);
    
    if ((func_8002DD78(this) || Player_IsAimingReady_Boomerang(this)) && (this->unk_664 == NULL)) {
        *arg1 *= Math_SinS(sp2C);
        
        if (*arg1 != 0.0f) {
            *arg2 = (((sp2E >= 0) ? 1 : -1) << 0xE) + this->actor.shape.rot.y;
        } else {
            *arg2 = this->actor.shape.rot.y;
        }
        
        if (this->unk_664 != NULL) {
            Player_LookAtTargetActor(this, 1);
        } else {
            Math_SmoothStepToS(&this->actor.focus.rot.x, sControlInput->rel.stick_y * 240.0f, 14, 4000, 30);
            Player_UpdateLookRot(this, 1);
        }
    } else {
        if (this->unk_664 != NULL) {
            return Player_GetUnfriendlyZTargetMoveDirection(this, *arg1, *arg2);
        } else {
            Player_SetLookAngle(this, play);
            if ((*arg1 != 0.0f) && (sp2C < 6000)) {
                return 1;
            } else if (*arg1 > Math_SinS((0x4000 - (sp2C >> 1))) * 200.0f) {
                return -1;
            }
        }
    }
    
    return 0;
}

s32 Player_GetPushPullDir(Player* this, f32* vel, s16* targetYaw) {
    s16 yawDiff = *targetYaw - this->actor.shape.rot.y;
    u16 absYawDiff = ABS(yawDiff);
    f32 cos = Math_CosS(absYawDiff);
    
    *vel *= cos;
    
    if (*vel != 0.0f) {
        if (cos > 0) {
            return 1;
        } else {
            return -1;
        }
    }
    
    return 0;
}

s32 Player_GetSpinAttackMoveDirection(Player* this, f32* arg1, s16* arg2, PlayState* play) {
    Player_SetLookAngle(this, play);
    
    if ((*arg1 != 0.0f) || (ABS(this->unk_87C) > 400)) {
        s16 temp1 = *arg2 - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        u16 temp2 = (ABS(temp1) - 0x2000) & 0xFFFF;
        
        if ((temp2 < 0x4000) || (this->unk_87C != 0)) {
            return -1;
        } else {
            return 1;
        }
    }
    
    return 0;
}

void Player_SetLeftRightBlendWeight(Player* this, f32 arg1, s16 arg2) {
    s16 temp = arg2 - this->actor.shape.rot.y;
    
    if (arg1 > 0.0f) {
        if (temp < 0) {
            this->unk_874 = 0.0f;
        } else {
            this->unk_874 = 1.0f;
        }
    }
    
    Math_StepToF(&this->unk_870, this->unk_874, 0.3f);
}

void func_808401B0(PlayState* play, Player* this) {
    LinkAnimation_BlendToJoint(
        play,
        &this->skelAnime,
        Player_GetAnim_FightRight(this),
        this->unk_868,
        Player_GetAnim_FightLeft(this),
        this->unk_868,
        this->unk_870,
        this->blendTable
    );
}

s32 Player_CheckWalkFrame(f32 arg0, f32 arg1, f32 arg2, f32 arg3) {
    f32 temp;
    
    if ((arg3 == 0.0f) && (arg1 > 0.0f)) {
        arg3 = arg2;
    }
    
    temp = (arg0 + arg1) - arg3;
    
    if (((temp * arg1) >= 0.0f) && (((temp - arg1) * arg1) < 0.0f)) {
        return 1;
    }
    
    return 0;
}

void Player_SetupWalkSfx(Player* this, f32 arg1) {
    f32 updateScale = R_UPDATE_RATE * 0.5f;
    
    arg1 *= updateScale;
    if (arg1 < -7.25) {
        arg1 = -7.25;
    } else if (arg1 > 7.25f) {
        arg1 = 7.25f;
    }
    
    if (1) {
    }
    
    if (
        (this->currentBoots == PLAYER_BOOTS_HOVER) && !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
        (this->hoverBootsTimer != 0)
    ) {
        func_8002F8F0(&this->actor, NA_SE_PL_HOBBERBOOTS_LV - SFX_FLAG);
    } else if (Player_CheckWalkFrame(this->unk_868, arg1, 29.0f, 10.0f) || Player_CheckWalkFrame(this->unk_868, arg1, 29.0f, 24.0f)) {
        Player_PlayWalkSfx(this, this->linearVelocity);
        if (this->linearVelocity > 4.0f) {
            this->stateFlags2 |= PLAYER_STATE2_3;
        }
    }
    
    this->unk_868 += arg1;
    
    if (this->unk_868 < 0.0f) {
        this->unk_868 += 29.0f;
    } else if (this->unk_868 >= 29.0f) {
        this->unk_868 -= 29.0f;
    }
}

void Player_UnfriendlyZTargetStandingStill(Player* this, PlayState* play) {
    f32 sp44;
    s16 sp42;
    s32 temp1;
    u32 temp2;
    s16 temp3;
    s32 temp4;
    
    if (this->stateFlags3 & PLAYER_STATE3_3) {
        if (Player_GetMeleeWeaponHeld(this) != 0) {
            this->stateFlags2 |= PLAYER_STATE2_5 | PLAYER_STATE2_6;
        } else {
            this->stateFlags3 &= ~PLAYER_STATE3_3;
        }
    }
    
    if (this->unk_850 != 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_EndAnimMovement(this);
            Player_PlayAnimLoop(play, this, Player_GetAnim_FightRight(this));
            this->unk_850 = 0;
            this->stateFlags3 &= ~PLAYER_STATE3_3;
        }
        Player_ResetLeftRightBlendWeight(this);
    } else {
        func_808401B0(play, this);
    }
    
    Player_StepLinearVelToZero(this);
    
    if (!Player_SetupAction(play, this, sAction_StandStill_TargetEnemy, 1)) {
        if (!Player_SetupStartEnemyZTargeting(this) && (!Player_IsFriendlyZTargeting(this) || (Player_StandingDefend != this->func_82C))) {
            Player_SetupEndUnfriendlyZTarget(this, play);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp44, &sp42, 0.0f, play);
        
        temp1 = Player_GetUnfriendlyZTargetMoveDirection(this, sp44, sp42);
        
        if (temp1 > 0) {
            Player_SetupZTargetRunning(this, play, sp42);
            
            return;
        }
        
        if (temp1 < 0) {
            Player_SetupUnfriendlyBackwalk(this, sp42, play);
            
            return;
        }
        
        if (sp44 > 4.0f) {
            Player_SetupSidewalk(this, play);
            
            return;
        }
        
        Player_SetupWalkSfx(this, (this->linearVelocity * 0.3f) + 1.0f);
        Player_SetLeftRightBlendWeight(this, sp44, sp42);
        
        temp2 = this->unk_868;
        if ((temp2 < 6) || ((temp2 - 0xE) < 6)) {
            Math_StepToF(&this->linearVelocity, 0.0f, 1.5f);
            
            return;
        }
        
        temp3 = sp42 - this->currentYaw;
        temp4 = ABS(temp3);
        
        if (temp4 > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.5f)) {
                this->currentYaw = sp42;
            }
            
            return;
        }
        
        Math_AsymStepToF(&this->linearVelocity, sp44 * 0.3f, 2.0f, 1.5f);
        
        if (!(this->stateFlags3 & PLAYER_STATE3_3)) {
            Math_ScaledStepToS(&this->currentYaw, sp42, temp4 * 0.1f);
        }
    }
}

void Player_FriendlyZTargetStandingStill(Player* this, PlayState* play) {
    f32 sp3C;
    s16 sp3A;
    s32 temp1;
    s16 temp2;
    s32 temp3;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndAnimMovement(this);
        Player_PlayAnimOnce(play, this, Player_GetAnim_StandingStill(this));
    }
    
    Player_StepLinearVelToZero(this);
    
    if (!Player_SetupAction(play, this, sAction_StandStill_TargetFriendly, 1)) {
        if (Player_SetupStartEnemyZTargeting(this)) {
            Player_SetupUnfriendlyZTarget(this, play);
            
            return;
        }
        
        if (!Player_IsFriendlyZTargeting(this)) {
            Player_SetActionFunc_KeepMoveFlags(play, this, Player_StandingStill, 1);
            this->currentYaw = this->actor.shape.rot.y;
            
            return;
        }
        
        if (Player_StandingDefend == this->func_82C) {
            Player_SetupUnfriendlyZTarget(this, play);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp3C, &sp3A, 0.0f, play);
        
        temp1 = Player_GetFriendlyZTargetingMoveDirection(this, &sp3C, &sp3A, play);
        
        if (temp1 > 0) {
            Player_SetupZTargetRunning(this, play, sp3A);
            
            return;
        }
        
        if (temp1 < 0) {
            Player_SetupFriendlyBackwalk(this, sp3A, play);
            
            return;
        }
        
        if (sp3C > 4.9f) {
            Player_SetupSidewalk(this, play);
            Player_ResetLeftRightBlendWeight(this);
            
            return;
        }
        if (sp3C != 0.0f) {
            Player_SetupFriendlySidewalk(this, play);
            
            return;
        }
        
        temp2 = sp3A - this->actor.shape.rot.y;
        temp3 = ABS(temp2);
        
        if (temp3 > 800) {
            Player_SetupTurn(play, this, sp3A);
        }
    }
}

void Player_SetupIdleAnim(PlayState* play, Player* this) {
    LinkAnimationHeader* anim;
    LinkAnimationHeader** animPtr;
    s32 heathIsCritical;
    s32 sp38;
    s32 sp34;
    
    if (
        (this->unk_664 != NULL) ||
        (!(heathIsCritical = Health_IsCritical()) && ((this->unk_6AC = (this->unk_6AC + 1) & 1) != 0))
    ) {
        this->stateFlags2 &= ~PLAYER_STATE2_28;
        anim = Player_GetAnim_StandingStill(this);
    } else {
        this->stateFlags2 |= PLAYER_STATE2_28;
        if (this->stateFlags1 & PLAYER_STATE1_11) {
            anim = Player_GetAnim_StandingStill(this);
        } else {
            sp38 = play->roomCtx.curRoom.behaviorType2;
            if (heathIsCritical) {
                if (this->unk_6AC >= 0) {
                    sp38 = 7;
                    this->unk_6AC = -1;
                } else {
                    sp38 = 8;
                }
            } else {
                sp34 = Rand_ZeroOne() * 5.0f;
                if (sp34 < 4) {
                    if (
                        ((sp34 != 0) && (sp34 != 3)) || ((this->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) &&
                        ((sp34 == 3) || (Player_GetMeleeWeaponHeld(this) != 0)))
                    ) {
                        if ((sp34 == 0) && Player_HoldsTwoHandedWeapon(this)) {
                            sp34 = 4;
                        }
                        sp38 = sp34 + 9;
                    }
                }
            }
            animPtr = &sAnims_Idle[sp38][0];
            if (this->modelAnimType != PLAYER_ANIMTYPE_1) {
                animPtr = &sAnims_Idle[sp38][1];
            }
            anim = *animPtr;
        }
    }
    
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        anim,
        (2.0f / 3.0f) * sWaterSpeedScale,
        0.0f,
        Animation_GetLastFrame(anim),
        ANIMMODE_ONCE,
        -6.0f
    );
}

void Player_StandingStill(Player* this, PlayState* play) {
    s32 sp44;
    s32 sp40;
    f32 sp3C;
    s16 sp3A;
    s16 temp;
    
    sp44 = Player_IsPlayingIdleAnim(this);
    sp40 = LinkAnimation_Update(play, &this->skelAnime);
    
    if (sp44 > 0) {
        Player_PlayIdleAnimSfx(this, sp44 - 1);
    }
    
    if (sp40 != 0) {
        if (this->unk_850 != 0) {
            if (DECR(this->unk_850) == 0) {
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
            this->skelAnime.jointTable[0].y = (this->skelAnime.jointTable[0].y + ((this->unk_850 & 1) * 0x50)) - 0x28;
        } else {
            Player_EndAnimMovement(this);
            Player_SetupIdleAnim(play, this);
        }
    }
    
    Player_StepLinearVelToZero(this);
    
    if (this->unk_850 == 0) {
        if (!Player_SetupAction(play, this, sAction_StandStill, 1)) {
            if (Player_SetupStartEnemyZTargeting(this)) {
                Player_SetupUnfriendlyZTarget(this, play);
                
                return;
            }
            
            if (Player_IsFriendlyZTargeting(this)) {
                Player_SetupFriendlyZTargetingStandStill(this, play);
                
                return;
            }
            
            Player_GetTargetVelAndYaw(this, &sp3C, &sp3A, 0.018f, play);
            
            if (sp3C != 0.0f) {
                Player_SetupZTargetRunning(this, play, sp3A);
                
                return;
            }
            
            temp = sp3A - this->actor.shape.rot.y;
            if (ABS(temp) > 800) {
                Player_SetupTurn(play, this, sp3A);
                
                return;
            }
            
            Math_ScaledStepToS(&this->actor.shape.rot.y, sp3A, 1200);
            this->currentYaw = this->actor.shape.rot.y;
            if (Player_GetAnim_StandingStill(this) == this->skelAnime.animation) {
                Player_SetLookAngle(this, play);
            }
        }
    }
}

void Player_EndSidewalk(Player* this, PlayState* play) {
    f32 frames;
    f32 coeff;
    f32 sp44;
    s16 sp42;
    s32 temp1;
    s16 temp2;
    s32 temp3;
    s32 direction;
    
    this->skelAnime.mode = 0;
    LinkAnimation_SetUpdateFunction(&this->skelAnime);
    
    this->skelAnime.animation = Player_GetAnim_SidewalkEnd(this);
    
    if (this->skelAnime.animation == &gPlayerAnim_link_bow_side_walk) {
        frames = 24.0f;
        coeff = -(MREG(95) / 100.0f);
    } else {
        frames = 29.0f;
        coeff = MREG(95) / 100.0f;
    }
    
    this->skelAnime.animLength = frames;
    this->skelAnime.endFrame = frames - 1.0f;
    
    if ((s16)(this->currentYaw - this->actor.shape.rot.y) >= 0) {
        direction = 1;
    } else {
        direction = -1;
    }
    
    this->skelAnime.playSpeed = direction * (this->linearVelocity * coeff);
    
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 0.0f) || LinkAnimation_OnFrame(&this->skelAnime, frames * 0.5f)) {
        Player_PlayWalkSfx(this, this->linearVelocity);
    }
    
    if (!Player_SetupAction(play, this, sAction_SidewalkStop, 1)) {
        if (Player_SetupStartEnemyZTargeting(this)) {
            Player_SetupUnfriendlyZTarget(this, play);
            
            return;
        }
        
        if (!Player_IsFriendlyZTargeting(this)) {
            Player_SetupStandingStillMorph(this, play);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp44, &sp42, 0.0f, play);
        temp1 = Player_GetFriendlyZTargetingMoveDirection(this, &sp44, &sp42, play);
        
        if (temp1 > 0) {
            Player_SetupZTargetRunning(this, play, sp42);
            
            return;
        }
        
        if (temp1 < 0) {
            Player_SetupFriendlyBackwalk(this, sp42, play);
            
            return;
        }
        
        if (sp44 > 4.9f) {
            Player_SetupSidewalk(this, play);
            Player_ResetLeftRightBlendWeight(this);
            
            return;
        }
        
        if ((sp44 == 0.0f) && (this->linearVelocity == 0.0f)) {
            Player_SetupFriendlyZTargetingStandStill(this, play);
            
            return;
        }
        
        temp2 = sp42 - this->currentYaw;
        temp3 = ABS(temp2);
        
        if (temp3 > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.5f)) {
                this->currentYaw = sp42;
            }
            
            return;
        }
        
        Math_AsymStepToF(&this->linearVelocity, sp44 * 0.4f, 1.5f, 1.5f);
        Math_ScaledStepToS(&this->currentYaw, sp42, temp3 * 0.1f);
    }
}

void Player_UpdateBackwalk(Player* this, PlayState* play) {
    f32 temp1;
    f32 temp2;
    
    if (this->unk_864 < 1.0f) {
        temp1 = R_UPDATE_RATE * 0.5f;
        Player_SetupWalkSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(
            play,
            &this->skelAnime,
            GET_PLAYER_ANIM(PLAYER_ANIMGROUP_31, this->modelAnimType),
            this->unk_868
        );
        this->unk_864 += 1 * temp1;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }
        temp1 = this->unk_864;
    } else {
        temp2 = this->linearVelocity - (REG(48) / 100.0f);
        if (temp2 < 0.0f) {
            temp1 = 1.0f;
            Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            LinkAnimation_LoadToJoint(
                play,
                &this->skelAnime,
                GET_PLAYER_ANIM(PLAYER_ANIMGROUP_31, this->modelAnimType),
                this->unk_868
            );
        } else {
            temp1 = (REG(37) / 1000.0f) * temp2;
            if (temp1 < 1.0f) {
                Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            } else {
                temp1 = 1.0f;
                Player_SetupWalkSfx(this, 1.2f + ((REG(38) / 1000.0f) * temp2));
            }
            LinkAnimation_LoadToMorph(
                play,
                &this->skelAnime,
                GET_PLAYER_ANIM(PLAYER_ANIMGROUP_31, this->modelAnimType),
                this->unk_868
            );
            LinkAnimation_LoadToJoint(play, &this->skelAnime, &gPlayerAnim_link_normal_back_run, this->unk_868 * (16.0f / 29.0f));
        }
    }
    
    if (temp1 < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - temp1);
    }
}

void Player_SetupHaltFriendlyBackwalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_HaltFriendlyBackwalk, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, &gPlayerAnim_link_normal_back_brake);
}

s32 Player_SetupEndFriendlyBackwalk(Player* this, f32* arg1, s16* arg2, PlayState* play) {
    if (this->linearVelocity > 6.0f) {
        Player_SetupHaltFriendlyBackwalk(this, play);
        
        return 1;
    }
    
    if (*arg1 != 0.0f) {
        if (Player_StepLinearVelToZero(this)) {
            *arg1 = 0.0f;
            *arg2 = this->currentYaw;
        } else {
            return 1;
        }
    }
    
    return 0;
}

void Player_FriendlyBackwalk(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    s32 sp2C;
    s16 sp2A;
    
    Player_UpdateBackwalk(this, play);
    
    if (!Player_SetupAction(play, this, sAction_Backwalk_TargetFriendly, 1)) {
        if (!Player_IsZTargetingSetupStartEnemy(this)) {
            Player_SetupZTargetRunning(this, play, this->currentYaw);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.0f, play);
        sp2C = Player_GetFriendlyZTargetingMoveDirection(this, &sp34, &sp32, play);
        
        if (sp2C >= 0) {
            if (!Player_SetupEndFriendlyBackwalk(this, &sp34, &sp32, play)) {
                if (sp2C != 0) {
                    Player_SetupRun(this, play);
                } else if (sp34 > 4.9f) {
                    Player_SetupSidewalk(this, play);
                } else {
                    Player_SetupFriendlySidewalk(this, play);
                }
            }
        } else {
            sp2A = sp32 - this->currentYaw;
            
            Math_AsymStepToF(&this->linearVelocity, sp34 * 1.5f, 1.5f, 2.0f);
            Math_ScaledStepToS(&this->currentYaw, sp32, sp2A * 0.1f);
            
            if ((sp34 == 0.0f) && (this->linearVelocity == 0.0f)) {
                Player_SetupFriendlyZTargetingStandStill(this, play);
            }
        }
    }
}

void Player_SetupEndHaltFriendlyBackwalk(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_EndHaltFriendlyBackwalk, 1);
    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_back_brake_end);
}

void Player_HaltFriendlyBackwalk(Player* this, PlayState* play) {
    s32 sp34;
    f32 sp30;
    s16 sp2E;
    
    sp34 = LinkAnimation_Update(play, &this->skelAnime);
    Player_StepLinearVelToZero(this);
    
    if (!Player_SetupAction(play, this, sAction_Backwalk_TargetFriendly, 1)) {
        Player_GetTargetVelAndYaw(this, &sp30, &sp2E, 0.0f, play);
        
        if (this->linearVelocity == 0.0f) {
            this->currentYaw = this->actor.shape.rot.y;
            
            if (Player_GetFriendlyZTargetingMoveDirection(this, &sp30, &sp2E, play) > 0) {
                Player_SetupRun(this, play);
            } else if ((sp30 != 0.0f) || (sp34 != 0)) {
                Player_SetupEndHaltFriendlyBackwalk(this, play);
            }
        }
    }
}

void Player_EndHaltFriendlyBackwalk(Player* this, PlayState* play) {
    s32 sp1C;
    
    sp1C = LinkAnimation_Update(play, &this->skelAnime);
    
    if (!Player_SetupAction(play, this, sAction_Backwalk_TargetFriendly, 1)) {
        if (sp1C != 0) {
            Player_SetupFriendlyZTargetingStandStill(this, play);
        }
    }
}

void Player_SetupSidewalkAnims(PlayState* play, Player* this) {
    f32 frame;
    LinkAnimationHeader* sp38 = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_24, this->modelAnimType);
    LinkAnimationHeader* sp34 = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_25, this->modelAnimType);
    
    this->skelAnime.animation = sp38;
    
    Player_SetupWalkSfx(this, (REG(30) / 1000.0f) + ((REG(32) / 1000.0f) * this->linearVelocity));
    
    frame = this->unk_868 * (16.0f / 29.0f);
    LinkAnimation_BlendToJoint(play, &this->skelAnime, sp34, frame, sp38, frame, this->unk_870, this->blendTable);
}

void Player_Sidewalk(Player* this, PlayState* play) {
    f32 sp3C;
    s16 sp3A;
    s32 temp1;
    s16 temp2;
    s32 temp3;
    
    Player_SetupSidewalkAnims(play, this);
    
    if (!Player_SetupAction(play, this, sAction_Sidewalk, 1)) {
        if (!Player_IsZTargetingSetupStartEnemy(this)) {
            Player_SetupRun(this, play);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp3C, &sp3A, 0.0f, play);
        
        if (Player_IsFriendlyZTargeting(this)) {
            temp1 = Player_GetFriendlyZTargetingMoveDirection(this, &sp3C, &sp3A, play);
        } else {
            temp1 = Player_GetUnfriendlyZTargetMoveDirection(this, sp3C, sp3A);
        }
        
        if (temp1 > 0) {
            Player_SetupRun(this, play);
            
            return;
        }
        
        if (temp1 < 0) {
            if (Player_IsFriendlyZTargeting(this)) {
                Player_SetupFriendlyBackwalk(this, sp3A, play);
            } else {
                Player_SetupUnfriendlyBackwalk(this, sp3A, play);
            }
            
            return;
        }
        
        if ((this->linearVelocity < 3.6f) && (sp3C < 4.0f)) {
            if (!func_8008E9C4(this) && Player_IsFriendlyZTargeting(this)) {
                Player_SetupFriendlySidewalk(this, play);
            } else {
                Player_SetupStandingStillType(this, play);
            }
            
            return;
        }
        
        Player_SetLeftRightBlendWeight(this, sp3C, sp3A);
        
        temp2 = sp3A - this->currentYaw;
        temp3 = ABS(temp2);
        
        if (temp3 > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 3.0f) != 0) {
                this->currentYaw = sp3A;
            }
            
            return;
        }
        
        sp3C *= 0.9f;
        Math_AsymStepToF(&this->linearVelocity, sp3C, 2.0f, 3.0f);
        Math_ScaledStepToS(&this->currentYaw, sp3A, temp3 * 0.1f);
    }
}

void Player_Turn(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (Player_HoldsTwoHandedWeapon(this)) {
        AnimationContext_SetLoadFrame(
            play,
            Player_GetAnim_StandingStill(this),
            0,
            this->skelAnime.limbCount,
            this->skelAnime.morphTable
        );
        AnimationContext_SetCopyTrue(
            play,
            this->skelAnime.limbCount,
            this->skelAnime.jointTable,
            this->skelAnime.morphTable,
            D_80853410
        );
    }
    
    Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.018f, play);
    
    if (!Player_SetupAction(play, this, sAction_Turn, 1)) {
        if (sp34 != 0.0f) {
            this->actor.shape.rot.y = sp32;
            Player_SetupRun(this, play);
        } else if (Math_ScaledStepToS(&this->actor.shape.rot.y, sp32, this->unk_87E)) {
            Player_SetupStandingStillNoMorph(this, play);
        }
        
        this->currentYaw = this->actor.shape.rot.y;
    }
}

void Player_BlendWalkAnims(Player* this, s32 arg1, PlayState* play) {
    LinkAnimationHeader* anim;
    s16 target;
    f32 rate;
    
    if (ABS(sFloorPitch) < 3640) {
        target = 0;
    } else {
        target = CLAMP(sFloorPitch, -10922, 10922);
    }
    
    Math_ScaledStepToS(&this->unk_89C, target, 400);
    
    if ((this->modelAnimType == PLAYER_ANIMTYPE_3) || ((this->unk_89C == 0) && (this->unk_6C4 <= 0.0f))) {
        if (arg1 == 0) {
            LinkAnimation_LoadToJoint(
                play,
                &this->skelAnime,
                GET_PLAYER_ANIM(PLAYER_ANIMGROUP_1, this->modelAnimType),
                this->unk_868
            );
        } else {
            LinkAnimation_LoadToMorph(
                play,
                &this->skelAnime,
                GET_PLAYER_ANIM(PLAYER_ANIMGROUP_1, this->modelAnimType),
                this->unk_868
            );
        }
        
        return;
    }
    
    if (this->unk_89C != 0) {
        rate = this->unk_89C / 10922.0f;
    } else {
        rate = this->unk_6C4 * 0.0006f;
    }
    
    rate *= fabsf(this->linearVelocity) * 0.5f;
    
    if (rate > 1.0f) {
        rate = 1.0f;
    }
    
    if (rate < 0.0f) {
        anim = &gPlayerAnim_link_normal_climb_down;
        rate = -rate;
    } else {
        anim = &gPlayerAnim_link_normal_climb_up;
    }
    
    if (arg1 == 0) {
        LinkAnimation_BlendToJoint(
            play,
            &this->skelAnime,
            GET_PLAYER_ANIM(PLAYER_ANIMGROUP_1, this->modelAnimType),
            this->unk_868,
            anim,
            this->unk_868,
            rate,
            this->blendTable
        );
    } else {
        LinkAnimation_BlendToMorph(
            play,
            &this->skelAnime,
            GET_PLAYER_ANIM(PLAYER_ANIMGROUP_1, this->modelAnimType),
            this->unk_868,
            anim,
            this->unk_868,
            rate,
            this->blendTable
        );
    }
}

void Player_SetupWalkAnims(Player* this, PlayState* play) {
    f32 temp1;
    f32 temp2;
    
    if (this->unk_864 < 1.0f) {
        temp1 = R_UPDATE_RATE * 0.5f;
        
        Player_SetupWalkSfx(this, REG(35) / 1000.0f);
        LinkAnimation_LoadToJoint(
            play,
            &this->skelAnime,
            GET_PLAYER_ANIM(PLAYER_ANIMGROUP_1, this->modelAnimType),
            this->unk_868
        );
        
        this->unk_864 += 1 * temp1;
        if (this->unk_864 >= 1.0f) {
            this->unk_864 = 1.0f;
        }
        
        temp1 = this->unk_864;
    } else {
        temp2 = this->linearVelocity - (REG(48) / 100.0f);
        
        if (temp2 < 0.0f) {
            temp1 = 1.0f;
            Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            
            Player_BlendWalkAnims(this, 0, play);
        } else {
            temp1 = (REG(37) / 1000.0f) * temp2;
            if (temp1 < 1.0f) {
                Player_SetupWalkSfx(this, (REG(35) / 1000.0f) + ((REG(36) / 1000.0f) * this->linearVelocity));
            } else {
                temp1 = 1.0f;
                Player_SetupWalkSfx(this, 1.2f + ((REG(38) / 1000.0f) * temp2));
            }
            
            Player_BlendWalkAnims(this, 1, play);
            
            LinkAnimation_LoadToJoint(play, &this->skelAnime, Player_GetAnim_Running(this), this->unk_868 * (20.0f / 29.0f));
        }
    }
    
    if (temp1 < 1.0f) {
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 1.0f - temp1);
    }
}

void Player_Run(Player* this, PlayState* play) {
    f32 sp2C;
    s16 sp2A;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    Player_SetupWalkAnims(this, play);
    
    if (!Player_SetupAction(play, this, sAction_Run, 1)) {
        if (Player_IsZTargetingSetupStartEnemy(this)) {
            Player_SetupRun(this, play);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp2C, &sp2A, 0.018f, play);
        
        if (!Player_SetupTurnAroundRunning(this, &sp2C, &sp2A)) {
            Player_SetRunVelAndYaw(this, sp2C, sp2A);
            func_8083DDC8(this, play);
            
            if ((this->linearVelocity == 0.0f) && (sp2C == 0.0f)) {
                Player_SetupEndRun(this, play);
            }
        }
    }
}

void Player_ZTargetingRun(Player* this, PlayState* play) {
    f32 sp2C;
    s16 sp2A;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    Player_SetupWalkAnims(this, play);
    
    if (!Player_SetupAction(play, this, sAction_Run_Targeting, 1)) {
        if (!Player_IsZTargetingSetupStartEnemy(this)) {
            Player_SetupRun(this, play);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp2C, &sp2A, 0.0f, play);
        
        if (!Player_SetupTurnAroundRunning(this, &sp2C, &sp2A)) {
            if (
                (Player_IsFriendlyZTargeting(this) && (sp2C != 0.0f) && (Player_GetFriendlyZTargetingMoveDirection(this, &sp2C, &sp2A, play) <= 0)) ||
                (!Player_IsFriendlyZTargeting(this) && (Player_GetUnfriendlyZTargetMoveDirection(this, sp2C, sp2A) <= 0))
            ) {
                Player_SetupStandingStillType(this, play);
                
                return;
            }
            
            Player_SetRunVelAndYaw(this, sp2C, sp2A);
            func_8083DDC8(this, play);
            
            if ((this->linearVelocity == 0) && (sp2C == 0)) {
                Player_SetupStandingStillType(this, play);
            }
        }
    }
}

void Player_UnfriendlyBackwalk(Player* this, PlayState* play) {
    s32 sp34;
    f32 sp30;
    s16 sp2E;
    
    sp34 = LinkAnimation_Update(play, &this->skelAnime);
    
    if (!Player_SetupAction(play, this, sAction_Sidewalk, 1)) {
        if (!Player_IsZTargetingSetupStartEnemy(this)) {
            Player_SetupRun(this, play);
            
            return;
        }
        
        Player_GetTargetVelAndYaw(this, &sp30, &sp2E, 0.0f, play);
        
        if ((this->skelAnime.morphWeight == 0.0f) && (this->skelAnime.curFrame > 5.0f)) {
            Player_StepLinearVelToZero(this);
            
            if ((this->skelAnime.curFrame > 10.0f) && (Player_GetUnfriendlyZTargetMoveDirection(this, sp30, sp2E) < 0)) {
                Player_SetupUnfriendlyBackwalk(this, sp2E, play);
                
                return;
            }
            
            if (sp34 != 0) {
                Player_SetupEndUnfriendlyBackwalk(this, play);
            }
        }
    }
}

void Player_EndUnfriendlyBackwalk(Player* this, PlayState* play) {
    s32 sp34;
    f32 sp30;
    s16 sp2E;
    
    sp34 = LinkAnimation_Update(play, &this->skelAnime);
    
    Player_StepLinearVelToZero(this);
    
    if (!Player_SetupAction(play, this, sAction_BackwalkStop, 1)) {
        Player_GetTargetVelAndYaw(this, &sp30, &sp2E, 0.0f, play);
        
        if (this->linearVelocity == 0.0f) {
            this->currentYaw = this->actor.shape.rot.y;
            
            if (Player_GetUnfriendlyZTargetMoveDirection(this, sp30, sp2E) > 0) {
                Player_SetupRun(this, play);
                
                return;
            }
            
            if ((sp30 != 0.0f) || (sp34 != 0)) {
                Player_SetupStandingStillType(this, play);
            }
        }
    }
}

void Player_GetDustPos(Vec3f* src, Vec3f* dest, f32 arg2, f32 arg3, f32 arg4) {
    dest->x = (Rand_ZeroOne() * arg3) + src->x;
    dest->y = (Rand_ZeroOne() * arg4) + (src->y + arg2);
    dest->z = (Rand_ZeroOne() * arg3) + src->z;
}

static Vec3f D_808545B4 = { 0.0f, 0.0f, 0.0f };
static Vec3f D_808545C0 = { 0.0f, 0.0f, 0.0f };

s32 Player_SetupSpawnDustAtFeet(PlayState* play, Player* this) {
    Vec3f sp2C;
    
    if ((this->unk_89E == 0) || (this->unk_89E == 1)) {
        Player_GetDustPos(
            &this->actor.shape.feetPos[FOOT_LEFT],
            &sp2C,
            this->actor.floorHeight - this->actor.shape.feetPos[FOOT_LEFT].y,
            7.0f,
            5.0f
        );
        func_800286CC(play, &sp2C, &D_808545B4, &D_808545C0, 50, 30);
        Player_GetDustPos(
            &this->actor.shape.feetPos[FOOT_RIGHT],
            &sp2C,
            this->actor.floorHeight - this->actor.shape.feetPos[FOOT_RIGHT].y,
            7.0f,
            5.0f
        );
        func_800286CC(play, &this->actor.shape.feetPos[FOOT_RIGHT], &D_808545B4, &D_808545C0, 50, 30);
        
        return 1;
    }
    
    return 0;
}

void func_8084279C(Player* this, PlayState* play) {
    Player_LoopAnimContinuously(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_33, this->modelAnimType));
    
    if (DECR(this->unk_850) == 0) {
        if (!Player_SetupItemCsOrFirstPerson(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_34, this->modelAnimType), play);
        }
        
        this->actor.flags &= ~ACTOR_FLAG_8;
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    }
}

s32 Player_SetupMeleeAttack(Player* this, f32 arg1, f32 arg2, f32 arg3) {
    if ((arg1 <= this->skelAnime.curFrame) && (this->skelAnime.curFrame <= arg3)) {
        Player_MeleeAttack(this, (arg2 <= this->skelAnime.curFrame) ? 1 : -1);
        
        return 1;
    }
    
    Player_InactivateMeleeWeapon(this);
    
    return 0;
}

s32 Player_AttackWhileDefending(Player* this, PlayState* play) {
    if (!Player_IsChildWithHylianShield(this) && (Player_GetMeleeWeaponHeld(this) != 0) && sActiveItemUseFlag) {
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_defense_kiru);
        this->unk_84F = 1;
        this->meleeWeaponAnimation = PLAYER_MWA_STAB_1H;
        this->currentYaw = this->actor.shape.rot.y + this->unk_6BE;
        
        return 1;
    }
    
    return 0;
}

s32 Player_IsBusy(Player* this, PlayState* play) {
    return Player_SetupItemCsOrFirstPerson(this, play) || Player_SetupSpeakOrCheck(this, play) || Player_SetupGetItemOrHoldBehavior(this, play);
}

void Player_SetQuake(PlayState* play, s32 speed, s32 y, s32 countdown) {
    s32 quakeIdx = Quake_Add(Play_GetCamera(play, CAM_ID_MAIN), 3);
    
    Quake_SetSpeed(quakeIdx, speed);
    Quake_SetQuakeValues(quakeIdx, y, 0, 0, 0);
    Quake_SetCountdown(quakeIdx, countdown);
}

void Player_SetupHammerHit(PlayState* play, Player* this) {
    Player_SetQuake(play, 27767, 7, 20);
    play->actorCtx.unk_02 = 4;
    Player_SetRumble(this, 255, 20, 150, 0);
    func_8002F7DC(&this->actor, NA_SE_IT_HAMMER_HIT);
}

void func_80842A88(PlayState* play, Player* this) {
    Inventory_ChangeAmmo(ITEM_STICK, -1);
    Player_UseItem(play, this, ITEM_NONE);
}

s32 func_80842AC4(PlayState* play, Player* this) {
    if ((this->heldItemActionParam == PLAYER_AP_STICK) && (this->unk_85C > 0.5f)) {
        if (AMMO(ITEM_STICK) != 0) {
            EffectSsStick_Spawn(play, &this->bodyPartsPos[PLAYER_BODYPART_R_HAND], this->actor.shape.rot.y + 0x8000);
            this->unk_85C = 0.5f;
            func_80842A88(play, this);
            func_8002F7DC(&this->actor, NA_SE_IT_WOODSTICK_BROKEN);
        }
        
        return 1;
    }
    
    return 0;
}

s32 func_80842B7C(PlayState* play, Player* this) {
    if (this->heldItemActionParam == PLAYER_AP_SWORD_BGS) {
        if (!gSaveContext.bgsFlag && (gSaveContext.swordHealth > 0.0f)) {
            if ((gSaveContext.swordHealth -= 1.0f) <= 0.0f) {
                EffectSsStick_Spawn(
                    play,
                    &this->bodyPartsPos[PLAYER_BODYPART_R_HAND],
                    this->actor.shape.rot.y + 0x8000
                );
                func_800849EC(play);
                func_8002F7DC(&this->actor, NA_SE_IT_MAJIN_SWORD_BROKEN);
            }
        }
        
        return 1;
    }
    
    return 0;
}

void func_80842CF0(PlayState* play, Player* this) {
    func_80842AC4(play, this);
    func_80842B7C(play, this);
}

static LinkAnimationHeader* D_808545CC[] = {
    &gPlayerAnim_link_fighter_rebound,
    &gPlayerAnim_link_fighter_rebound_long,
    &gPlayerAnim_link_fighter_reboundR,
    &gPlayerAnim_link_fighter_rebound_longR,
};

void Player_SetupMeleeWeaponRebound(PlayState* play, Player* this) {
    s32 sp28;
    
    if (Player_AimShieldCrouched != this->func_674) {
        Player_ResetAttributes(play, this);
        Player_SetActionFunc(play, this, Player_MeleeWeaponRebound, 0);
        
        if (func_8008E9C4(this)) {
            sp28 = 2;
        } else {
            sp28 = 0;
        }
        
        Player_PlayAnimOnceSlowed(play, this, D_808545CC[Player_HoldsTwoHandedWeapon(this) + sp28]);
    }
    
    Player_SetRumble(this, 180, 20, 100, 0);
    this->linearVelocity = -18.0f;
    func_80842CF0(play, this);
}

s32 func_80842DF4(PlayState* play, Player* this) {
    f32 phi_f2;
    CollisionPoly* sp78;
    s32 sp74;
    Vec3f sp68;
    Vec3f sp5C;
    Vec3f sp50;
    s32 temp1;
    s32 sp48;
    
    if (this->meleeWeaponState > 0) {
        if (this->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) {
            if (
                !(this->meleeWeaponQuads[0].base.atFlags & AT_BOUNCED) &&
                !(this->meleeWeaponQuads[1].base.atFlags & AT_BOUNCED)
            ) {
                if (this->skelAnime.curFrame >= 2.0f) {
                    phi_f2 = Math_Vec3f_DistXYZAndStoreDiff(
                        &this->meleeWeaponInfo[0].tip,
                        &this->meleeWeaponInfo[0].base,
                        &sp50
                    );
                    if (phi_f2 != 0.0f) {
                        phi_f2 = (phi_f2 + 10.0f) / phi_f2;
                    }
                    
                    sp68.x = this->meleeWeaponInfo[0].tip.x + (sp50.x * phi_f2);
                    sp68.y = this->meleeWeaponInfo[0].tip.y + (sp50.y * phi_f2);
                    sp68.z = this->meleeWeaponInfo[0].tip.z + (sp50.z * phi_f2);
                    
                    if (
                        BgCheck_EntityLineTest1(
                            &play->colCtx,
                            &sp68,
                            &this->meleeWeaponInfo[0].tip,
                            &sp5C,
                            &sp78,
                            true,
                            false,
                            false,
                            true,
                            &sp74
                        ) &&
                        !SurfaceType_IsIgnoredByEntities(&play->colCtx, sp78, sp74) &&
                        (SurfaceType_GetFloorType(&play->colCtx, sp78, sp74) != 6) &&
                        (func_8002F9EC(play, &this->actor, sp78, sp74, &sp5C) == 0)
                    ) {
                        if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
                            Player_SetFreezeFlashTimer(play);
                            Player_SetupHammerHit(play, this);
                            Player_SetupMeleeWeaponRebound(play, this);
                            
                            return 1;
                        }
                        
                        if (this->linearVelocity >= 0.0f) {
                            sp48 = SurfaceType_GetSfxType(&play->colCtx, sp78, sp74);
                            
                            if (sp48 == 0xA) {
                                CollisionCheck_SpawnShieldParticlesWood(play, &sp5C, &this->actor.projectedPos);
                            } else {
                                CollisionCheck_SpawnShieldParticles(play, &sp5C);
                                if (sp48 == 0xB) {
                                    func_8002F7DC(&this->actor, NA_SE_IT_WALL_HIT_SOFT);
                                } else {
                                    func_8002F7DC(&this->actor, NA_SE_IT_WALL_HIT_HARD);
                                }
                            }
                            
                            func_80842CF0(play, this);
                            this->linearVelocity = -14.0f;
                            Player_SetRumble(this, 180, 20, 100, 0);
                        }
                    }
                }
            } else {
                Player_SetupMeleeWeaponRebound(play, this);
                Player_SetFreezeFlashTimer(play);
                
                return 1;
            }
        }
        
        temp1 = (this->meleeWeaponQuads[0].base.atFlags & AT_HIT) || (this->meleeWeaponQuads[1].base.atFlags & AT_HIT);
        
        if (temp1) {
            if (this->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) {
                Actor* at = this->meleeWeaponQuads[temp1 ? 1 : 0].base.at;
                
                if ((at != NULL) && (at->id != ACTOR_EN_KANBAN)) {
                    Player_SetFreezeFlashTimer(play);
                }
            }
            
            if ((func_80842AC4(play, this) == 0) && (this->heldItemActionParam != PLAYER_AP_HAMMER)) {
                func_80842B7C(play, this);
                
                if (this->actor.colChkInfo.atHitEffect == 1) {
                    this->actor.colChkInfo.damage = 8;
                    Player_SetupDamage(play, this, 4, 0.0f, 0.0f, this->actor.shape.rot.y, 20);
                    
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

void Player_AimShieldCrouched(Player* this, PlayState* play) {
    f32 sp54;
    f32 sp50;
    s16 sp4E;
    s16 sp4C;
    s16 sp4A;
    s16 sp48;
    s16 sp46;
    f32 sp40;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_IsChildWithHylianShield(this)) {
            Player_PlayAnimLoop(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_21, this->modelAnimType));
        }
        this->unk_850 = 1;
        this->unk_84F = 0;
    }
    
    if (!Player_IsChildWithHylianShield(this)) {
        this->stateFlags1 |= PLAYER_STATE1_22;
        Player_SetupCurrentUpperAction(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_22;
    }
    
    Player_StepLinearVelToZero(this);
    
    if (this->unk_850 != 0) {
        sp54 = sControlInput->rel.stick_y * 100;
        sp50 = sControlInput->rel.stick_x * -120;
        sp4E = this->actor.shape.rot.y - Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
        
        sp40 = Math_CosS(sp4E);
        sp4C = (Math_SinS(sp4E) * sp50) + (sp54 * sp40);
        sp40 = Math_CosS(sp4E);
        sp4A = (sp50 * sp40) - (Math_SinS(sp4E) * sp54);
        
        if (sp4C > 3500) {
            sp4C = 3500;
        }
        
        sp48 = ABS(sp4C - this->actor.focus.rot.x) * 0.25f;
        if (sp48 < 100) {
            sp48 = 100;
        }
        
        sp46 = ABS(sp4A - this->unk_6BE) * 0.25f;
        if (sp46 < 50) {
            sp46 = 50;
        }
        
        Math_ScaledStepToS(&this->actor.focus.rot.x, sp4C, sp48);
        this->unk_6BC = this->actor.focus.rot.x;
        Math_ScaledStepToS(&this->unk_6BE, sp4A, sp46);
        
        if (this->unk_84F != 0) {
            if (!func_80842DF4(play, this)) {
                if (this->skelAnime.curFrame < 2.0f) {
                    Player_MeleeAttack(this, 1);
                }
            } else {
                this->unk_850 = 1;
                this->unk_84F = 0;
            }
        } else if (!Player_IsBusy(this, play)) {
            if (Player_SetupDefend(this, play)) {
                Player_AttackWhileDefending(this, play);
            } else {
                this->stateFlags1 &= ~PLAYER_STATE1_22;
                Player_InactivateMeleeWeapon(this);
                
                if (Player_IsChildWithHylianShield(this)) {
                    Player_SetupReturnToStandStill(this, play);
                    LinkAnimation_Change(
                        play,
                        &this->skelAnime,
                        &gPlayerAnim_clink_normal_defense_ALL,
                        1.0f,
                        Animation_GetLastFrame(&gPlayerAnim_clink_normal_defense_ALL),
                        0.0f,
                        ANIMMODE_ONCE,
                        0.0f
                    );
                    Player_SetupAnimMovement(play, this, 4);
                } else {
                    if (this->itemActionParam < 0) {
                        func_8008EC70(this);
                    }
                    Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_22, this->modelAnimType), play);
                }
                
                func_8002F7DC(&this->actor, NA_SE_IT_SHIELD_REMOVE);
                
                return;
            }
        } else {
            return;
        }
    }
    
    this->stateFlags1 |= PLAYER_STATE1_22;
    Player_SetModelsForHoldingShield(this);
    
    this->unk_6AE |= 0xC1;
}

void Player_DeflectAttackWithShield(Player* this, PlayState* play) {
    s32 temp;
    LinkAnimationHeader* anim;
    f32 frames;
    
    Player_StepLinearVelToZero(this);
    
    if (this->unk_84F == 0) {
        D_808535E0 = Player_SetupCurrentUpperAction(this, play);
        if ((Player_StandingDefend == this->func_82C) || (Player_IsActionInterrupted(play, this, &this->skelAnime2, 4.0f) > 0)) {
            Player_SetActionFunc(play, this, Player_UnfriendlyZTargetStandingStill, 1);
        }
    } else {
        temp = Player_IsActionInterrupted(play, this, &this->skelAnime, 4.0f);
        if ((temp != 0) && ((temp > 0) || LinkAnimation_Update(play, &this->skelAnime))) {
            Player_SetActionFunc(play, this, Player_AimShieldCrouched, 1);
            this->stateFlags1 |= PLAYER_STATE1_22;
            Player_SetModelsForHoldingShield(this);
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_20, this->modelAnimType);
            frames = Animation_GetLastFrame(anim);
            LinkAnimation_Change(play, &this->skelAnime, anim, 1.0f, frames, frames, ANIMMODE_ONCE, 0.0f);
        }
    }
}

void func_8084370C(Player* this, PlayState* play) {
    s32 sp1C;
    
    Player_StepLinearVelToZero(this);
    
    sp1C = Player_IsActionInterrupted(play, this, &this->skelAnime, 16.0f);
    if ((sp1C != 0) && (LinkAnimation_Update(play, &this->skelAnime) || (sp1C > 0))) {
        Player_SetupStandingStillType(this, play);
    }
}

void Player_StartKnockback(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5 | PLAYER_STATE2_6;
    
    Player_RoundUpInvincibilityTimer(this);
    
    if (!(this->stateFlags1 & PLAYER_STATE1_29) && (this->unk_850 == 0) && (this->unk_8A1 != 0)) {
        s16 temp = this->actor.shape.rot.y - this->unk_8A2;
        
        this->currentYaw = this->actor.shape.rot.y = this->unk_8A2;
        this->linearVelocity = this->unk_8A4;
        
        if (ABS(temp) > 0x4000) {
            this->actor.shape.rot.y = this->unk_8A2 + 0x8000;
        }
        
        if (this->actor.velocity.y < 0.0f) {
            this->actor.gravity = 0.0f;
            this->actor.velocity.y = 0.0f;
        }
    }
    
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (this->unk_850 != 0) {
            this->unk_850--;
            if (this->unk_850 == 0) {
                Player_SetupStandingStillMorph(this, play);
            }
        } else if (
            (this->stateFlags1 & PLAYER_STATE1_29) ||
            (!(this->cylinder.base.acFlags & AC_HIT) && (this->unk_8A1 == 0))
        ) {
            if (this->stateFlags1 & PLAYER_STATE1_29) {
                this->unk_850++;
            } else {
                Player_SetActionFunc(play, this, Player_DownFromKnockback, 0);
                this->stateFlags1 |= PLAYER_STATE1_26;
            }
            
            Player_PlayAnimOnce(
                play,
                this,
                (this->currentYaw != this->actor.shape.rot.y) ? &gPlayerAnim_link_normal_front_downB : &gPlayerAnim_link_normal_back_downB
            );
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FREEZE);
        }
    }
    
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND_TOUCH) {
        Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
    }
}

void Player_DownFromKnockback(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5 | PLAYER_STATE2_6;
    Player_RoundUpInvincibilityTimer(this);
    
    Player_StepLinearVelToZero(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->linearVelocity == 0.0f)) {
        if (this->stateFlags1 & PLAYER_STATE1_29) {
            this->unk_850++;
        } else {
            Player_SetActionFunc(play, this, Player_GetUpFromKnockback, 0);
            this->stateFlags1 |= PLAYER_STATE1_26;
        }
        
        Player_PlayAnimOnceSlowed(
            play,
            this,
            (this->currentYaw != this->actor.shape.rot.y) ? &gPlayerAnim_link_normal_front_down_wake : &gPlayerAnim_link_normal_back_down_wake
        );
        this->currentYaw = this->actor.shape.rot.y;
    }
}

void Player_GetUpFromKnockback(Player* this, PlayState* play) {
    static struct_80832924 sAnimSfx_KnockbackGetUp[] = {
        { 0, 0x4014  },
        { 0, -0x401E },
    };
    s32 sp24;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    Player_RoundUpInvincibilityTimer(this);
    
    if (this->stateFlags1 & PLAYER_STATE1_29) {
        LinkAnimation_Update(play, &this->skelAnime);
    } else {
        sp24 = Player_IsActionInterrupted(play, this, &this->skelAnime, 16.0f);
        if ((sp24 != 0) && (LinkAnimation_Update(play, &this->skelAnime) || (sp24 > 0))) {
            Player_SetupStandingStillType(this, play);
        }
    }
    
    Player_PlayAnimSfx(this, sAnimSfx_KnockbackGetUp);
}

void Player_EndDie(PlayState* play, Player* this) {
    static Vec3f sDeathReviveFairyPos = { 0.0f, 0.0f, 5.0f };
    
    if (this->unk_850 != 0) {
        if (this->unk_850 > 0) {
            this->unk_850--;
            if (this->unk_850 == 0) {
                if (this->stateFlags1 & PLAYER_STATE1_27) {
                    LinkAnimation_Change(
                        play,
                        &this->skelAnime,
                        &gPlayerAnim_link_swimer_swim_wait,
                        1.0f,
                        0.0f,
                        Animation_GetLastFrame(&gPlayerAnim_link_swimer_swim_wait),
                        ANIMMODE_ONCE,
                        -16.0f
                    );
                } else {
                    LinkAnimation_Change(
                        play,
                        &this->skelAnime,
                        &gPlayerAnim_link_derth_rebirth,
                        1.0f,
                        99.0f,
                        Animation_GetLastFrame(&gPlayerAnim_link_derth_rebirth),
                        ANIMMODE_ONCE,
                        0.0f
                    );
                }
                gSaveContext.healthAccumulator = 0x140;
                this->unk_850 = -1;
            }
        } else if (gSaveContext.healthAccumulator == 0) {
            this->stateFlags1 &= ~PLAYER_STATE1_7;
            if (this->stateFlags1 & PLAYER_STATE1_27) {
                Player_SetupSwimIdle(play, this);
            } else {
                Player_SetupStandingStillMorph(this, play);
            }
            this->unk_A87 = 20;
            Player_SetupInvincibility_NoDamageFlash(this, -20);
            func_800F47FC();
        }
    } else if (this->unk_84F != 0) {
        this->unk_850 = 60;
        Player_SpawnFairy(play, this, &this->actor.world.pos, &sDeathReviveFairyPos, FAIRY_REVIVE_DEATH);
        func_8002F7DC(&this->actor, NA_SE_EV_FIATY_HEAL - SFX_FLAG);
        OnePointCutscene_Init(play, 9908, 125, &this->actor, CAM_ID_MAIN);
    } else if (play->gameOverCtx.state == GAMEOVER_DEATH_WAIT_GROUND) {
        play->gameOverCtx.state = GAMEOVER_DEATH_DELAY_MENU;
    }
}

void Player_Die(Player* this, PlayState* play) {
    static struct_80832924 sAnimSfx_DyingOrReviving[] = {
        { NA_SE_PL_BOUND, 0x103C               },
        { 0,              0x408C               },
        { 0,              0x40A4               },
        { 0,              -0x40AA              },
    };
    
    if (this->currentTunic != PLAYER_TUNIC_GORON) {
        if (
            (play->roomCtx.curRoom.behaviorType2 == ROOM_BEHAVIOR_TYPE2_3) || (sFloorSpecialProperty == 9) ||
            ((Player_GetFloorDamageType(sFloorSpecialProperty) >= PLAYER_FLOODRAMAGE_SPIKE) &&
            !func_80042108(&play->colCtx, this->actor.floorPoly, this->actor.floorBgId))
        ) {
            Player_SetLinkBurning(this);
        }
    }
    
    Player_StepLinearVelToZero(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->actor.category == ACTORCAT_PLAYER) {
            Player_EndDie(play, this);
        }
        
        return;
    }
    
    if (this->skelAnime.animation == &gPlayerAnim_link_derth_rebirth) {
        Player_PlayAnimSfx(this, sAnimSfx_DyingOrReviving);
    } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_electric_shock_end) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 88.0f)) {
            Player_PlayMoveSfx(this, NA_SE_PL_BOUND);
        }
    }
}

void Player_PlayFallingVoiceSfx(Player* this, u16 sfxId) {
    Player_PlayVoiceSfxForAge(this, sfxId);
    
    if ((this->heldActor != NULL) && (this->heldActor->id == ACTOR_EN_RU1)) {
        Audio_PlayActorSfx2(this->heldActor, NA_SE_VO_RT_FALL);
    }
}

s32 Player_SetupFallLanding(PlayState* play, Player* this) {
    static FallImpactInfo sFallImpactInfo[] = {
        { -8,  180,  40,  100,  NA_SE_VO_LI_LAND_DAMAGE_S },
        { -16, 255,  140, 150,  NA_SE_VO_LI_LAND_DAMAGE_S },
    };
    s32 sp34;
    
    if ((sFloorSpecialProperty == 6) || (sFloorSpecialProperty == 9)) {
        sp34 = 0;
    } else {
        sp34 = this->fallDistance;
    }
    
    Math_StepToF(&this->linearVelocity, 0.0f, 1.0f);
    
    this->stateFlags1 &= ~(PLAYER_STATE1_18 | PLAYER_STATE1_19);
    
    if (sp34 >= 400) {
        s32 impactIndex;
        FallImpactInfo* impactInfo;
        
        if (this->fallDistance < 800) {
            impactIndex = 0;
        } else {
            impactIndex = 1;
        }
        
        impactInfo = &sFallImpactInfo[impactIndex];
        
        if (Player_InflictDamage(play, impactInfo->damage)) {
            return -1;
        }
        
        Player_SetupInvincibility(this, 40);
        Player_SetQuake(play, 32967, 2, 30);
        Player_SetRumble(this, impactInfo->unk_01, impactInfo->unk_02, impactInfo->unk_03, 0);
        func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
        Player_PlayVoiceSfxForAge(this, impactInfo->sfxId);
        
        return impactIndex + 1;
    }
    
    if (sp34 > 200) {
        sp34 *= 2;
        
        if (sp34 > 255) {
            sp34 = 255;
        }
        
        Player_SetRumble(this, (u8)sp34, (u8)(sp34 * 0.1f), (u8)sp34, 0);
        
        if (sFloorSpecialProperty == 6) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }
    }
    
    Player_PlayLandingSfx(this);
    
    return 0;
}

void Player_ThrowActor(PlayState* play, Player* this, f32 speedXZ, f32 velocityY) {
    Actor* heldActor = this->heldActor;
    
    if (!Player_InterruptHoldingActor(play, this, heldActor)) {
        heldActor->world.rot.y = this->actor.shape.rot.y;
        heldActor->speedXZ = speedXZ;
        heldActor->velocity.y = velocityY;
        Player_SetupHeldItemUpperActionFunc(play, this);
        func_8002F7DC(&this->actor, NA_SE_PL_THROW);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }
}

void Player_UpdateMidair(Player* this, PlayState* play) {
    f32 sp4C;
    s16 sp4A;
    
    if (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 40) {
        this->actor.gravity = 0.0f;
    } else if (func_8008E9C4(this)) {
        this->actor.gravity = -1.2f;
    }
    
    Player_GetTargetVelAndYaw(this, &sp4C, &sp4A, 0.0f, play);
    
    if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        if (this->stateFlags1 & PLAYER_STATE1_11) {
            Actor* heldActor = this->heldActor;
            
            if (
                !Player_InterruptHoldingActor(play, this, heldActor) && (heldActor->id == ACTOR_EN_NIW) &&
                CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)
            ) {
                Player_ThrowActor(play, this, this->linearVelocity + 2.0f, this->actor.velocity.y + 2.0f);
            }
        }
        
        LinkAnimation_Update(play, &this->skelAnime);
        
        if (!(this->stateFlags2 & PLAYER_STATE2_19)) {
            func_8083DFE0(this, &sp4C, &sp4A);
        }
        
        Player_SetupCurrentUpperAction(this, play);
        
        if (((this->stateFlags2 & PLAYER_STATE2_19) && (this->unk_84F == 2)) || !Player_SetupMidairJumpSlash(this, play)) {
            if (this->actor.velocity.y < 0.0f) {
                if (this->unk_850 >= 0) {
                    if (
                        (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) || (this->unk_850 == 0) ||
                        (this->fallDistance > 0)
                    ) {
                        if ((sFloorDistY > 800.0f) || (this->stateFlags1 & PLAYER_STATE1_2)) {
                            Player_PlayFallingVoiceSfx(this, NA_SE_VO_LI_FALL_S);
                            this->stateFlags1 &= ~PLAYER_STATE1_2;
                        }
                        
                        LinkAnimation_Change(
                            play,
                            &this->skelAnime,
                            &gPlayerAnim_link_normal_landing,
                            1.0f,
                            0.0f,
                            0.0f,
                            ANIMMODE_ONCE,
                            8.0f
                        );
                        this->unk_850 = -1;
                    }
                } else {
                    if ((this->unk_850 == -1) && (this->fallDistance > 120.0f) && (sFloorDistY > 280.0f)) {
                        this->unk_850 = -2;
                        Player_PlayFallingVoiceSfx(this, NA_SE_VO_LI_FALL_L);
                    }
                    
                    if (
                        (this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) &&
                        !(this->stateFlags2 & PLAYER_STATE2_19) &&
                        !(this->stateFlags1 & (PLAYER_STATE1_11 | PLAYER_STATE1_27)) && (this->linearVelocity > 0.0f)
                    ) {
                        if ((this->wallHeight >= 150.0f) && (this->unk_84B[this->unk_846] == 0)) {
                            Player_SetupClimbWallOrLadder(this, play, sTouchedWallFlags);
                        } else if (
                            (this->unk_88C >= 2) && (this->wallHeight < 150.0f) &&
                            (((this->actor.world.pos.y - this->actor.floorHeight) + this->wallHeight) >
                            (70.0f * this->ageProperties->unk_08))
                        ) {
                            AnimationContext_DisableQueue(play);
                            if (this->stateFlags1 & PLAYER_STATE1_2) {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HOOKSHOT_HANG);
                            } else {
                                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HANG);
                            }
                            this->actor.world.pos.y += this->wallHeight;
                            Player_SetupGrabLedge(
                                play,
                                this,
                                this->actor.wallPoly,
                                this->wallDistance,
                                GET_PLAYER_ANIM(PLAYER_ANIMGROUP_39, this->modelAnimType)
                            );
                            this->actor.shape.rot.y = this->currentYaw += 0x8000;
                            this->stateFlags1 |= PLAYER_STATE1_13;
                        }
                    }
                }
            }
        }
    } else {
        LinkAnimationHeader* anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_14, this->modelAnimType);
        s32 sp3C;
        
        if (this->stateFlags2 & PLAYER_STATE2_19) {
            if (func_8008E9C4(this)) {
                anim = sAnims_ManualJump[this->unk_84F][2];
            } else {
                anim = sAnims_ManualJump[this->unk_84F][1];
            }
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_run_jump) {
            anim = &gPlayerAnim_link_normal_run_jump_end;
        } else if (func_8008E9C4(this)) {
            anim = &gPlayerAnim_link_anchor_landingR;
            Player_ResetLeftRightBlendWeight(this);
        } else if (this->fallDistance <= 80) {
            anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_15, this->modelAnimType);
        } else if (
            (this->fallDistance < 800) && (this->unk_84B[this->unk_846] == 0) &&
            !(this->stateFlags1 & PLAYER_STATE1_11)
        ) {
            Player_SetupRolling(this, play);
            
            return;
        }
        
        sp3C = Player_SetupFallLanding(play, this);
        
        if (sp3C > 0) {
            Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_14, this->modelAnimType), play);
            this->skelAnime.endFrame = 8.0f;
            if (sp3C == 1) {
                this->unk_850 = 10;
            } else {
                this->unk_850 = 20;
            }
        } else if (sp3C == 0) {
            Player_SetupReturnToStandStillSetAnim(this, anim, play);
        }
    }
}

void Player_Rolling(Player* this, PlayState* play) {
    static struct_80832924 sAnimSfx_Roll[] = {
        { NA_SE_VO_LI_SWORD_N,  0x2001                     },
        { NA_SE_PL_WALK_GROUND, 0x1806                     },
        { NA_SE_PL_ROLL,        0x806                      },
        { 0,                    -0x2812                    },
    };
    Actor* cylinderOc;
    s32 temp;
    s32 sp44;
    DynaPolyActor* wallPolyActor;
    f32 sp38;
    s16 sp36;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    cylinderOc = NULL;
    sp44 = LinkAnimation_Update(play, &this->skelAnime);
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 8.0f)) {
        Player_SetupInvincibility_NoDamageFlash(this, -10);
    }
    
    if (Player_IsBusy(this, play) == 0) {
        if (this->unk_850 != 0) {
            Math_StepToF(&this->linearVelocity, 0.0f, 2.0f);
            
            temp = Player_IsActionInterrupted(play, this, &this->skelAnime, 5.0f);
            if ((temp != 0) && ((temp > 0) || sp44)) {
                Player_SetupReturnToStandStill(this, play);
            }
        } else {
            if (this->linearVelocity >= 7.0f) {
                if (
                    ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sTouchedWallYaw2 < 0x2000)) ||
                    ((this->cylinder.base.ocFlags1 & OC1_HIT) &&
                    (cylinderOc = this->cylinder.base.oc,
                    ((cylinderOc->id == ACTOR_EN_WOOD02) &&
                    (ABS((s16)(this->actor.world.rot.y - cylinderOc->yawTowardsPlayer)) > 0x6000))))
                ) {
                    if (cylinderOc != NULL) {
                        cylinderOc->home.rot.y = 1;
                    } else if (this->actor.wallBgId != BGCHECK_SCENE) {
                        wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
                        if ((wallPolyActor != NULL) && (wallPolyActor->actor.id == ACTOR_OBJ_KIBAKO2)) {
                            wallPolyActor->actor.home.rot.z = 1;
                        }
                    }
                    
                    Player_PlayAnimOnce(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_17, this->modelAnimType));
                    this->linearVelocity = -this->linearVelocity;
                    Player_SetQuake(play, 33267, 3, 12);
                    Player_SetRumble(this, 255, 20, 150, 0);
                    func_8002F7DC(&this->actor, NA_SE_PL_BODY_HIT);
                    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
                    this->unk_850 = 1;
                    
                    return;
                }
            }
            
            if ((this->skelAnime.curFrame < 15.0f) || !Player_SetupStartMeleeWeaponAttack(this, play)) {
                if (this->skelAnime.curFrame >= 20.0f) {
                    Player_SetupReturnToStandStill(this, play);
                    
                    return;
                }
                
                Player_GetTargetVelAndYaw(this, &sp38, &sp36, 0.018f, play);
                
                sp38 *= 1.5f;
                if ((sp38 < 3.0f) || (this->unk_84B[this->unk_846] != 0)) {
                    sp38 = 3.0f;
                }
                
                Player_SetRunVelAndYaw(this, sp38, this->actor.shape.rot.y);
                
                if (Player_SetupSpawnDustAtFeet(play, this)) {
                    func_8002F8F0(&this->actor, NA_SE_PL_ROLL_DUST - SFX_FLAG);
                }
                
                Player_PlayAnimSfx(this, sAnimSfx_Roll);
            }
        }
    }
}

void Player_FallingDive(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_run_jump_water_fall_wait);
    }
    
    Math_StepToF(&this->linearVelocity, 0.0f, 0.05f);
    
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        this->actor.colChkInfo.damage = 0x10;
        Player_SetupDamage(play, this, 1, 4.0f, 5.0f, this->actor.shape.rot.y, 20);
    }
}

void Player_JumpSlash(Player* this, PlayState* play) {
    f32 sp2C;
    s16 sp2A;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    this->actor.gravity = -1.2f;
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (!func_80842DF4(play, this)) {
        Player_SetupMeleeAttack(this, 6.0f, 7.0f, 99.0f);
        
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
            Player_GetTargetVelAndYaw(this, &sp2C, &sp2A, 0.0f, play);
            func_8083DFE0(this, &sp2C, &this->currentYaw);
            
            return;
        }
        
        if (Player_SetupFallLanding(play, this) >= 0) {
            this->meleeWeaponAnimation += 2;
            Player_StartMeleeWeaponAttack(play, this, this->meleeWeaponAnimation);
            this->unk_845 = 3;
            Player_PlayLandingSfx(this);
        }
    }
}

s32 Player_SetupReleaseSpinAttack(Player* this, PlayState* play) {
    s32 temp;
    
    if (Player_SetupCutscene(play, this)) {
        this->stateFlags2 |= PLAYER_STATE2_17;
    } else {
        if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
            if ((this->unk_858 >= 0.85f) || Player_CanQuickspin(this)) {
                temp = sAnimMwaFlag_BigSpin[Player_HoldsTwoHandedWeapon(this)];
            } else {
                temp = sAnimMwaFlag_SmallSpin[Player_HoldsTwoHandedWeapon(this)];
            }
            
            Player_StartMeleeWeaponAttack(play, this, temp);
            Player_SetupInvincibility_NoDamageFlash(this, -8);
            
            this->stateFlags2 |= PLAYER_STATE2_17;
            if (this->unk_84B[this->unk_846] == 0) {
                this->stateFlags2 |= PLAYER_STATE2_30;
            }
        } else {
            return 0;
        }
    }
    
    return 1;
}

void Player_SetupWalkChargingSpinAttack(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_WalkChargingSpinAttack, 1);
}

void Player_SetupSidewalkChargingSpinAttack(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_SidewalkChargingSpinAttack, 1);
}

void Player_CancelSpinAttackCharge(Player* this, PlayState* play) {
    Player_ReturnToStandStill(this, play);
    Player_InactivateMeleeWeapon(this);
    Player_ChangeAnimMorphToLastFrame(play, this, sAnims_SpinAttackCancel[Player_HoldsTwoHandedWeapon(this)]);
    this->currentYaw = this->actor.shape.rot.y;
}

void Player_SetupChargeSpinAttack(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_ChargeSpinAttack, 1);
    this->unk_868 = 0.0f;
    Player_PlayAnimLoop(play, this, sAnims_SpinAttackCharge[Player_HoldsTwoHandedWeapon(this)]);
    this->unk_850 = 1;
}

void Player_UpdateSpinAttackTimer(Player* this) {
    Math_StepToF(&this->unk_858, 1.0f, 0.02f);
}

void Player_ChargeSpinAttack(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    s32 temp;
    
    this->stateFlags1 |= PLAYER_STATE1_12;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndAnimMovement(this);
        Player_SetZTargetFriendlyYaw(this);
        this->stateFlags1 &= ~PLAYER_STATE1_17;
        Player_PlayAnimLoop(play, this, sAnims_SpinAttackCharge[Player_HoldsTwoHandedWeapon(this)]);
        this->unk_850 = -1;
    }
    
    Player_StepLinearVelToZero(this);
    
    if (!Player_IsBusy(this, play) && (this->unk_850 != 0)) {
        Player_UpdateSpinAttackTimer(this);
        
        if (this->unk_850 < 0) {
            if (this->unk_858 >= 0.1f) {
                this->unk_845 = 0;
                this->unk_850 = 1;
            } else if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
                Player_CancelSpinAttackCharge(this, play);
            }
        } else if (!Player_SetupReleaseSpinAttack(this, play)) {
            Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.0f, play);
            
            temp = Player_GetSpinAttackMoveDirection(this, &sp34, &sp32, play);
            if (temp > 0) {
                Player_SetupWalkChargingSpinAttack(this, play);
            } else if (temp < 0) {
                Player_SetupSidewalkChargingSpinAttack(this, play);
            }
        }
    }
}

void Player_WalkChargingSpinAttack(Player* this, PlayState* play) {
    s16 temp1;
    s32 temp2;
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    s16 sp52;
    s32 temp4;
    s16 temp5;
    s32 sp44;
    
    temp1 = this->currentYaw - this->actor.shape.rot.y;
    temp2 = ABS(temp1);
    
    sp5C = fabsf(this->linearVelocity);
    sp58 = sp5C * 1.5f;
    
    this->stateFlags1 |= PLAYER_STATE1_12;
    
    if (sp58 < 1.5f) {
        sp58 = 1.5f;
    }
    
    sp58 = ((temp2 < 0x4000) ? -1.0f : 1.0f) * sp58;
    
    Player_SetupWalkSfx(this, sp58);
    
    sp58 = CLAMP(sp5C * 0.5f, 0.5f, 1.0f);
    
    LinkAnimation_BlendToJoint(
        play,
        &this->skelAnime,
        sAnims_SpinAttackCharge[Player_HoldsTwoHandedWeapon(this)],
        0.0f,
        sAnims_SpinAttackWalk[Player_HoldsTwoHandedWeapon(this)],
        this->unk_868 * (21.0f / 29.0f),
        sp58,
        this->blendTable
    );
    
    if (!Player_IsBusy(this, play) && !Player_SetupReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_GetTargetVelAndYaw(this, &sp54, &sp52, 0.0f, play);
        
        temp4 = Player_GetSpinAttackMoveDirection(this, &sp54, &sp52, play);
        
        if (temp4 < 0) {
            Player_SetupSidewalkChargingSpinAttack(this, play);
            
            return;
        }
        
        if (temp4 == 0) {
            sp54 = 0.0f;
            sp52 = this->currentYaw;
        }
        
        temp5 = sp52 - this->currentYaw;
        sp44 = ABS(temp5);
        
        if (sp44 > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
                this->currentYaw = sp52;
            }
            
            return;
        }
        
        Math_AsymStepToF(&this->linearVelocity, sp54 * 0.2f, 1.0f, 0.5f);
        Math_ScaledStepToS(&this->currentYaw, sp52, sp44 * 0.1f);
        
        if ((sp54 == 0.0f) && (this->linearVelocity == 0.0f)) {
            Player_SetupChargeSpinAttack(this, play);
        }
    }
}

void Player_SidewalkChargingSpinAttack(Player* this, PlayState* play) {
    f32 sp5C;
    f32 sp58;
    f32 sp54;
    s16 sp52;
    s32 temp4;
    s16 temp5;
    s32 sp44;
    
    sp5C = fabsf(this->linearVelocity);
    
    this->stateFlags1 |= PLAYER_STATE1_12;
    
    if (sp5C == 0.0f) {
        sp5C = ABS(this->unk_87C) * 0.0015f;
        if (sp5C < 400.0f) {
            sp5C = 0.0f;
        }
        Player_SetupWalkSfx(this, ((this->unk_87C >= 0) ? 1 : -1) * sp5C);
    } else {
        sp58 = sp5C * 1.5f;
        if (sp58 < 1.5f) {
            sp58 = 1.5f;
        }
        Player_SetupWalkSfx(this, sp58);
    }
    
    sp58 = CLAMP(sp5C * 0.5f, 0.5f, 1.0f);
    
    LinkAnimation_BlendToJoint(
        play,
        &this->skelAnime,
        sAnims_SpinAttackCharge[Player_HoldsTwoHandedWeapon(this)],
        0.0f,
        sAnims_SpinAttackSidewalk[Player_HoldsTwoHandedWeapon(this)],
        this->unk_868 * (21.0f / 29.0f),
        sp58,
        this->blendTable
    );
    
    if (!Player_IsBusy(this, play) && !Player_SetupReleaseSpinAttack(this, play)) {
        Player_UpdateSpinAttackTimer(this);
        Player_GetTargetVelAndYaw(this, &sp54, &sp52, 0.0f, play);
        
        temp4 = Player_GetSpinAttackMoveDirection(this, &sp54, &sp52, play);
        
        if (temp4 > 0) {
            Player_SetupWalkChargingSpinAttack(this, play);
            
            return;
        }
        
        if (temp4 == 0) {
            sp54 = 0.0f;
            sp52 = this->currentYaw;
        }
        
        temp5 = sp52 - this->currentYaw;
        sp44 = ABS(temp5);
        
        if (sp44 > 0x4000) {
            if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
                this->currentYaw = sp52;
            }
            
            return;
        }
        
        Math_AsymStepToF(&this->linearVelocity, sp54 * 0.2f, 1.0f, 0.5f);
        Math_ScaledStepToS(&this->currentYaw, sp52, sp44 * 0.1f);
        
        if ((sp54 == 0.0f) && (this->linearVelocity == 0.0f) && (sp5C == 0.0f)) {
            Player_SetupChargeSpinAttack(this, play);
        }
    }
}

void Player_JumpUpToLedge(Player* this, PlayState* play) {
    s32 sp3C;
    f32 temp1;
    s32 temp2;
    f32 temp3;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    sp3C = LinkAnimation_Update(play, &this->skelAnime);
    
    if (this->skelAnime.animation == &gPlayerAnim_link_normal_250jump_start) {
        this->linearVelocity = 1.0f;
        
        if (LinkAnimation_OnFrame(&this->skelAnime, 8.0f)) {
            temp1 = this->wallHeight;
            
            if (temp1 > this->ageProperties->unk_0C) {
                temp1 = this->ageProperties->unk_0C;
            }
            
            if (this->stateFlags1 & PLAYER_STATE1_27) {
                temp1 *= 0.085f;
            } else {
                temp1 *= 0.072f;
            }
            
            if (!LINK_IS_ADULT) {
                temp1 += 1.0f;
            }
            
            Player_SetupJumpWithSfx(this, NULL, temp1, play, NA_SE_VO_LI_AUTO_JUMP);
            this->unk_850 = -1;
            
            return;
        }
    } else {
        temp2 = Player_IsActionInterrupted(play, this, &this->skelAnime, 4.0f);
        
        if (temp2 == 0) {
            this->stateFlags1 &= ~(PLAYER_STATE1_14 | PLAYER_STATE1_18);
            
            return;
        }
        
        if ((sp3C != 0) || (temp2 > 0)) {
            Player_SetupStandingStillNoMorph(this, play);
            this->stateFlags1 &= ~(PLAYER_STATE1_14 | PLAYER_STATE1_18);
            
            return;
        }
        
        temp3 = 0.0f;
        
        if (this->skelAnime.animation == &gPlayerAnim_link_swimer_swim_15step_up) {
            if (LinkAnimation_OnFrame(&this->skelAnime, 30.0f)) {
                Player_StartJumpOutOfWater(play, this, 10.0f);
            }
            temp3 = 50.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_150step_up) {
            temp3 = 30.0f;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_normal_100step_up) {
            temp3 = 16.0f;
        }
        
        if (LinkAnimation_OnFrame(&this->skelAnime, temp3)) {
            Player_PlayLandingSfx(this);
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
        }
        
        if ((this->skelAnime.animation == &gPlayerAnim_link_normal_100step_up) || (this->skelAnime.curFrame > 5.0f)) {
            if (this->unk_850 == 0) {
                Player_PlayJumpSfx(this);
                this->unk_850 = 1;
            }
            Math_StepToF(&this->actor.shape.yOffset, 0.0f, 150.0f);
        }
    }
}

void Player_RunCutsceneFunc(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5 | PLAYER_STATE2_6;
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (
        ((this->stateFlags1 & PLAYER_STATE1_11) && (this->heldActor != NULL) && (this->getItemId == GI_NONE)) ||
        !Player_SetupCurrentUpperAction(this, play)
    ) {
        this->func_A74(play, this);
    }
}

s32 Player_CutsceneMoveUsingActionPos(PlayState* play, Player* this, CsCmdActorAction* arg2, f32 arg3, s16 arg4, s32 arg5) {
    if ((arg5 != 0) && (this->linearVelocity == 0.0f)) {
        return LinkAnimation_Update(play, &this->skelAnime);
    }
    
    if (arg5 != 2) {
        f32 sp34 = R_UPDATE_RATE * 0.5f;
        f32 selfDistX = arg2->endPos.x - this->actor.world.pos.x;
        f32 selfDistZ = arg2->endPos.z - this->actor.world.pos.z;
        f32 sp28 = sqrtf(SQ(selfDistX) + SQ(selfDistZ)) / sp34;
        s32 sp24 = (arg2->endFrame - play->csCtx.frames) + 1;
        
        arg4 = Math_Atan2S(selfDistZ, selfDistX);
        
        if (arg5 == 1) {
            f32 distX = arg2->endPos.x - arg2->startPos.x;
            f32 distZ = arg2->endPos.z - arg2->startPos.z;
            s32 temp = (((sqrtf(SQ(distX) + SQ(distZ)) / sp34) / (arg2->endFrame - arg2->startFrame)) / 1.5f) * 4.0f;
            
            if (temp >= sp24) {
                arg4 = this->actor.shape.rot.y;
                arg3 = 0.0f;
            } else {
                arg3 = sp28 / ((sp24 - temp) + 1);
            }
        } else {
            arg3 = sp28 / sp24;
        }
    }
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    Player_SetupWalkAnims(this, play);
    Player_SetRunVelAndYaw(this, arg3, arg4);
    
    if ((arg3 == 0.0f) && (this->linearVelocity == 0.0f)) {
        Player_EndRun(this, play);
    }
    
    return 0;
}

s32 Player_CutsceneMoveUsingActionPosIntoRange(PlayState* play, Player* this, f32* arg2, s32 arg3) {
    f32 dx = this->unk_450.x - this->actor.world.pos.x;
    f32 dz = this->unk_450.z - this->actor.world.pos.z;
    s32 sp2C = sqrtf(SQ(dx) + SQ(dz));
    s16 yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->unk_450);
    
    if (sp2C < arg3) {
        *arg2 = 0.0f;
        yaw = this->actor.shape.rot.y;
    }
    
    if (Player_CutsceneMoveUsingActionPos(play, this, NULL, *arg2, yaw, 2)) {
        return 0;
    }
    
    return sp2C;
}

s32 func_80845C68(PlayState* play, s32 arg1) {
    if (arg1 == 0) {
        Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
    }
    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 0;
    
    return arg1;
}

void Player_CutsceneMovement(Player* this, PlayState* play) {
    f32 sp3C;
    s32 temp;
    f32 sp34;
    s32 sp30;
    
    if (!Player_SetupItemCsOrFirstPerson(this, play)) {
        if (this->unk_850 == 0) {
            LinkAnimation_Update(play, &this->skelAnime);
            
            if (DECR(this->doorTimer) == 0) {
                this->linearVelocity = 0.1f;
                this->unk_850 = 1;
            }
        } else if (this->unk_84F == 0) {
            sp3C = 5.0f * sWaterSpeedScale;
            
            if (Player_CutsceneMoveUsingActionPosIntoRange(play, this, &sp3C, -1) < 30) {
                this->unk_84F = 1;
                this->stateFlags1 |= PLAYER_STATE1_29;
                
                this->unk_450.x = this->unk_45C.x;
                this->unk_450.z = this->unk_45C.z;
            }
        } else {
            sp34 = 5.0f;
            sp30 = 20;
            
            if (this->stateFlags1 & PLAYER_STATE1_0) {
                sp34 = gSaveContext.entranceSpeed;
                
                if (sConveyorSpeedIndex != 0) {
                    this->unk_450.x = (Math_SinS(sConveyorYaw) * 400.0f) + this->actor.world.pos.x;
                    this->unk_450.z = (Math_CosS(sConveyorYaw) * 400.0f) + this->actor.world.pos.z;
                }
            } else if (this->unk_850 < 0) {
                this->unk_850++;
                
                sp34 = gSaveContext.entranceSpeed;
                sp30 = -1;
            }
            
            temp = Player_CutsceneMoveUsingActionPosIntoRange(play, this, &sp34, sp30);
            
            if (
                (this->unk_850 == 0) || ((temp == 0) && (this->linearVelocity == 0.0f) &&
                (Play_GetCamera(play, CAM_ID_MAIN)->unk_14C & 0x10))
            ) {
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
                func_80845C68(play, gSaveContext.respawn[RESPAWN_MODE_DOWN].data);
                
                if (!Player_SetupSpeakOrCheck(this, play)) {
                    Player_EndCutsceneMovement(this, play);
                }
            }
        }
    }
    
    if (this->stateFlags1 & PLAYER_STATE1_11) {
        Player_SetupCurrentUpperAction(this, play);
    }
}

void Player_OpenDoor(Player* this, PlayState* play) {
    s32 sp2C;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    sp2C = LinkAnimation_Update(play, &this->skelAnime);
    
    Player_SetupCurrentUpperAction(this, play);
    
    if (sp2C) {
        if (this->unk_850 == 0) {
            if (DECR(this->doorTimer) == 0) {
                this->unk_850 = 1;
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
        } else {
            Player_SetupStandingStillNoMorph(this, play);
            if (play->roomCtx.prevRoom.num >= 0) {
                func_80097534(play, &play->roomCtx);
            }
            func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xDFF);
        }
        
        return;
    }
    
    if (!(this->stateFlags1 & PLAYER_STATE1_29) && LinkAnimation_OnFrame(&this->skelAnime, 15.0f)) {
        play->func_11D54(this, play);
    }
}

void Player_LiftActor(Player* this, PlayState* play) {
    Player_StepLinearVelToZero(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillType(this, play);
        Player_SetupHoldActorUpperAction(this, play);
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 4.0f)) {
        Actor* interactRangeActor = this->interactRangeActor;
        
        if (!Player_InterruptHoldingActor(play, this, interactRangeActor)) {
            this->heldActor = interactRangeActor;
            this->actor.child = interactRangeActor;
            interactRangeActor->parent = &this->actor;
            interactRangeActor->bgCheckFlags &=
                ~(BGCHECKFLAG_GROUND | BGCHECKFLAG_GROUND_TOUCH | BGCHECKFLAG_GROUND_LEAVE | BGCHECKFLAG_WALL |
                BGCHECKFLAG_CEILING | BGCHECKFLAG_WATER | BGCHECKFLAG_WATER_TOUCH | BGCHECKFLAG_GROUND_STRICT);
            this->unk_3BC.y = interactRangeActor->shape.rot.y - this->actor.shape.rot.y;
        }
        
        return;
    }
    
    Math_ScaledStepToS(&this->unk_3BC.y, 0, 4000);
}

void Player_ThrowStonePillar(Player* this, PlayState* play) {
    static struct_80832924 sAnimSfx_ThrowStonePillar[] = {
        { NA_SE_VO_LI_SWORD_L, 0x2031  },
        { NA_SE_VO_LI_SWORD_N, -0x20E6 },
    };
    
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->unk_850++ > 20)) {
        if (!Player_SetupItemCsOrFirstPerson(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_heavy_carry_end, play);
        }
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 41.0f)) {
        BgHeavyBlock* heavyBlock = (BgHeavyBlock*)this->interactRangeActor;
        
        this->heldActor = &heavyBlock->dyna.actor;
        this->actor.child = &heavyBlock->dyna.actor;
        heavyBlock->dyna.actor.parent = &this->actor;
        func_8002DBD0(&heavyBlock->dyna.actor, &heavyBlock->unk_164, &this->leftHandPos);
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 229.0f)) {
        Actor* heldActor = this->heldActor;
        
        heldActor->speedXZ = Math_SinS(heldActor->shape.rot.x) * 40.0f;
        heldActor->velocity.y = Math_CosS(heldActor->shape.rot.x) * 40.0f;
        heldActor->gravity = -2.0f;
        heldActor->minVelocityY = -30.0f;
        Player_DetatchHeldActor(play, this);
        
        return;
    }
    
    Player_PlayAnimSfx(this, sAnimSfx_ThrowStonePillar);
}

void Player_LiftSilverBoulder(Player* this, PlayState* play) {
    Player_StepLinearVelToZero(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_silver_wait);
        this->unk_850 = 1;
        
        return;
    }
    
    if (this->unk_850 == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 27.0f)) {
            Actor* interactRangeActor = this->interactRangeActor;
            
            this->heldActor = interactRangeActor;
            this->actor.child = interactRangeActor;
            interactRangeActor->parent = &this->actor;
            
            return;
        }
        
        if (LinkAnimation_OnFrame(&this->skelAnime, 25.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
            
            return;
        }
    } else if (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN)) {
        Player_SetActionFunc(play, this, Player_ThrowSilverBoulder, 1);
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_silver_throw);
    }
}

void Player_ThrowSilverBoulder(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillType(this, play);
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 6.0f)) {
        Actor* heldActor = this->heldActor;
        
        heldActor->world.rot.y = this->actor.shape.rot.y;
        heldActor->speedXZ = 10.0f;
        heldActor->velocity.y = 20.0f;
        Player_SetupHeldItemUpperActionFunc(play, this);
        func_8002F7DC(&this->actor, NA_SE_PL_THROW);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }
}

void Player_FailToLiftActor(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_nocarry_free_wait);
        this->unk_850 = 15;
        
        return;
    }
    
    if (this->unk_850 != 0) {
        this->unk_850--;
        if (this->unk_850 == 0) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_nocarry_free_end, play);
            this->stateFlags1 &= ~PLAYER_STATE1_11;
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DAMAGE_S);
        }
    }
}

void Player_SetupPutDownActor(Player* this, PlayState* play) {
    Player_StepLinearVelToZero(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillType(this, play);
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 4.0f)) {
        Actor* heldActor = this->heldActor;
        
        if (!Player_InterruptHoldingActor(play, this, heldActor)) {
            heldActor->velocity.y = 0.0f;
            heldActor->speedXZ = 0.0f;
            Player_SetupHeldItemUpperActionFunc(play, this);
        }
    }
}

void Player_StartThrowActor(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    
    Player_StepLinearVelToZero(this);
    
    if (
        LinkAnimation_Update(play, &this->skelAnime) ||
        ((this->skelAnime.curFrame >= 8.0f) && Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.018f, play))
    ) {
        Player_SetupStandingStillType(this, play);
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 3.0f)) {
        Player_ThrowActor(play, this, this->linearVelocity + 8.0f, 12.0f);
    }
}

static ColliderCylinderInit sBodyCylInit = {
    {
        COLTYPE_HIT5,
        AT_NONE,
        AC_ON | AC_TYPE_ENEMY,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_PLAYER,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK1,
        { 0x00000000, 0x00,       0x00 },
        { 0xFFCFFFFF, 0x00,       0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_ON,
    },
    { 12,         60,         0, { 0, 0, 0} },
};

static ColliderQuadInit sMeleeWpnQuadInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_PLAYER,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000100, 0x00,   0x01 },
        { 0xFFCFFFFF, 0x00,   0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { { { 0.0f,   0.0f,   0.0f },{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

static ColliderQuadInit sShieldQuadInit = {
    {
        COLTYPE_METAL,
        AT_ON | AT_TYPE_PLAYER,
        AC_ON | AC_HARD | AC_TYPE_ENEMY,
        OC1_NONE,
        OC2_TYPE_PLAYER,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00100000, 0x00,   0x00 },
        { 0xDFCFFFFF, 0x00,   0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_NONE,
    },
    { { { 0.0f,   0.0f,   0.0f },{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

void Player_DoNothing3(Actor* thisx, PlayState* play) {
}

void Player_SpawnNoUpdateOrDraw(PlayState* play, Player* this) {
    this->actor.update = Player_DoNothing3;
    this->actor.draw = NULL;
}

void Player_SetupSpawnFromBlueWarp(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_SpawnFromBlueWarp, 0);
    if ((play->sceneId == SCENE_SPOT06) && (gSaveContext.sceneLayer >= 4)) {
        this->unk_84F = 1;
    }
    this->stateFlags1 |= PLAYER_STATE1_29;
    LinkAnimation_Change(play, &this->skelAnime, &gPlayerAnim_link_okarina_warp_goal, 2.0f / 3.0f, 0.0f, 24.0f, ANIMMODE_ONCE, 0.0f);
    this->actor.world.pos.y += 800.0f;
}

void Player_CutsceneDrawSword(PlayState* play, Player* this, s32 arg2) {
    static u8 sItemID_SwordByAge[] = { ITEM_SWORD_MASTER, ITEM_SWORD_KOKIRI };
    s32 item = sItemID_SwordByAge[(void)0, gSaveContext.linkAge];
    s32 actionParam = sItemActionParams[item];
    
    Player_PutAwayHookshot(this);
    Player_DetatchHeldActor(play, this);
    
    this->heldItemId = item;
    this->nextModelGroup = Player_ActionToModelGroup(this, actionParam);
    
    Player_ChangeItem(play, this, actionParam);
    Player_SetupHeldItemUpperActionFunc(play, this);
    
    if (arg2 != 0) {
        func_8002F7DC(&this->actor, NA_SE_IT_SWORD_PICKOUT);
    }
}

void Player_SpawnFromTimeTravel(PlayState* play, Player* this) {
    static Vec3f sEndTimeTravelStartPos = { -1.0f, 69.0f, 20.0f };
    
    Player_SetActionFunc(play, this, Player_EndTimeTravel, 0);
    this->stateFlags1 |= PLAYER_STATE1_29;
    Math_Vec3f_Copy(&this->actor.world.pos, &sEndTimeTravelStartPos);
    this->currentYaw = this->actor.shape.rot.y = -0x8000;
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        this->ageProperties->unk_A0,
        2.0f / 3.0f,
        0.0f,
        0.0f,
        ANIMMODE_ONCE,
        0.0f
    );
    Player_SetupAnimMovement(play, this, 0x28F);
    if (LINK_IS_ADULT) {
        Player_CutsceneDrawSword(play, this, 0);
    }
    this->unk_850 = 20;
}

void Player_SpawnOpeningDoor(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_SetupOpenDoorFromSpawn, 0);
    Player_SetupAnimMovement(play, this, 0x9B);
}

void Player_SpawnExitingGrotto(PlayState* play, Player* this) {
    Player_SetupJump(this, &gPlayerAnim_link_normal_jump, 12.0f, play);
    Player_SetActionFunc(play, this, Player_JumpFromGrotto, 0);
    this->stateFlags1 |= PLAYER_STATE1_29;
    this->fallStartHeight = this->actor.world.pos.y;
    OnePointCutscene_Init(play, 5110, 40, &this->actor, CAM_ID_MAIN);
}

void Player_SpawnWithKnockback(PlayState* play, Player* this) {
    Player_SetupDamage(play, this, 1, 2.0f, 2.0f, this->actor.shape.rot.y + 0x8000, 0);
}

void Player_SetupSpawnFromWarpSong(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_SpawnFromWarpSong, 0);
    this->actor.draw = NULL;
    this->stateFlags1 |= PLAYER_STATE1_29;
}

static s16 sMagicSpellActors[] = { ACTOR_MAGIC_WIND, ACTOR_MAGIC_DARK, ACTOR_MAGIC_FIRE };

Actor* Player_SpawnMagicSpellActor(PlayState* play, Player* this, s32 arg2) {
    return Actor_Spawn(
        &play->actorCtx,
        play,
        sMagicSpellActors[arg2],
        this->actor.world.pos.x,
        this->actor.world.pos.y,
        this->actor.world.pos.z,
        0,
        0,
        0,
        0
    );
}

void Player_SetupSpawnFromFaroresWind(PlayState* play, Player* this) {
    this->actor.draw = NULL;
    Player_SetActionFunc(play, this, Player_SpawnFromFaroresWind, 0);
    this->stateFlags1 |= PLAYER_STATE1_29;
}

static InitChainEntry sInitChain[] = {
    ICHAIN_F32(targetArrowOffset, 500, ICHAIN_STOP),
};

static EffectBlureInit2 sBlure2InitParams = {
    0, 8, 0, { 255, 255, 255, 255 },         { 255, 255, 255, 64  }, { 255, 255, 255, 0 }, { 255, 255, 255, 0 }, 4,
    0, 2, 0, { 0,   0,   0,   0   },         { 0,   0,   0,   0   },
};

static Vec3s sBaseTransl = { -57, 3377, 0 };

void Player_InitCommon(Player* this, PlayState* play, FlexSkeletonHeader* skelHeader) {
    this->ageProperties = &sAgeProperties[gSaveContext.linkAge];
    Actor_ProcessInitChain(&this->actor, sInitChain);
    this->meleeWeaponEffectIndex = TOTAL_EFFECT_COUNT;
    this->currentYaw = this->actor.world.rot.y;
    Player_SetupHeldItemUpperActionFunc(play, this);
    
    SkelAnime_InitLink(
        play,
        &this->skelAnime,
        skelHeader,
        GET_PLAYER_ANIM(PLAYER_ANIMGROUP_0, this->modelAnimType),
        9,
        this->jointTable,
        this->morphTable,
        PLAYER_LIMB_MAX
    );
    this->skelAnime.baseTransl = sBaseTransl;
    SkelAnime_InitLink(
        play,
        &this->skelAnime2,
        skelHeader,
        Player_GetAnim_StandingStill(this),
        9,
        this->jointTable2,
        this->morphTable2,
        PLAYER_LIMB_MAX
    );
    this->skelAnime2.baseTransl = sBaseTransl;
    
    Effect_Add(play, &this->meleeWeaponEffectIndex, EFFECT_BLURE2, 0, 0, &sBlure2InitParams);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawFeet, this->ageProperties->unk_04);
    this->subCamId = CAM_ID_NONE;
    
    Collider_InitCylinder(play, &this->cylinder);
    Collider_SetCylinder(play, &this->cylinder, &this->actor, &sBodyCylInit);
    Collider_InitQuad(play, &this->meleeWeaponQuads[0]);
    Collider_SetQuad(play, &this->meleeWeaponQuads[0], &this->actor, &sMeleeWpnQuadInit);
    Collider_InitQuad(play, &this->meleeWeaponQuads[1]);
    Collider_SetQuad(play, &this->meleeWeaponQuads[1], &this->actor, &sMeleeWpnQuadInit);
    Collider_InitQuad(play, &this->shieldQuad);
    Collider_SetQuad(play, &this->shieldQuad, &this->actor, &sShieldQuadInit);
}

static void (*sSpawnFuncs[])(PlayState* play, Player* this) = {
    Player_SpawnNoUpdateOrDraw,       Player_SpawnFromTimeTravel,  Player_SetupSpawnFromBlueWarp, Player_SpawnOpeningDoor,       Player_SpawnExitingGrotto, Player_SetupSpawnFromWarpSong,
    Player_SetupSpawnFromFaroresWind, Player_SpawnWithKnockback,   Player_SpawnWalking_Slow,      Player_SpawnWalking_Slow,      Player_SpawnWalking_Slow,  Player_SpawnWalking_Slow,
    Player_SpawnWalking_Slow,         Player_SpawnWalking_Default, Player_SpawnWalking_Slow,      Player_SpawnWalking_PrevSpeed,
};

static Vec3f sNaviSpawnPos = { 0.0f, 50.0f, 0.0f };

void Player_Init(Actor* thisx, PlayState* play2) {
    Player* this = (Player*)thisx;
    PlayState* play = play2;
    SceneTableEntry* scene = play->loadedScene;
    u32 titleFileSize;
    s32 initMode;
    s32 respawnFlag;
    s32 respawnMode;
    
    play->shootingGalleryStatus = play->bombchuBowlingStatus = 0;
    
    play->playerInit = Player_InitCommon;
    play->playerUpdate = Player_UpdateCommon;
    play->isPlayerDroppingFish = Player_IsDroppingFish;
    play->startPlayerFishing = Player_StartFishing;
    play->grabPlayer = Player_SetupRestrainedByEnemy;
    play->startPlayerCutscene = Player_SetupPlayerCutscene;
    play->func_11D54 = Player_SetupStandingStillMorph;
    play->damagePlayer = Player_InflictDamage;
    play->talkWithPlayer = Player_StartTalkingWithActor;
    
    thisx->room = -1;
    this->ageProperties = &sAgeProperties[gSaveContext.linkAge];
    this->itemActionParam = this->heldItemActionParam = -1;
    this->heldItemId = ITEM_NONE;
    
    Player_UseItem(play, this, ITEM_NONE);
    Player_SetEquipmentData(play, this);
    this->prevBoots = this->currentBoots;
    Player_InitCommon(this, play, gPlayerSkelHeaders[((void)0, gSaveContext.linkAge)]);
    Assert(this->giObjectSegment = (void*)(((u32)ZeldaArena_MallocDebug(Pathch_GetItem_SegmentSize, "../z_player.c", 17175) + 8) & ~0xF));
    
    respawnFlag = gSaveContext.respawnFlag;
    
    if (respawnFlag != 0) {
        if (respawnFlag == -3) {
            thisx->params = gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams;
        } else {
            if ((respawnFlag == 1) || (respawnFlag == -1)) {
                this->unk_A86 = -2;
            }
            
            if (respawnFlag < 0) {
                respawnMode = RESPAWN_MODE_DOWN;
            } else {
                respawnMode = respawnFlag - 1;
                Math_Vec3f_Copy(&thisx->world.pos, &gSaveContext.respawn[respawnMode].pos);
                Math_Vec3f_Copy(&thisx->home.pos, &thisx->world.pos);
                Math_Vec3f_Copy(&thisx->prevPos, &thisx->world.pos);
                this->fallStartHeight = thisx->world.pos.y;
                this->currentYaw = thisx->shape.rot.y = gSaveContext.respawn[respawnMode].yaw;
                thisx->params = gSaveContext.respawn[respawnMode].playerParams;
            }
            
            play->actorCtx.flags.tempSwch = gSaveContext.respawn[respawnMode].tempSwchFlags & 0xFFFFFF;
            play->actorCtx.flags.tempCollect = gSaveContext.respawn[respawnMode].tempCollectFlags;
        }
    }
    
    if ((respawnFlag == 0) || (respawnFlag < -1)) {
        titleFileSize = scene->titleFile.vromEnd - scene->titleFile.vromStart;
        if ((titleFileSize != 0) && gSaveContext.showTitleCard) {
            if (
                (gSaveContext.sceneLayer < 4) &&
                (gEntranceTable[((void)0, gSaveContext.entranceIndex) + ((void)0, gSaveContext.sceneLayer)].field &
                0x4000) &&
                ((play->sceneId != SCENE_DDAN) || GET_EVENTCHKINF(EVENTCHKINF_B0)) &&
                ((play->sceneId != SCENE_NIGHT_SHOP) || GET_EVENTCHKINF(EVENTCHKINF_25))
            ) {
                TitleCard_InitPlaceName(play, &play->actorCtx.titleCtx, this->giObjectSegment, 160, 120, 144, 24, 20);
            }
        }
        gSaveContext.showTitleCard = true;
    }
    
    if (func_80845C68(play, (respawnFlag == 2) ? 1 : 0) == 0) {
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = (thisx->params & 0xFF) | 0xD00;
    }
    
    gSaveContext.respawn[RESPAWN_MODE_DOWN].data = 1;
    
    if (play->sceneId <= SCENE_GANONTIKA_SONOGO) {
        gSaveContext.infTable[INFTABLE_1AX_INDEX] |= gBitFlags[play->sceneId];
    }
    
    initMode = (thisx->params & 0xF00) >> 8;
    if ((initMode == 5) || (initMode == 6)) {
        if (gSaveContext.cutsceneIndex >= 0xFFF0) {
            initMode = 13;
        }
    }
    
    sSpawnFuncs[initMode](play, this);
    
    if (initMode != 0) {
        if ((gSaveContext.gameMode == 0) || (gSaveContext.gameMode == 3)) {
            this->naviActor = Player_SpawnFairy(play, this, &thisx->world.pos, &sNaviSpawnPos, FAIRY_NAVI);
            if (gSaveContext.dogParams != 0) {
                gSaveContext.dogParams |= 0x8000;
            }
        }
    }
    
    if (gSaveContext.nayrusLoveTimer != 0) {
        gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
        Player_SpawnMagicSpellActor(play, this, 1);
        this->stateFlags3 &= ~PLAYER_STATE3_RESTORE_NAYRUS_LOVE;
    }
    
    if (gSaveContext.entranceSound != 0) {
        Audio_PlayActorSfx2(&this->actor, ((void)0, gSaveContext.entranceSound));
        gSaveContext.entranceSound = 0;
    }
    
    Map_SavePlayerInitialInfo(play);
    MREG(64) = 0;
}

void Player_StepValueToZero(s16* pValue) {
    s16 step;
    
    step = (ABS(*pValue) * 100.0f) / 1000.0f;
    step = CLAMP(step, 400, 4000);
    
    Math_ScaledStepToS(pValue, 0, step);
}

void Player_StepLookToZero(Player* this) {
    s16 sp26;
    
    if (!(this->unk_6AE & 2)) {
        sp26 = this->actor.focus.rot.y - this->actor.shape.rot.y;
        Player_StepValueToZero(&sp26);
        this->actor.focus.rot.y = this->actor.shape.rot.y + sp26;
    }
    
    if (!(this->unk_6AE & 1)) {
        Player_StepValueToZero(&this->actor.focus.rot.x);
    }
    
    if (!(this->unk_6AE & 8)) {
        Player_StepValueToZero(&this->unk_6B6);
    }
    
    if (!(this->unk_6AE & 0x40)) {
        Player_StepValueToZero(&this->unk_6BC);
    }
    
    if (!(this->unk_6AE & 4)) {
        Player_StepValueToZero(&this->actor.focus.rot.z);
    }
    
    if (!(this->unk_6AE & 0x10)) {
        Player_StepValueToZero(&this->unk_6B8);
    }
    
    if (!(this->unk_6AE & 0x20)) {
        Player_StepValueToZero(&this->unk_6BA);
    }
    
    if (!(this->unk_6AE & 0x80)) {
        if (this->unk_6B0 != 0) {
            Player_StepValueToZero(&this->unk_6B0);
        } else {
            Player_StepValueToZero(&this->unk_6BE);
        }
    }
    
    if (!(this->unk_6AE & 0x100)) {
        Player_StepValueToZero(&this->unk_6C0);
    }
    
    this->unk_6AE = 0;
}

static f32 sScaleDiveDists[] = { 120.0f, 240.0f, 360.0f };

static u8 sDoAction_Dive[] = { DO_ACTION_1, DO_ACTION_2, DO_ACTION_3, DO_ACTION_4,
                               DO_ACTION_5, DO_ACTION_6, DO_ACTION_7, DO_ACTION_8 };

void Player_SetupDoActionText(PlayState* play, Player* this) {
    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) && (this->actor.category == ACTORCAT_PLAYER)) {
        Actor* heldActor = this->heldActor;
        Actor* interactRangeActor = this->interactRangeActor;
        s32 sp24;
        s32 sp20 = this->unk_84B[this->unk_846];
        s32 sp1C = Player_IsSwimming(this);
        s32 doAction = DO_ACTION_NONE;
        
        if (!Player_InBlockingCsMode(play, this)) {
            if (this->stateFlags1 & PLAYER_STATE1_20) {
                doAction = DO_ACTION_RETURN;
            } else if ((this->heldItemActionParam == PLAYER_AP_FISHING_POLE) && (this->unk_860 != 0)) {
                if (this->unk_860 == 2) {
                    doAction = DO_ACTION_REEL;
                }
            } else if ((Player_PlayOcarina != this->func_674) && !(this->stateFlags2 & PLAYER_STATE2_18)) {
                if (
                    (this->doorType != PLAYER_DOORTYPE_NONE) &&
                    (!(this->stateFlags1 & PLAYER_STATE1_11) ||
                    ((heldActor != NULL) && (heldActor->id == ACTOR_EN_RU1)))
                ) {
                    doAction = DO_ACTION_OPEN;
                } else if (
                    (!(this->stateFlags1 & PLAYER_STATE1_11) || (heldActor == NULL)) &&
                    (interactRangeActor != NULL) &&
                    ((!sp1C && (this->getItemId == GI_NONE)) ||
                    ((this->getItemId < 0) && !(this->stateFlags1 & PLAYER_STATE1_27)))
                ) {
                    if (this->getItemId < 0) {
                        doAction = DO_ACTION_OPEN;
                    } else if ((interactRangeActor->id == ACTOR_BG_TOKI_SWD) && LINK_IS_ADULT) {
                        doAction = DO_ACTION_DROP;
                    } else {
                        doAction = DO_ACTION_GRAB;
                    }
                } else if (!sp1C && (this->stateFlags2 & PLAYER_STATE2_0)) {
                    doAction = DO_ACTION_GRAB;
                } else if (
                    (this->stateFlags2 & PLAYER_STATE2_2) ||
                    (!(this->stateFlags1 & PLAYER_STATE1_23) && (this->rideActor != NULL))
                ) {
                    doAction = DO_ACTION_CLIMB;
                } else if (
                    (this->stateFlags1 & PLAYER_STATE1_23) && !EN_HORSE_CHECK_4((EnHorse*)this->rideActor) &&
                    (Player_DismountHorse != this->func_674)
                ) {
                    if ((this->stateFlags2 & PLAYER_STATE2_1) && (this->targetActor != NULL)) {
                        if (this->targetActor->category == ACTORCAT_NPC) {
                            doAction = DO_ACTION_SPEAK;
                        } else {
                            doAction = DO_ACTION_CHECK;
                        }
                    } else if (!func_8002DD78(this) && !(this->stateFlags1 & PLAYER_STATE1_20)) {
                        doAction = DO_ACTION_FASTER;
                    }
                } else if ((this->stateFlags2 & PLAYER_STATE2_1) && (this->targetActor != NULL)) {
                    if (this->targetActor->category == ACTORCAT_NPC) {
                        doAction = DO_ACTION_SPEAK;
                    } else {
                        doAction = DO_ACTION_CHECK;
                    }
                } else if (
                    (this->stateFlags1 & (PLAYER_STATE1_13 | PLAYER_STATE1_21)) ||
                    ((this->stateFlags1 & PLAYER_STATE1_23) && (this->stateFlags2 & PLAYER_STATE2_22))
                ) {
                    doAction = DO_ACTION_DOWN;
                } else if (this->stateFlags2 & PLAYER_STATE2_16) {
                    doAction = DO_ACTION_ENTER;
                } else if (
                    (this->stateFlags1 & PLAYER_STATE1_11) && (this->getItemId == GI_NONE) &&
                    (heldActor != NULL)
                ) {
                    if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (heldActor->id == ACTOR_EN_NIW)) {
                        if (Player_CanThrowActor(this, heldActor) == 0) {
                            doAction = DO_ACTION_DROP;
                        } else {
                            doAction = DO_ACTION_THROW;
                        }
                    }
                } else if (
                    !(this->stateFlags1 & PLAYER_STATE1_27) && Player_CanHoldActor(this) &&
                    (this->getItemId < GI_MAX)
                ) {
                    doAction = DO_ACTION_GRAB;
                } else if (this->stateFlags2 & PLAYER_STATE2_11) {
                    sp24 = (sScaleDiveDists[CUR_UPG_VALUE(UPG_SCALE)] - this->actor.yDistToWater) / 40.0f;
                    sp24 = CLAMP(sp24, 0, 7);
                    doAction = sDoAction_Dive[sp24];
                } else if (sp1C && !(this->stateFlags2 & PLAYER_STATE2_10)) {
                    doAction = DO_ACTION_DIVE;
                } else if (
                    !sp1C && (!(this->stateFlags1 & PLAYER_STATE1_22) || Player_IsZTargeting(this) ||
                    !Player_IsChildWithHylianShield(this))
                ) {
                    if (
                        (!(this->stateFlags1 & PLAYER_STATE1_14) && (sp20 <= 0) &&
                        (func_8008E9C4(this) ||
                        ((sFloorSpecialProperty != 7) &&
                        (Player_IsFriendlyZTargeting(this) || ((play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
                        !(this->stateFlags1 & PLAYER_STATE1_22) && (sp20 == 0))))))
                    ) {
                        doAction = DO_ACTION_ATTACK;
                    } else if (
                        (play->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) && Player_IsZTargeting(this) &&
                        (sp20 > 0)
                    ) {
                        doAction = DO_ACTION_JUMP;
                    } else if (
                        (this->heldItemActionParam >= PLAYER_AP_SWORD_MASTER) ||
                        ((this->stateFlags2 & PLAYER_STATE2_20) &&
                        (play->actorCtx.targetCtx.arrowPointedActor == NULL))
                    ) {
                        doAction = DO_ACTION_PUTAWAY;
                    }
                }
            }
        }
        
        if (doAction != DO_ACTION_PUTAWAY) {
            this->unk_837 = 20;
        } else if (this->unk_837 != 0) {
            doAction = DO_ACTION_NONE;
            this->unk_837--;
        }
        
        Interface_SetDoAction(play, doAction);
        
        if (this->stateFlags2 & PLAYER_STATE2_21) {
            if (this->unk_664 != NULL) {
                Interface_SetNaviCall(play, 0x1E);
            } else {
                Interface_SetNaviCall(play, 0x1D);
            }
            Interface_SetNaviCall(play, 0x1E);
        } else {
            Interface_SetNaviCall(play, 0x1F);
        }
    }
}

s32 Player_SetupHover(Player* this) {
    s32 cond;
    
    if ((this->currentBoots == PLAYER_BOOTS_HOVER) && (this->hoverBootsTimer != 0)) {
        this->hoverBootsTimer--;
    } else {
        this->hoverBootsTimer = 0;
    }
    
    cond = (this->currentBoots == PLAYER_BOOTS_HOVER) &&
        ((this->actor.yDistToWater >= 0.0f) || (Player_GetFloorDamageType(sFloorSpecialProperty) >= PLAYER_FLOODRAMAGE_SPIKE) || Player_IsFloorSinkingSand(sFloorSpecialProperty));
    
    if (cond && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (this->hoverBootsTimer != 0)) {
        this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    }
    
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        if (!cond) {
            this->hoverBootsTimer = 19;
        }
        
        return 0;
    }
    
    sFloorSpecialProperty = 0;
    this->unk_898 = this->unk_89A = sFloorPitch = 0;
    
    return 1;
}

static Vec3f sWallCheckOffset = { 0.0f, 18.0f, 0.0f };

void Player_UpdateBgcheck(PlayState* play, Player* this) {
    u8 spC7 = 0;
    CollisionPoly* floorPoly;
    Vec3f spB4;
    f32 spB0;
    f32 spAC;
    f32 spA8;
    u32 spA4;
    
    sFloorProperty = this->unk_A7A;
    
    if (this->stateFlags2 & PLAYER_STATE2_18) {
        spB0 = 10.0f;
        spAC = 15.0f;
        spA8 = 30.0f;
    } else {
        spB0 = this->ageProperties->unk_38;
        spAC = 26.0f;
        spA8 = this->ageProperties->unk_00;
    }
    
    if (this->stateFlags1 & (PLAYER_STATE1_29 | PLAYER_STATE1_31)) {
        if (this->stateFlags1 & PLAYER_STATE1_31) {
            this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
            spA4 = UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else if ((this->stateFlags1 & PLAYER_STATE1_0) && ((this->unk_A84 - (s32)this->actor.world.pos.y) >= 100)) {
            spA4 = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else if (
            !(this->stateFlags1 & PLAYER_STATE1_0) &&
            ((Player_OpenDoor == this->func_674) || (Player_CutsceneMovement == this->func_674))
        ) {
            this->actor.bgCheckFlags &= ~(BGCHECKFLAG_WALL | BGCHECKFLAG_PLAYER_WALL_INTERACT);
            spA4 = UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        } else {
            spA4 = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 |
                UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
        }
    } else {
        spA4 = UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 |
            UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_5;
    }
    
    if (this->stateFlags3 & PLAYER_STATE3_0) {
        spA4 &= ~(UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2);
    }
    
    if (spA4 & UPDBGCHECKINFO_FLAG_2) {
        this->stateFlags3 |= PLAYER_STATE3_4;
    }
    
    Math_Vec3f_Copy(&spB4, &this->actor.world.pos);
    Actor_UpdateBgCheckInfo(play, &this->actor, spAC, spB0, spA8, spA4);
    
    if (this->actor.bgCheckFlags & BGCHECKFLAG_CEILING) {
        this->actor.velocity.y = 0.0f;
    }
    
    sFloorDistY = this->actor.world.pos.y - this->actor.floorHeight;
    sConveyorSpeedIndex = 0;
    
    floorPoly = this->actor.floorPoly;
    
    if (floorPoly != NULL) {
        this->unk_A7A = SurfaceType_GetFloorProperty(&play->colCtx, floorPoly, this->actor.floorBgId);
        this->unk_A82 = this->unk_89E;
        
        if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
            if (this->actor.yDistToWater < 20.0f) {
                this->unk_89E = 4;
            } else {
                this->unk_89E = 5;
            }
        } else {
            if (this->stateFlags2 & PLAYER_STATE2_9) {
                this->unk_89E = 1;
            } else {
                this->unk_89E = SurfaceType_GetSfxId(&play->colCtx, floorPoly, this->actor.floorBgId);
            }
        }
        
        if (this->actor.category == ACTORCAT_PLAYER) {
            Audio_SetCodeReverb(SurfaceType_GetEcho(&play->colCtx, floorPoly, this->actor.floorBgId));
            
            if (this->actor.floorBgId == BGCHECK_SCENE) {
                Environment_ChangeLightSetting(
                    play,
                    SurfaceType_GetLightSetting(&play->colCtx, floorPoly, this->actor.floorBgId)
                );
            } else {
                DynaPoly_SetPlayerAbove(&play->colCtx, this->actor.floorBgId);
            }
        }
        
        // This block extracts the conveyor properties from the floor poly
        sConveyorSpeedIndex = SurfaceType_GetConveyorSpeed(&play->colCtx, floorPoly, this->actor.floorBgId);
        if (sConveyorSpeedIndex != 0) {
            sIsFloorConveyor = SurfaceType_IsFloorConveyor(&play->colCtx, floorPoly, this->actor.floorBgId);
            if (
                (!sIsFloorConveyor && (this->actor.yDistToWater > 20.0f) &&
                (this->currentBoots != PLAYER_BOOTS_IRON)) ||
                (sIsFloorConveyor && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND))
            ) {
                sConveyorYaw =
                    SurfaceType_GetConveyorDirection(&play->colCtx, floorPoly, this->actor.floorBgId) * (0x10000 / 64);
            } else {
                sConveyorSpeedIndex = 0;
            }
        }
    }
    
    Player_SetupExit(play, this, floorPoly, this->actor.floorBgId);
    
    this->actor.bgCheckFlags &= ~BGCHECKFLAG_PLAYER_WALL_INTERACT;
    
    if (this->actor.bgCheckFlags & BGCHECKFLAG_WALL) {
        CollisionPoly* spA0;
        s32 sp9C;
        s16 sp9A;
        
        sWallCheckOffset.y = 18.0f;
        sWallCheckOffset.z = this->ageProperties->unk_38 + 10.0f;
        
        if (
            !(this->stateFlags2 & PLAYER_STATE2_18) &&
            Player_RelativeLineRaycast(play, this, &sWallCheckOffset, &spA0, &sp9C, &sWallIntersectPos)
        ) {
            this->actor.bgCheckFlags |= BGCHECKFLAG_PLAYER_WALL_INTERACT;
            if (this->actor.wallPoly != spA0) {
                this->actor.wallPoly = spA0;
                this->actor.wallBgId = sp9C;
                this->actor.wallYaw = Math_Atan2S(spA0->normal.z, spA0->normal.x);
            }
        }
        
        sp9A = this->actor.shape.rot.y - (s16)(this->actor.wallYaw + 0x8000);
        
        sTouchedWallFlags = SurfaceType_GetWallFlags(&play->colCtx, this->actor.wallPoly, this->actor.wallBgId);
        
        sTouchedWallYaw = ABS(sp9A);
        
        sp9A = this->currentYaw - (s16)(this->actor.wallYaw + 0x8000);
        
        sTouchedWallYaw2 = ABS(sp9A);
        
        spB0 = sTouchedWallYaw2 * 0.00008f;
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || spB0 >= 1.0f) {
            this->unk_880 = R_RUN_SPEED_LIMIT / 100.0f;
        } else {
            spAC = (R_RUN_SPEED_LIMIT / 100.0f * spB0);
            this->unk_880 = spAC;
            if (spAC < 0.1f) {
                this->unk_880 = 0.1f;
            }
        }
        
        if ((this->actor.bgCheckFlags & BGCHECKFLAG_PLAYER_WALL_INTERACT) && (sTouchedWallYaw < 0x3000)) {
            CollisionPoly* wallPoly = this->actor.wallPoly;
            
            if (ABS(wallPoly->normal.y) < 600) {
                f32 wallPolyNormalX = COLPOLY_GET_NORMAL(wallPoly->normal.x);
                f32 wallPolyNormalY = COLPOLY_GET_NORMAL(wallPoly->normal.y);
                f32 wallPolyNormalZ = COLPOLY_GET_NORMAL(wallPoly->normal.z);
                f32 wallHeight;
                CollisionPoly* sp7C;
                CollisionPoly* sp78;
                s32 sp74;
                Vec3f sp68;
                f32 sp64;
                f32 sp60;
                s32 temp3;
                
                this->wallDistance = Math3D_UDistPlaneToPos(
                    wallPolyNormalX,
                    wallPolyNormalY,
                    wallPolyNormalZ,
                    wallPoly->dist,
                    &this->actor.world.pos
                );
                
                spB0 = this->wallDistance + 10.0f;
                sp68.x = this->actor.world.pos.x - (spB0 * wallPolyNormalX);
                sp68.z = this->actor.world.pos.z - (spB0 * wallPolyNormalZ);
                sp68.y = this->actor.world.pos.y + this->ageProperties->unk_0C;
                
                sp64 = BgCheck_EntityRaycastDown1(&play->colCtx, &sp7C, &sp68);
                wallHeight = sp64 - this->actor.world.pos.y;
                this->wallHeight = wallHeight;
                
                if (
                    (this->wallHeight < 18.0f) ||
                    BgCheck_EntityCheckCeiling(
                        &play->colCtx,
                        &sp60,
                        &this->actor.world.pos,
                        (sp64 - this->actor.world.pos.y) + 20.0f,
                        &sp78,
                        &sp74,
                        &this->actor
                    )
                ) {
                    this->wallHeight = 399.96002f;
                } else {
                    sWallCheckOffset.y = (sp64 + 5.0f) - this->actor.world.pos.y;
                    
                    if (
                        Player_RelativeLineRaycast(play, this, &sWallCheckOffset, &sp78, &sp74, &sWallIntersectPos) &&
                        (temp3 = this->actor.wallYaw - Math_Atan2S(sp78->normal.z, sp78->normal.x),
                        ABS(temp3) < 0x4000) &&
                        !SurfaceType_CheckWallFlag1(&play->colCtx, sp78, sp74)
                    ) {
                        this->wallHeight = 399.96002f;
                    } else if (SurfaceType_CheckWallFlag0(&play->colCtx, wallPoly, this->actor.wallBgId) == 0) {
                        if (this->ageProperties->unk_1C <= this->wallHeight) {
                            if (ABS(sp7C->normal.y) > 28000) {
                                if (this->ageProperties->unk_14 <= this->wallHeight) {
                                    spC7 = 4;
                                } else if (this->ageProperties->unk_18 <= this->wallHeight) {
                                    spC7 = 3;
                                } else {
                                    spC7 = 2;
                                }
                            }
                        } else {
                            spC7 = 1;
                        }
                    }
                }
            }
        }
    } else {
        this->unk_880 = R_RUN_SPEED_LIMIT / 100.0f;
        this->unk_88D = 0;
        this->wallHeight = 0.0f;
    }
    
    if (spC7 == this->unk_88C) {
        if ((this->linearVelocity != 0.0f) && (this->unk_88D < 100)) {
            this->unk_88D++;
        }
    } else {
        this->unk_88C = spC7;
        this->unk_88D = 0;
    }
    
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        sFloorSpecialProperty = SurfaceType_GetFloorType(&play->colCtx, floorPoly, this->actor.floorBgId);
        
        if (!Player_SetupHover(this)) {
            f32 floorPolyNormalX;
            f32 invFloorPolyNormalY;
            f32 floorPolyNormalZ;
            f32 sp4C;
            f32 sp44;
            
            if (this->actor.floorBgId != BGCHECK_SCENE) {
                DynaPoly_SetPlayerOnTop(&play->colCtx, this->actor.floorBgId);
            }
            
            floorPolyNormalX = COLPOLY_GET_NORMAL(floorPoly->normal.x);
            invFloorPolyNormalY = 1.0f / COLPOLY_GET_NORMAL(floorPoly->normal.y);
            floorPolyNormalZ = COLPOLY_GET_NORMAL(floorPoly->normal.z);
            
            sp4C = Math_SinS(this->currentYaw);
            sp44 = Math_CosS(this->currentYaw);
            
            this->unk_898 =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * sp4C) - (floorPolyNormalZ * sp44)) * invFloorPolyNormalY);
            this->unk_89A =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * sp44) - (floorPolyNormalZ * sp4C)) * invFloorPolyNormalY);
            
            sp4C = Math_SinS(this->actor.shape.rot.y);
            sp44 = Math_CosS(this->actor.shape.rot.y);
            
            sFloorPitch =
                Math_Atan2S(1.0f, (-(floorPolyNormalX * sp4C) - (floorPolyNormalZ * sp44)) * invFloorPolyNormalY);
            
            Player_WalkOnSlope(play, this, floorPoly);
        }
    } else {
        Player_SetupHover(this);
    }
    
    if (this->unk_A7B == sFloorSpecialProperty) {
        this->unk_A79++;
    } else {
        this->unk_A7B = sFloorSpecialProperty;
        this->unk_A79 = 0;
    }
}

void Player_UpdateCamAndSeqModes(PlayState* play, Player* this) {
    u8 seqMode;
    Actor* unk_664;
    s32 camMode;
    
    if (this->actor.category == ACTORCAT_PLAYER) {
        seqMode = SEQ_MODE_DEFAULT;
        
        if (this->csMode != 0) {
            Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_NORMAL);
        } else if (!(this->stateFlags1 & PLAYER_STATE1_20)) {
            if ((this->actor.parent != NULL) && (this->stateFlags3 & PLAYER_STATE3_7)) {
                camMode = CAM_MODE_HOOKSHOT;
                Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, this->actor.parent);
            } else if (Player_StartKnockback == this->func_674) {
                camMode = CAM_MODE_STILL;
            } else if (this->stateFlags2 & PLAYER_STATE2_8) {
                camMode = CAM_MODE_PUSHPULL;
            } else if ((unk_664 = this->unk_664) != NULL) {
                if (CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_8)) {
                    camMode = CAM_MODE_TALK;
                } else if (this->stateFlags1 & PLAYER_STATE1_16) {
                    if (this->stateFlags1 & PLAYER_STATE1_25) {
                        camMode = CAM_MODE_FOLLOWBOOMERANG;
                    } else {
                        camMode = CAM_MODE_FOLLOWTARGET;
                    }
                } else {
                    camMode = CAM_MODE_BATTLE;
                }
                Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, unk_664);
            } else if (this->stateFlags1 & PLAYER_STATE1_12) {
                camMode = CAM_MODE_CHARGE;
            } else if (this->stateFlags1 & PLAYER_STATE1_25) {
                camMode = CAM_MODE_FOLLOWBOOMERANG;
                Camera_SetParam(Play_GetCamera(play, CAM_ID_MAIN), 8, this->boomerangActor);
            } else if (this->stateFlags1 & (PLAYER_STATE1_13 | PLAYER_STATE1_14)) {
                if (Player_IsFriendlyZTargeting(this)) {
                    camMode = CAM_MODE_HANGZ;
                } else {
                    camMode = CAM_MODE_HANG;
                }
            } else if (this->stateFlags1 & (PLAYER_STATE1_17 | PLAYER_STATE1_30)) {
                if (func_8002DD78(this) || Player_IsAimingReady_Boomerang(this)) {
                    camMode = CAM_MODE_BOWARROWZ;
                } else if (this->stateFlags1 & PLAYER_STATE1_21) {
                    camMode = CAM_MODE_CLIMBZ;
                } else {
                    camMode = CAM_MODE_TARGET;
                }
            } else if (this->stateFlags1 & (PLAYER_STATE1_18 | PLAYER_STATE1_21)) {
                if ((Player_JumpUpToLedge == this->func_674) || (this->stateFlags1 & PLAYER_STATE1_21)) {
                    camMode = CAM_MODE_CLIMB;
                } else {
                    camMode = CAM_MODE_JUMP;
                }
            } else if (this->stateFlags1 & PLAYER_STATE1_19) {
                camMode = CAM_MODE_FREEFALL;
            } else if (
                (this->meleeWeaponState != 0) && (this->meleeWeaponAnimation >= PLAYER_MWA_FORWARD_SLASH_1H) &&
                (this->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H)
            ) {
                camMode = CAM_MODE_STILL;
            } else {
                camMode = CAM_MODE_NORMAL;
                if (
                    (this->linearVelocity == 0.0f) &&
                    (!(this->stateFlags1 & PLAYER_STATE1_23) || (this->rideActor->speedXZ == 0.0f))
                ) {
                    // not moving
                    seqMode = SEQ_MODE_STILL;
                }
            }
            
            Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), camMode);
        } else {
            // First person mode
            seqMode = SEQ_MODE_STILL;
        }
        
        if (play->actorCtx.targetCtx.bgmEnemy != NULL) {
            seqMode = SEQ_MODE_ENEMY;
            Audio_SetBgmEnemyVolume(sqrtf(play->actorCtx.targetCtx.bgmEnemy->xyzDistToPlayerSq));
        }
        
        if (play->sceneId != SCENE_TURIBORI) {
            Audio_SetSequenceMode(seqMode);
        }
    }
}

static Vec3f sStickFlameVelocity = { 0.0f, 0.5f, 0.0f };
static Vec3f sStickFlameAccel = { 0.0f, 0.5f, 0.0f };

static Color_RGBA8 sStickFlamePrimColor = { 255, 255, 100, 255 };
static Color_RGBA8 sStickFlameEnvColor = { 255, 50, 0, 0 };

void Player_UpdateDekuStick(PlayState* play, Player* this) {
    f32 temp;
    
    if (this->unk_85C == 0.0f) {
        Player_UseItem(play, this, 0xFF);
        
        return;
    }
    
    temp = 1.0f;
    if (DECR(this->unk_860) == 0) {
        Inventory_ChangeAmmo(ITEM_STICK, -1);
        this->unk_860 = 1;
        temp = 0.0f;
        this->unk_85C = temp;
    } else if (this->unk_860 > 200) {
        temp = (210 - this->unk_860) / 10.0f;
    } else if (this->unk_860 < 20) {
        temp = this->unk_860 / 20.0f;
        this->unk_85C = temp;
    }
    
    func_8002836C(
        play,
        &this->meleeWeaponInfo[0].tip,
        &sStickFlameVelocity,
        &sStickFlameAccel,
        &sStickFlamePrimColor,
        &sStickFlameEnvColor,
        temp * 200.0f,
        0,
        8
    );
}

void Player_ElectricShock(PlayState* play, Player* this) {
    Vec3f shockPos;
    Vec3f* randBodyPart;
    s32 shockScale;
    
    this->shockTimer--;
    this->unk_892 += this->shockTimer;
    
    if (this->unk_892 > 20) {
        shockScale = this->shockTimer * 2;
        this->unk_892 -= 20;
        
        if (shockScale > 40) {
            shockScale = 40;
        }
        
        randBodyPart = this->bodyPartsPos + (s32)Rand_ZeroFloat(PLAYER_BODYPART_MAX - 0.1f);
        shockPos.x = (Rand_CenteredFloat(5.0f) + randBodyPart->x) - this->actor.world.pos.x;
        shockPos.y = (Rand_CenteredFloat(5.0f) + randBodyPart->y) - this->actor.world.pos.y;
        shockPos.z = (Rand_CenteredFloat(5.0f) + randBodyPart->z) - this->actor.world.pos.z;
        
        EffectSsFhgFlash_SpawnShock(play, &this->actor, &shockPos, shockScale, FHGFLASH_SHOCK_PLAYER);
        func_8002F8F0(&this->actor, NA_SE_PL_SPARK - SFX_FLAG);
    }
}

void Player_Burning(PlayState* play, Player* this) {
    s32 spawnedFlame;
    u8* timerPtr;
    s32 timerStep;
    f32 flameScale;
    f32 flameIntensity;
    s32 dmgCooldown;
    s32 i;
    s32 sp58;
    s32 sp54;
    
    if (this->currentTunic == PLAYER_TUNIC_GORON) {
        sp54 = 20;
    } else {
        sp54 = (s32)(this->linearVelocity * 0.4f) + 1;
    }
    
    spawnedFlame = false;
    timerPtr = this->flameTimers;
    
    if (this->stateFlags2 & PLAYER_STATE2_3) {
        sp58 = 100;
    } else {
        sp58 = 0;
    }
    
    Player_BurnDekuShield(this, play);
    
    for (i = 0; i < PLAYER_BODYPART_MAX; i++, timerPtr++) {
        timerStep = sp58 + sp54;
        
        if (*timerPtr <= timerStep) {
            *timerPtr = 0;
        } else {
            spawnedFlame = true;
            *timerPtr -= timerStep;
            
            if (*timerPtr > 20.0f) {
                flameIntensity = (*timerPtr - 20.0f) * 0.01f;
                flameScale = CLAMP(flameIntensity, 0.19999999f, 0.2f);
            } else {
                flameScale = *timerPtr * 0.01f;
            }
            
            flameIntensity = (*timerPtr - 25.0f) * 0.02f;
            flameIntensity = CLAMP(flameIntensity, 0.0f, 1.0f);
            EffectSsFireTail_SpawnFlameOnPlayer(play, flameScale, i, flameIntensity);
        }
    }
    
    if (spawnedFlame) {
        func_8002F7DC(&this->actor, NA_SE_EV_TORCH - SFX_FLAG);
        
        if (play->sceneId == SCENE_JYASINBOSS) {
            dmgCooldown = 0;
        } else {
            dmgCooldown = 7;
        }
        
        if ((dmgCooldown & play->gameplayFrames) == 0) {
            Player_InflictDamage(play, -1);
        }
    } else {
        this->isBurning = false;
    }
}

void Player_SetupStoneOfAgonyRumble(Player* this) {
    if (CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY)) {
        f32 temp = 200000.0f - (this->unk_6A4 * 5.0f);
        
        if (temp < 0.0f) {
            temp = 0.0f;
        }
        
        this->unk_6A0 += temp;
        if (this->unk_6A0 > 4000000.0f) {
            this->unk_6A0 = 0.0f;
            Player_SetRumble(this, 120, 20, 10, 0);
        }
    }
}

static s8 sLinkActionCsCmds[] = {
    0,  3,  3,  5,   4,   8,    9,    13,   14,   15, 16, 17, 18, -22, 23,   24,   25,  26,  27,   28,  29,  31,  32, 33, 34, -35,
    30, 36, 38, -39, -40, -41,  42,   43,   45,   46, 0,  0,  0,  67,  48,   47,   -50, 51,  -52,  -53, 54,  55,  56, 57, 58, 59,
    60, 61, 62, 63,  64,  -65,  -66,  68,   11,   69, 70, 71, 8,  8,   72,   73,   78,  79,  80,   89,  90,  91,  92, 77, 19, 94,
};
static Vec3f sHorseRaycastOffset = { 0.0f, 0.0f, 200.0f };
static f32 sWaterConveyorSpeeds[] = { 2.0f, 4.0f, 7.0f };
static f32 sFloorConveyorSpeeds[] = { 0.5f, 1.0f, 3.0f };

void Player_UpdateCommon(Player* this, PlayState* play, Input* input) {
    sControlInput = input;
    
    if (this->unk_A86 < 0) {
        this->unk_A86++;
        if (this->unk_A86 == 0) {
            this->unk_A86 = 1;
            func_80078884(NA_SE_OC_REVENGE);
        }
    }
    
    Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.home.pos);
    
    if (this->unk_A73 != 0) {
        this->unk_A73--;
    }
    
    if (this->unk_88E != 0) {
        this->unk_88E--;
    }
    
    if (this->unk_A87 != 0) {
        this->unk_A87--;
    }
    
    if (this->invincibilityTimer < 0) {
        this->invincibilityTimer++;
    } else if (this->invincibilityTimer > 0) {
        this->invincibilityTimer--;
    }
    
    if (this->unk_890 != 0) {
        this->unk_890--;
    }
    
    Player_SetupDoActionText(play, this);
    Player_SetupZTargeting(this, play);
    
    if ((this->heldItemActionParam == PLAYER_AP_STICK) && (this->unk_860 != 0)) {
        Player_UpdateDekuStick(play, this);
    } else if ((this->heldItemActionParam == PLAYER_AP_FISHING_POLE) && (this->unk_860 < 0)) {
        this->unk_860++;
    }
    
    if (this->shockTimer != 0) {
        Player_ElectricShock(play, this);
    }
    
    if (this->isBurning) {
        Player_Burning(play, this);
    }
    
    if (
        (this->stateFlags3 & PLAYER_STATE3_RESTORE_NAYRUS_LOVE) && (gSaveContext.nayrusLoveTimer != 0) &&
        (gSaveContext.magicState == MAGIC_STATE_IDLE)
    ) {
        gSaveContext.magicState = MAGIC_STATE_METER_FLASH_1;
        Player_SpawnMagicSpellActor(play, this, 1);
        this->stateFlags3 &= ~PLAYER_STATE3_RESTORE_NAYRUS_LOVE;
    }
    
    if (this->stateFlags2 & PLAYER_STATE2_15) {
        if (!(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
            Player_StopMovement(this);
            Actor_MoveForward(&this->actor);
        }
        
        Player_UpdateBgcheck(play, this);
    } else {
        f32 temp_f0;
        f32 phi_f12;
        
        if (this->currentBoots != this->prevBoots) {
            if (this->currentBoots == PLAYER_BOOTS_IRON) {
                if (this->stateFlags1 & PLAYER_STATE1_27) {
                    Player_ResetSubCam(play, this);
                    if (this->ageProperties->unk_2C < this->actor.yDistToWater) {
                        this->stateFlags2 |= PLAYER_STATE2_10;
                    }
                }
            } else {
                if (this->stateFlags1 & PLAYER_STATE1_27) {
                    if ((this->prevBoots == PLAYER_BOOTS_IRON) || (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                        func_8083D36C(play, this);
                        this->stateFlags2 &= ~PLAYER_STATE2_10;
                    }
                }
            }
            
            this->prevBoots = this->currentBoots;
        }
        
        if ((this->actor.parent == NULL) && (this->stateFlags1 & PLAYER_STATE1_23)) {
            this->actor.parent = this->rideActor;
            Player_SetupRideHorse(play, this);
            this->stateFlags1 |= PLAYER_STATE1_23;
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_uma_wait_1);
            Player_SetupAnimMovement(play, this, 0x9B);
            this->unk_850 = 99;
        }
        
        if (this->unk_844 == 0) {
            this->unk_845 = 0;
        } else if (this->unk_844 < 0) {
            this->unk_844++;
        } else {
            this->unk_844--;
        }
        
        Math_ScaledStepToS(&this->unk_6C2, 0, 400);
        func_80032CB4(this->unk_3A8, 20, 80, 6);
        
        this->actor.shape.face = this->unk_3A8[0] + ((play->gameplayFrames & 32) ? 0 : 3);
        
        if (this->currentMask == PLAYER_MASK_BUNNY) {
            Player_BunnyHoodPhysics(this);
        }
        
        if (func_8002DD6C(this) != 0) {
            Player_BowStringMoveAfterShot(this);
        }
        
        if (!(this->skelAnime.moveFlags & 0x80)) {
            if (
                ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && (sFloorSpecialProperty == 5) &&
                (this->currentBoots != PLAYER_BOOTS_IRON)) ||
                ((this->currentBoots == PLAYER_BOOTS_HOVER) &&
                !(this->stateFlags1 & (PLAYER_STATE1_27 | PLAYER_STATE1_29)))
            ) {
                f32 sp70 = this->linearVelocity;
                s16 sp6E = this->currentYaw;
                s16 yawDiff = this->actor.world.rot.y - sp6E;
                
                if ((ABS(yawDiff) > 0x6000) && (this->actor.speedXZ != 0.0f)) {
                    sp70 = 0.0f;
                    sp6E += 0x8000;
                }
                
                if (Math_StepToF(&this->actor.speedXZ, sp70, 0.35f) && (sp70 == 0.0f)) {
                    this->actor.world.rot.y = this->currentYaw;
                }
                
                if (this->linearVelocity != 0.0f) {
                    s32 phi_v0;
                    
                    phi_v0 = (fabsf(this->linearVelocity) * 700.0f) - (fabsf(this->actor.speedXZ) * 100.0f);
                    phi_v0 = CLAMP(phi_v0, 0, 1350);
                    
                    Math_ScaledStepToS(&this->actor.world.rot.y, sp6E, phi_v0);
                }
                
                if ((this->linearVelocity == 0.0f) && (this->actor.speedXZ != 0.0f)) {
                    func_800F4138(&this->actor.projectedPos, 0xD0, this->actor.speedXZ);
                }
            } else {
                this->actor.speedXZ = this->linearVelocity;
                this->actor.world.rot.y = this->currentYaw;
            }
            
            func_8002D868(&this->actor);
            
            if (
                (this->pushedSpeed != 0.0f) && !Player_InCsMode(play) &&
                !(this->stateFlags1 & (PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_21)) &&
                (Player_JumpUpToLedge != this->func_674) && (Player_UpdateMagicSpell != this->func_674)
            ) {
                this->actor.velocity.x += this->pushedSpeed * Math_SinS(this->pushedYaw);
                this->actor.velocity.z += this->pushedSpeed * Math_CosS(this->pushedYaw);
            }
            
            func_8002D7EC(&this->actor);
            Player_UpdateBgcheck(play, this);
        } else {
            sFloorSpecialProperty = 0;
            this->unk_A7A = 0;
            
            if (!(this->stateFlags1 & PLAYER_STATE1_0) && (this->stateFlags1 & PLAYER_STATE1_23)) {
                EnHorse* rideActor = (EnHorse*)this->rideActor;
                CollisionPoly* sp5C;
                s32 sp58;
                Vec3f sp4C;
                
                if (!(rideActor->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
                    Player_RelativeFloorRaycast_PolyInfo(play, this, &sHorseRaycastOffset, &sp4C, &sp5C, &sp58);
                } else {
                    sp5C = rideActor->actor.floorPoly;
                    sp58 = rideActor->actor.floorBgId;
                }
                
                if ((sp5C != NULL) && Player_SetupExit(play, this, sp5C, sp58)) {
                    if (DREG(25) != 0) {
                        DREG(25) = 0;
                    } else {
                        AREG(6) = 1;
                    }
                }
            }
            
            sConveyorSpeedIndex = 0;
            this->pushedSpeed = 0.0f;
        }
        
        // This block applies the bg conveyor to pushedSpeed
        if ((sConveyorSpeedIndex != 0) && (this->currentBoots != PLAYER_BOOTS_IRON)) {
            f32 conveyorSpeed;
            
            // converts 1-index to 0-index
            sConveyorSpeedIndex--;
            
            if (!sIsFloorConveyor) {
                conveyorSpeed = sWaterConveyorSpeeds[sConveyorSpeedIndex];
                
                if (!(this->stateFlags1 & PLAYER_STATE1_27)) {
                    conveyorSpeed *= 0.25f;
                }
            } else {
                conveyorSpeed = sFloorConveyorSpeeds[sConveyorSpeedIndex];
            }
            
            Math_StepToF(&this->pushedSpeed, conveyorSpeed, conveyorSpeed * 0.1f);
            
            Math_ScaledStepToS(
                &this->pushedYaw,
                sConveyorYaw,
                ((this->stateFlags1 & PLAYER_STATE1_27) ? 400.0f : 800.0f) * conveyorSpeed
            );
        } else if (this->pushedSpeed != 0.0f) {
            Math_StepToF(&this->pushedSpeed, 0.0f, (this->stateFlags1 & PLAYER_STATE1_27) ? 0.5f : 1.0f);
        }
        
        if (!Player_InBlockingCsMode(play, this) && !(this->stateFlags2 & PLAYER_STATE2_18)) {
            func_8083D53C(play, this);
            
            if ((this->actor.category == ACTORCAT_PLAYER) && (gSaveContext.health == 0)) {
                if (this->stateFlags1 & (PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_21)) {
                    Player_ResetAttributes(play, this);
                    Player_SetupFallFromLedge(this, play);
                } else if ((this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || (this->stateFlags1 & PLAYER_STATE1_27)) {
                    Player_SetupDie(
                        play,
                        this,
                        Player_IsSwimming(this)       ? &gPlayerAnim_link_swimer_swim_down
                  : (this->shockTimer != 0) ? &gPlayerAnim_link_normal_electric_shock_end
                                : &gPlayerAnim_link_derth_rebirth
                    );
                }
            } else {
                if (
                    (this->actor.parent == NULL) && ((play->transitionTrigger == TRANS_TRIGGER_START) ||
                    (this->unk_A87 != 0) || !Player_UpdateDamage(this, play))
                ) {
                    Player_SetupMidairBehavior(this, play);
                } else {
                    this->fallStartHeight = this->actor.world.pos.y;
                }
                Player_SetupStoneOfAgonyRumble(this);
            }
        }
        
        if (
            (play->csCtx.state != CS_STATE_IDLE) && (this->csMode != 6) && !(this->stateFlags1 & PLAYER_STATE1_23) &&
            !(this->stateFlags2 & PLAYER_STATE2_7) && (this->actor.category == ACTORCAT_PLAYER)
        ) {
            CsCmdActorAction* linkActionCsCmd = play->csCtx.linkAction;
            
            if ((linkActionCsCmd != NULL) && (sLinkActionCsCmds[linkActionCsCmd->action] != 0)) {
                func_8002DF54(play, NULL, 6);
                Player_StopMovement(this);
            } else if (
                (this->csMode == 0) && !(this->stateFlags2 & PLAYER_STATE2_10) &&
                (play->csCtx.state != CS_STATE_UNSKIPPABLE_INIT)
            ) {
                func_8002DF54(play, NULL, 0x31);
                Player_StopMovement(this);
            }
        }
        
        if (this->csMode != 0) {
            if (
                (this->csMode != 7) ||
                !(this->stateFlags1 & (PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_21 | PLAYER_STATE1_26))
            ) {
                this->unk_6AD = 3;
            } else if (Player_StartCutscene != this->func_674) {
                Player_CutsceneEnd(play, this, NULL);
            }
        } else {
            this->prevCsMode = 0;
        }
        
        func_8083D6EC(play, this);
        
        if ((this->unk_664 == NULL) && (this->naviTextId == 0)) {
            this->stateFlags2 &= ~(PLAYER_STATE2_1 | PLAYER_STATE2_21);
        }
        
        this->stateFlags1 &= ~(PLAYER_STATE1_SWINGING_BOTTLE | PLAYER_STATE1_9 | PLAYER_STATE1_12 | PLAYER_STATE1_22);
        this->stateFlags2 &= ~(PLAYER_STATE2_0 | PLAYER_STATE2_2 | PLAYER_STATE2_3 | PLAYER_STATE2_5 | PLAYER_STATE2_6 |
            PLAYER_STATE2_8 | PLAYER_STATE2_9 | PLAYER_STATE2_12 | PLAYER_STATE2_14 |
            PLAYER_STATE2_16 | PLAYER_STATE2_22 | PLAYER_STATE2_26);
        this->stateFlags3 &= ~PLAYER_STATE3_4;
        
        Player_StepLookToZero(this);
        Player_UpdateAnalogInput(play, this);
        
        if (this->stateFlags1 & PLAYER_STATE1_27) {
            sWaterSpeedScale = 0.5f;
        } else {
            sWaterSpeedScale = 1.0f;
        }
        
        sWaterSpeedInvScale = 1.0f / sWaterSpeedScale;
        sActiveItemUseFlag = sActiveItemUseFlag2 = 0;
        sCurrentMask = this->currentMask;
        
        if (!(this->stateFlags3 & PLAYER_STATE3_2)) {
            Assert(this->func_674 != NULL);
            this->func_674(this, play);
        }
        
        Player_UpdateCamAndSeqModes(play, this);
        
        if (this->skelAnime.moveFlags & 8) {
            AnimationContext_SetMoveActor(
                play,
                &this->actor,
                &this->skelAnime,
                (this->skelAnime.moveFlags & 4) ? 1.0f : this->ageProperties->unk_08
            );
        }
        
        Player_UpdateYaw(this, play);
        
        if (CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_8)) {
            this->targetActorDistance = 0.0f;
        } else {
            this->targetActor = NULL;
            this->targetActorDistance = FLT_MAX;
            this->exchangeItemId = EXCH_ITEM_NONE;
        }
        
        if (!(this->stateFlags1 & PLAYER_STATE1_11)) {
            this->interactRangeActor = NULL;
            this->getItemDirection = 0x6000;
        }
        
        if (this->actor.parent == NULL) {
            this->rideActor = NULL;
        }
        
        this->naviTextId = 0;
        
        if (!(this->stateFlags2 & PLAYER_STATE2_25)) {
            this->unk_6A8 = NULL;
        }
        
        this->stateFlags2 &= ~PLAYER_STATE2_23;
        this->unk_6A4 = FLT_MAX;
        
        temp_f0 = this->actor.world.pos.y - this->actor.prevPos.y;
        
        this->doorType = PLAYER_DOORTYPE_NONE;
        this->unk_8A1 = 0;
        this->unk_684 = NULL;
        
        phi_f12 =
            ((this->bodyPartsPos[PLAYER_BODYPART_L_FOOT].y + this->bodyPartsPos[PLAYER_BODYPART_R_FOOT].y) * 0.5f) +
            temp_f0;
        temp_f0 += this->bodyPartsPos[PLAYER_BODYPART_HEAD].y + 10.0f;
        
        this->cylinder.dim.height = temp_f0 - phi_f12;
        
        if (this->cylinder.dim.height < 0) {
            phi_f12 = temp_f0;
            this->cylinder.dim.height = -this->cylinder.dim.height;
        }
        
        this->cylinder.dim.yShift = phi_f12 - this->actor.world.pos.y;
        
        if (this->stateFlags1 & PLAYER_STATE1_22) {
            this->cylinder.dim.height = this->cylinder.dim.height * 0.8f;
        }
        
        Collider_UpdateCylinder(&this->actor, &this->cylinder);
        
        if (!(this->stateFlags2 & PLAYER_STATE2_14)) {
            if (!(this->stateFlags1 & (PLAYER_STATE1_7 | PLAYER_STATE1_13 | PLAYER_STATE1_14 | PLAYER_STATE1_23))) {
                CollisionCheck_SetOC(play, &play->colChkCtx, &this->cylinder.base);
            }
            
            if (!(this->stateFlags1 & (PLAYER_STATE1_7 | PLAYER_STATE1_26)) && (this->invincibilityTimer <= 0)) {
                CollisionCheck_SetAC(play, &play->colChkCtx, &this->cylinder.base);
                
                if (this->invincibilityTimer < 0) {
                    CollisionCheck_SetAT(play, &play->colChkCtx, &this->cylinder.base);
                }
            }
        }
        
        AnimationContext_SetNextQueue(play);
    }
    
    Math_Vec3f_Copy(&this->actor.home.pos, &this->actor.world.pos);
    Math_Vec3f_Copy(&this->unk_A88, &this->bodyPartsPos[PLAYER_BODYPART_WAIST]);
    
    if (this->stateFlags1 & (PLAYER_STATE1_7 | PLAYER_STATE1_28 | PLAYER_STATE1_29)) {
        this->actor.colChkInfo.mass = MASS_IMMOVABLE;
    } else {
        this->actor.colChkInfo.mass = 50;
    }
    
    this->stateFlags3 &= ~PLAYER_STATE3_2;
    
    Collider_ResetCylinderAC(play, &this->cylinder.base);
    
    Collider_ResetQuadAT(play, &this->meleeWeaponQuads[0].base);
    Collider_ResetQuadAT(play, &this->meleeWeaponQuads[1].base);
    
    Collider_ResetQuadAC(play, &this->shieldQuad.base);
    Collider_ResetQuadAT(play, &this->shieldQuad.base);
}

static Vec3f D_80854838 = { 0.0f, 0.0f, -30.0f };

void Player_Update(Actor* thisx, PlayState* play) {
    static Vec3f sDogSpawnPos;
    Player* this = (Player*)thisx;
    s32 dogParams;
    Input sp44;
    Actor* dog;
    
    if (Player_DebugMode(this, play)) {
        if (gSaveContext.dogParams < 0) {
            if (Object_GetIndex(&play->objectCtx, OBJECT_DOG) < 0) {
                gSaveContext.dogParams = 0;
            } else {
                gSaveContext.dogParams &= 0x7FFF;
                Player_PosRelativeToPlayerYaw(this, &this->actor.world.pos, &D_80854838, &sDogSpawnPos);
                dogParams = gSaveContext.dogParams;
                
                dog = Actor_Spawn(
                    &play->actorCtx,
                    play,
                    ACTOR_EN_DOG,
                    sDogSpawnPos.x,
                    sDogSpawnPos.y,
                    sDogSpawnPos.z,
                    0,
                    this->actor.shape.rot.y,
                    0,
                    dogParams | 0x8000
                );
                if (dog != NULL) {
                    dog->room = 0;
                }
            }
        }
        
        if ((this->interactRangeActor != NULL) && (this->interactRangeActor->update == NULL)) {
            this->interactRangeActor = NULL;
        }
        
        if ((this->heldActor != NULL) && (this->heldActor->update == NULL)) {
            Player_DetatchHeldActor(play, this);
        }
        
        if (this->stateFlags1 & (PLAYER_STATE1_5 | PLAYER_STATE1_29)) {
            bzero(&sp44, sizeof(sp44));
        } else {
            sp44 = play->state.input[0];
            if (this->unk_88E != 0) {
                sp44.cur.button &= ~(BTN_A | BTN_B | BTN_CUP);
                sp44.press.button &= ~(BTN_A | BTN_B | BTN_CUP);
            }
        }
        
        Player_UpdateCommon(this, play, &sp44);
    }
    
    MREG(52) = this->actor.world.pos.x;
    MREG(53) = this->actor.world.pos.y;
    MREG(54) = this->actor.world.pos.z;
    MREG(55) = this->actor.world.rot.y;
}

static struct_80858AC8 D_80858AC8;
static Vec3s D_80858AD8[25];

static Gfx* sMaskDlists[PLAYER_MASK_MAX - 1] = {
    gLinkChildKeatonMaskDL, gLinkChildSkullMaskDL, gLinkChildSpookyMaskDL,  gLinkChildBunnyHoodDL,
    gLinkChildGoronMaskDL,  gLinkChildZoraMaskDL,  gLinkChildGerudoMaskDL,  gLinkChildMaskOfTruthDL,
};

static Vec3s D_80854864 = { 0, 0, 0 };

void Player_DrawGameplay(PlayState* play, Player* this, s32 lod, Gfx* cullDList, OverrideLimbDrawOpa overrideLimbDraw) {
    static s32 D_8085486C = 255;
    
    OPEN_DISPS(play->state.gfxCtx, "../z_player.c", 19228);
    
    gSPSegment(POLY_OPA_DISP++, 0x0C, cullDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, cullDList);
    
    Player_DrawImpl(
        play,
        this->skelAnime.skeleton,
        this->skelAnime.jointTable,
        this->skelAnime.dListCount,
        lod,
        this->currentTunic,
        this->currentBoots,
        this->actor.shape.face,
        overrideLimbDraw,
        Player_PostLimbDrawGameplay,
        this
    );
    
    if ((overrideLimbDraw == Player_OverrideLimbDrawGameplayDefault) && (this->currentMask != PLAYER_MASK_NONE)) {
        Mtx* sp70 = Graph_Alloc(play->state.gfxCtx, 2 * sizeof(Mtx));
        
        if (this->currentMask == PLAYER_MASK_BUNNY) {
            Vec3s sp68;
            
            gSPSegment(POLY_OPA_DISP++, 0x0B, sp70);
            
            sp68.x = D_80858AC8.unk_02 + 0x3E2;
            sp68.y = D_80858AC8.unk_04 + 0xDBE;
            sp68.z = D_80858AC8.unk_00 - 0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, -240.0f, &sp68);
            Matrix_ToMtx(sp70++, "../z_player.c", 19273);
            
            sp68.x = D_80858AC8.unk_02 - 0x3E2;
            sp68.y = -0xDBE - D_80858AC8.unk_04;
            sp68.z = D_80858AC8.unk_00 - 0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, 240.0f, &sp68);
            Matrix_ToMtx(sp70, "../z_player.c", 19279);
        }
        
        gSPDisplayList(POLY_OPA_DISP++, sMaskDlists[this->currentMask - 1]);
    }
    
    if (
        (this->currentBoots == PLAYER_BOOTS_HOVER) && !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
        !(this->stateFlags1 & PLAYER_STATE1_23) && (this->hoverBootsTimer != 0)
    ) {
        s32 sp5C;
        s32 hoverBootsTimer = this->hoverBootsTimer;
        
        if (this->hoverBootsTimer < 19) {
            if (hoverBootsTimer >= 15) {
                D_8085486C = (19 - hoverBootsTimer) * 51.0f;
            } else if (hoverBootsTimer < 19) {
                sp5C = hoverBootsTimer;
                
                if (sp5C > 9) {
                    sp5C = 9;
                }
                
                D_8085486C = (-sp5C * 4) + 36;
                D_8085486C = D_8085486C * D_8085486C;
                D_8085486C = (s32)((Math_CosS(D_8085486C) * 100.0f) + 100.0f) + 55.0f;
                D_8085486C = D_8085486C * (sp5C * (1.0f / 9.0f));
            }
            
            Matrix_SetTranslateRotateYXZ(
                this->actor.world.pos.x,
                this->actor.world.pos.y + 2.0f,
                this->actor.world.pos.z,
                &D_80854864
            );
            Matrix_Scale(4.0f, 4.0f, 4.0f, MTXMODE_APPLY);
            
            gSPMatrix(
                POLY_XLU_DISP++,
                Matrix_NewMtx(play->state.gfxCtx, "../z_player.c", 19317),
                G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW
            );
            gSPSegment(
                POLY_XLU_DISP++,
                0x08,
                Gfx_TwoTexScroll(
                    play->state.gfxCtx,
                    0,
                    0,
                    0,
                    16,
                    32,
                    1,
                    0,
                    (play->gameplayFrames * -15) % 128,
                    16,
                    32
                )
            );
            gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 255, D_8085486C);
            gDPSetEnvColor(POLY_XLU_DISP++, 120, 90, 30, 128);
            gSPDisplayList(POLY_XLU_DISP++, gHoverBootsCircleDL);
        }
    }
    
    CLOSE_DISPS(play->state.gfxCtx, "../z_player.c", 19328);
}

void Player_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Player* this = (Player*)thisx;
    
    OPEN_DISPS(play->state.gfxCtx, "../z_player.c", 19346);
    
    if (!(this->stateFlags2 & PLAYER_STATE2_29)) {
        OverrideLimbDrawOpa overrideLimbDraw = Player_OverrideLimbDrawGameplayDefault;
        s32 lod;
        
        if ((this->csMode != 0) || (func_8008E9C4(this) && 0) || (this->actor.projectedPos.z < 160.0f)) {
            lod = 0;
        } else {
            lod = 1;
        }
        
        func_80093C80(play);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        
        if (this->invincibilityTimer > 0) {
            this->unk_88F += CLAMP(50 - this->invincibilityTimer, 8, 40);
            POLY_OPA_DISP =
                Gfx_SetFog2(POLY_OPA_DISP, 255, 0, 0, 0, 0, 4000 - (s32)(Math_CosS(this->unk_88F * 256) * 2000.0f));
        }
        
        func_8002EBCC(&this->actor, play, 0);
        func_8002ED80(&this->actor, play, 0);
        
        if (this->unk_6AD != 0) {
            Vec3f projectedHeadPos;
            
            SkinMatrix_Vec3fMtxFMultXYZ(&play->viewProjectionMtxF, &this->actor.focus.pos, &projectedHeadPos);
            if (projectedHeadPos.z < -4.0f) {
                overrideLimbDraw = Player_OverrideLimbDrawGameplayFirstPerson;
            }
        } else if (this->stateFlags2 & PLAYER_STATE2_18) {
            if (this->actor.projectedPos.z < 0.0f) {
                overrideLimbDraw = Player_OverrideLimbDrawGameplay_80090440;
            }
        }
        
        if (this->stateFlags2 & PLAYER_STATE2_26) {
            f32 sp78 = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 600));
            f32 sp74 = BINANG_TO_RAD_ALT2((u16)(play->gameplayFrames * 1000));
            
            Matrix_Push();
            this->actor.scale.y = -this->actor.scale.y;
            Matrix_SetTranslateRotateYXZ(
                this->actor.world.pos.x,
                (this->actor.floorHeight + (this->actor.floorHeight - this->actor.world.pos.y)) +
                (this->actor.shape.yOffset * this->actor.scale.y),
                this->actor.world.pos.z,
                &this->actor.shape.rot
            );
            Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
            Matrix_RotateX(sp78, MTXMODE_APPLY);
            Matrix_RotateY(sp74, MTXMODE_APPLY);
            Matrix_Scale(1.1f, 0.95f, 1.05f, MTXMODE_APPLY);
            Matrix_RotateY(-sp74, MTXMODE_APPLY);
            Matrix_RotateX(-sp78, MTXMODE_APPLY);
            Player_DrawGameplay(play, this, lod, gCullFrontDList, overrideLimbDraw);
            this->actor.scale.y = -this->actor.scale.y;
            Matrix_Pop();
        }
        
        gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BOTH);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BOTH);
        
        Player_DrawGameplay(play, this, lod, gCullBackDList, overrideLimbDraw);
        
        if (this->invincibilityTimer > 0) {
            POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);
        }
        
        if (this->stateFlags2 & PLAYER_STATE2_14) {
            f32 scale = (this->unk_84F >> 1) * 22.0f;
            
            gSPSegment(
                POLY_XLU_DISP++,
                0x08,
                Gfx_TwoTexScroll(
                    play->state.gfxCtx,
                    0,
                    0,
                    (0 - play->gameplayFrames) % 128,
                    32,
                    32,
                    1,
                    0,
                    (play->gameplayFrames * -2) % 128,
                    32,
                    32
                )
            );
            
            Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
            gSPMatrix(
                POLY_XLU_DISP++,
                Matrix_NewMtx(play->state.gfxCtx, "../z_player.c", 19459),
                G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW
            );
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 50, 100, 255);
            gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment3DL);
        }
        
        if (this->unk_862 > 0) {
            Player_DrawGetItem(play, this);
        }
    }
    
    CLOSE_DISPS(play->state.gfxCtx, "../z_player.c", 19473);
}

void Player_Destroy(Actor* thisx, PlayState* play) {
    Player* this = (Player*)thisx;
    
    Effect_Delete(play, this->meleeWeaponEffectIndex);
    
    Collider_DestroyCylinder(play, &this->cylinder);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[0]);
    Collider_DestroyQuad(play, &this->meleeWeaponQuads[1]);
    Collider_DestroyQuad(play, &this->shieldQuad);
    
    Magic_Reset(play);
    
    gSaveContext.linkAge = play->linkAgeOnLoad;
}

s16 Player_SetFirstPersonAimLookAngles(PlayState* play, Player* this, s32 arg2, s16 arg3) {
    s32 temp1;
    s16 temp2;
    s16 temp3;
    
    if (!func_8002DD78(this) && !Player_IsAimingReady_Boomerang(this) && (arg2 == 0)) {
        temp2 = sControlInput->rel.stick_y * 240.0f;
        Math_SmoothStepToS(&this->actor.focus.rot.x, temp2, 14, 4000, 30);
        
        temp2 = sControlInput->rel.stick_x * -16.0f;
        temp2 = CLAMP(temp2, -3000, 3000);
        this->actor.focus.rot.y += temp2;
    } else {
        temp1 = (this->stateFlags1 & PLAYER_STATE1_23) ? 3500 : 14000;
        temp3 = ((sControlInput->rel.stick_y >= 0) ? 1 : -1) *
            (s32)((1.0f - Math_CosS(sControlInput->rel.stick_y * 200)) * 1500.0f);
        this->actor.focus.rot.x += temp3;
        this->actor.focus.rot.x = CLAMP(this->actor.focus.rot.x, -temp1, temp1);
        
        temp1 = 19114;
        temp2 = this->actor.focus.rot.y - this->actor.shape.rot.y;
        temp3 = ((sControlInput->rel.stick_x >= 0) ? 1 : -1) *
            (s32)((1.0f - Math_CosS(sControlInput->rel.stick_x * 200)) * -1500.0f);
        temp2 += temp3;
        this->actor.focus.rot.y = CLAMP(temp2, -temp1, temp1) + this->actor.shape.rot.y;
    }
    
    this->unk_6AE |= 2;
    
    return Player_UpdateLookRot(this, (play->shootingGalleryStatus != 0) || func_8002DD78(this) || Player_IsAimingReady_Boomerang(this)) - arg3;
}

void Player_UpdateSwimMovement(Player* this, f32* arg1, f32 arg2, s16 arg3) {
    f32 temp1;
    f32 temp2;
    
    temp1 = this->skelAnime.curFrame - 10.0f;
    
    temp2 = (R_RUN_SPEED_LIMIT / 100.0f) * 0.8f;
    if (*arg1 > temp2) {
        *arg1 = temp2;
    }
    
    if ((0.0f < temp1) && (temp1 < 10.0f)) {
        temp1 *= 6.0f;
    } else {
        temp1 = 0.0f;
        arg2 = 0.0f;
    }
    
    Math_AsymStepToF(arg1, arg2 * 0.8f, temp1, (fabsf(*arg1) * 0.02f) + 0.05f);
    Math_ScaledStepToS(&this->currentYaw, arg3, 1600);
}

void Player_SetVerticalWaterVelocity(Player* this) {
    f32 phi_f18;
    f32 phi_f16;
    f32 phi_f14;
    f32 yDistToWater;
    
    phi_f14 = -5.0f;
    
    phi_f16 = this->ageProperties->unk_28;
    if (this->actor.velocity.y < 0.0f) {
        phi_f16 += 1.0f;
    }
    
    if (this->actor.yDistToWater < phi_f16) {
        if (this->actor.velocity.y <= 0.0f) {
            phi_f16 = 0.0f;
        } else {
            phi_f16 = this->actor.velocity.y * 0.5f;
        }
        phi_f18 = -0.1f - phi_f16;
    } else {
        if (
            !(this->stateFlags1 & PLAYER_STATE1_7) && (this->currentBoots == PLAYER_BOOTS_IRON) &&
            (this->actor.velocity.y >= -3.0f)
        ) {
            phi_f18 = -0.2f;
        } else {
            phi_f14 = 2.0f;
            if (this->actor.velocity.y >= 0.0f) {
                phi_f16 = 0.0f;
            } else {
                phi_f16 = this->actor.velocity.y * -0.3f;
            }
            phi_f18 = phi_f16 + 0.1f;
        }
        
        yDistToWater = this->actor.yDistToWater;
        if (yDistToWater > 100.0f) {
            this->stateFlags2 |= PLAYER_STATE2_10;
        }
    }
    
    this->actor.velocity.y += phi_f18;
    
    if (((this->actor.velocity.y - phi_f14) * phi_f18) > 0) {
        this->actor.velocity.y = phi_f14;
    }
    
    this->actor.gravity = 0.0f;
}

void Player_PlaySwimAnim(PlayState* play, Player* this, Input* input, f32 arg3) {
    f32 temp;
    
    if ((input != NULL) && CHECK_BTN_ANY(input->press.button, BTN_A | BTN_B)) {
        temp = 1.0f;
    } else {
        temp = 0.5f;
    }
    
    temp *= arg3;
    
    if (temp < 1.0f) {
        temp = 1.0f;
    }
    
    this->skelAnime.playSpeed = temp;
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_FirstPersonAiming(Player* this, PlayState* play) {
    if (this->stateFlags1 & PLAYER_STATE1_27) {
        Player_SetVerticalWaterVelocity(this);
        Player_UpdateSwimMovement(this, &this->linearVelocity, 0, this->actor.shape.rot.y);
    } else {
        Player_StepLinearVelToZero(this);
    }
    
    if ((this->unk_6AD == 2) && (func_8002DD6C(this) || Player_IsAiming_Boomerang(this))) {
        Player_SetupCurrentUpperAction(this, play);
    }
    
    if (
        (this->csMode != 0) || (this->unk_6AD == 0) || (this->unk_6AD >= 4) || Player_SetupStartEnemyZTargeting(this) ||
        (this->unk_664 != NULL) || !Player_SetupCameraMode(play, this) ||
        (((this->unk_6AD == 2) && (CHECK_BTN_ANY(sControlInput->press.button, BTN_A | BTN_B | BTN_R) ||
        Player_IsFriendlyZTargeting(this) || (!func_8002DD78(this) && !Player_IsAimingReady_Boomerang(this)))) ||
        ((this->unk_6AD == 1) &&
        CHECK_BTN_ANY(
            sControlInput->press.button,
            BTN_A | BTN_B | BTN_R | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN
        )))
    ) {
        Player_ClearLookAndAttention(this, play);
        func_80078884(NA_SE_SY_CAMERA_ZOOM_UP);
    } else if ((DECR(this->unk_850) == 0) || (this->unk_6AD != 2)) {
        if (func_8008F128(this)) {
            this->unk_6AE |= 0x43;
        } else {
            this->actor.shape.rot.y = Player_SetFirstPersonAimLookAngles(play, this, 0, 0);
        }
    }
    
    this->currentYaw = this->actor.shape.rot.y;
}

s32 Player_SetupShootingGalleryPlay(PlayState* play, Player* this) {
    if (play->shootingGalleryStatus != 0) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetActionFunc(play, this, Player_ShootingGalleryPlay, 0);
        
        if (!func_8002DD6C(this) || Player_HoldsHookshot(this)) {
            Player_UseItem(play, this, 3);
        }
        
        this->stateFlags1 |= PLAYER_STATE1_20;
        Player_PlayAnimOnce(play, this, Player_GetAnim_StandingStill(this));
        Player_StopMovement(this);
        Player_ResetLookAngles(this);
        
        return 1;
    }
    
    return 0;
}

void Player_SetOcarinaItemAP(Player* this) {
    this->itemActionParam =
        (INV_CONTENT(ITEM_OCARINA_FAIRY) == ITEM_OCARINA_FAIRY) ? PLAYER_AP_OCARINA_FAIRY : PLAYER_AP_OCARINA_TIME;
}

s32 Player_SetupForcePullOcarina(PlayState* play, Player* this) {
    if (this->stateFlags3 & PLAYER_STATE3_5) {
        this->stateFlags3 &= ~PLAYER_STATE3_5;
        Player_SetOcarinaItemAP(this);
        this->unk_6AD = 4;
        Player_SetupItemCsOrFirstPerson(this, play);
        
        return 1;
    }
    
    return 0;
}

void Player_TalkWithActor(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5;
    Player_SetupCurrentUpperAction(this, play);
    
    if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
        this->actor.flags &= ~ACTOR_FLAG_8;
        
        if (!CHECK_FLAG_ALL(this->targetActor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_2)) {
            this->stateFlags2 &= ~PLAYER_STATE2_13;
        }
        
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
        
        if (!Player_SetupForcePullOcarina(play, this) && !Player_SetupShootingGalleryPlay(play, this) && !Player_SetupCutscene(play, this)) {
            if ((this->targetActor != this->interactRangeActor) || !Player_SetupGetItemOrHoldBehavior(this, play)) {
                if (this->stateFlags1 & PLAYER_STATE1_23) {
                    s32 sp24 = this->unk_850;
                    Player_SetupRideHorse(play, this);
                    this->unk_850 = sp24;
                } else if (Player_IsSwimming(this)) {
                    Player_SetupSwimIdle(play, this);
                } else {
                    Player_SetupStandingStillMorph(this, play);
                }
            }
        }
        
        this->unk_88E = 10;
        
        return;
    }
    
    if (this->stateFlags1 & PLAYER_STATE1_23) {
        Player_RideHorse(this, play);
    } else if (Player_IsSwimming(this)) {
        Player_UpdateSwimIdle(this, play);
    } else if (!func_8008E9C4(this) && LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->skelAnime.moveFlags != 0) {
            Player_EndAnimMovement(this);
            if (
                (this->targetActor->category == ACTORCAT_NPC) &&
                (this->heldItemActionParam != PLAYER_AP_FISHING_POLE)
            ) {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_normal_talk_free);
            } else {
                Player_PlayAnimLoop(play, this, Player_GetAnim_StandingStill(this));
            }
        } else {
            Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_normal_talk_free_wait);
        }
    }
    
    if (this->unk_664 != NULL) {
        this->currentYaw = this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_GrabPushPullWall(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    s32 temp;
    
    this->stateFlags2 |= PLAYER_STATE2_0 | PLAYER_STATE2_6 | PLAYER_STATE2_8;
    Player_SetPosYaw_PushPull(play, this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!Player_SetupPushPullWallIdle(play, this)) {
            Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.0f, play);
            temp = Player_GetPushPullDir(this, &sp34, &sp32);
            if (temp > 0) {
                Player_SetupPushWall(this, play);
            } else if (temp < 0) {
                Player_SetupPullWall(this, play);
            }
        }
    }
}

void func_8084B840(PlayState* play, Player* this, f32 arg2) {
    if (this->actor.wallBgId != BGCHECK_SCENE) {
        DynaPolyActor* dynaPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
        
        if (dynaPolyActor != NULL) {
            func_8002DFA4(dynaPolyActor, arg2, this->actor.world.rot.y);
        }
    }
}

static struct_80832924 D_80854870[] = {
    { NA_SE_PL_SLIP, 0x1003  },
    { NA_SE_PL_SLIP, -0x1015 },
};

void Player_PushWall(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    s32 temp;
    
    this->stateFlags2 |= PLAYER_STATE2_0 | PLAYER_STATE2_6 | PLAYER_STATE2_8;
    
    if (Player_LoopAnimContinuously(play, this, &gPlayerAnim_link_normal_pushing)) {
        this->unk_850 = 1;
    } else if (this->unk_850 == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 11.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_PUSH);
        }
    }
    
    Player_PlayAnimSfx(this, D_80854870);
    Player_SetPosYaw_PushPull(play, this);
    
    if (!Player_SetupPushPullWallIdle(play, this)) {
        Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.0f, play);
        temp = Player_GetPushPullDir(this, &sp34, &sp32);
        if (temp < 0) {
            Player_SetupPullWall(this, play);
        } else if (temp == 0) {
            Player_StartGrabPushPullWall(this, &gPlayerAnim_link_normal_push_end, play);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_4;
        }
    }
    
    if (this->stateFlags2 & PLAYER_STATE2_4) {
        func_8084B840(play, this, 2.0f);
        this->linearVelocity = 2.0f;
    }
}

static struct_80832924 D_80854878[] = {
    { NA_SE_PL_SLIP, 0x1004  },
    { NA_SE_PL_SLIP, -0x1018 },
};

static Vec3f D_80854880 = { 0.0f, 26.0f, -40.0f };

void Player_PullWall(Player* this, PlayState* play) {
    LinkAnimationHeader* anim;
    f32 sp70;
    s16 sp6E;
    s32 temp1;
    Vec3f sp5C;
    f32 temp2;
    CollisionPoly* sp54;
    s32 sp50;
    Vec3f sp44;
    Vec3f sp38;
    
    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_36, this->modelAnimType);
    this->stateFlags2 |= PLAYER_STATE2_0 | PLAYER_STATE2_6 | PLAYER_STATE2_8;
    
    if (Player_LoopAnimContinuously(play, this, anim)) {
        this->unk_850 = 1;
    } else {
        if (this->unk_850 == 0) {
            if (LinkAnimation_OnFrame(&this->skelAnime, 11.0f)) {
                Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_PUSH);
            }
        } else {
            Player_PlayAnimSfx(this, D_80854878);
        }
    }
    
    Player_SetPosYaw_PushPull(play, this);
    
    if (!Player_SetupPushPullWallIdle(play, this)) {
        Player_GetTargetVelAndYaw(this, &sp70, &sp6E, 0.0f, play);
        temp1 = Player_GetPushPullDir(this, &sp70, &sp6E);
        if (temp1 > 0) {
            Player_SetupPushWall(this, play);
        } else if (temp1 == 0) {
            Player_StartGrabPushPullWall(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_37, this->modelAnimType), play);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_4;
        }
    }
    
    if (this->stateFlags2 & PLAYER_STATE2_4) {
        temp2 = Player_RelativeFloorRaycast(play, this, &D_80854880, &sp5C) - this->actor.world.pos.y;
        if (fabsf(temp2) < 20.0f) {
            sp44.x = this->actor.world.pos.x;
            sp44.z = this->actor.world.pos.z;
            sp44.y = sp5C.y;
            if (!BgCheck_EntityLineTest1(&play->colCtx, &sp44, &sp5C, &sp38, &sp54, true, false, false, true, &sp50)) {
                func_8084B840(play, this, -2.0f);
                
                return;
            }
        }
        this->stateFlags2 &= ~PLAYER_STATE2_4;
    }
}

void Player_GrabLedge(Player* this, PlayState* play) {
    f32 sp3C;
    s16 sp3A;
    LinkAnimationHeader* anim;
    f32 temp;
    
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        // clang-format off
        anim = (this->unk_84F > 0) ? &gPlayerAnim_link_normal_fall_wait : GET_PLAYER_ANIM(PLAYER_ANIMGROUP_40, this->modelAnimType); Player_PlayAnimLoop(play, this, anim);
        // clang-format on
    } else if (this->unk_84F == 0) {
        if (this->skelAnime.animation == &gPlayerAnim_link_normal_fall) {
            temp = 11.0f;
        } else {
            temp = 1.0f;
        }
        
        if (LinkAnimation_OnFrame(&this->skelAnime, temp)) {
            Player_PlayMoveSfx(this, NA_SE_PL_WALK_GROUND);
            if (this->skelAnime.animation == &gPlayerAnim_link_normal_fall) {
                this->unk_84F = 1;
            } else {
                this->unk_84F = -1;
            }
        }
    }
    
    Math_ScaledStepToS(&this->actor.shape.rot.y, this->currentYaw, 0x800);
    
    if (this->unk_84F != 0) {
        Player_GetTargetVelAndYaw(this, &sp3C, &sp3A, 0.0f, play);
        if (this->unk_847[this->unk_846] >= 0) {
            if (this->unk_84F > 0) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_38, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_41, this->modelAnimType);
            }
            Player_SetupClimbOntoLedge(this, anim, play);
            
            return;
        }
        
        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) || (this->actor.shape.feetFloorFlag != 0)) {
            Player_SetLedgeGrabPosition(this);
            if (this->unk_84F < 0) {
                this->linearVelocity = -0.8f;
            } else {
                this->linearVelocity = 0.8f;
            }
            Player_SetupFallFromLedge(this, play);
            this->stateFlags1 &= ~(PLAYER_STATE1_13 | PLAYER_STATE1_14);
        }
    }
}

void Player_ClimbOntoLedge(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_UpdateAnimMovement(this, 1);
        Player_SetupStandingStillNoMorph(this, play);
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, this->skelAnime.endFrame - 6.0f)) {
        Player_PlayLandingSfx(this);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, this->skelAnime.endFrame - 34.0f)) {
        this->stateFlags1 &= ~(PLAYER_STATE1_13 | PLAYER_STATE1_14);
        func_8002F7DC(&this->actor, NA_SE_PL_CLIMB_CLIFF);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_CLIMB_END);
    }
}

void Player_PlayClimbingSfx(Player* this) {
    func_8002F7DC(&this->actor, (this->unk_84F != 0) ? NA_SE_PL_WALK_WALL : NA_SE_PL_WALK_LADDER);
}

void Player_ClimbingWallOrDownLedge(Player* this, PlayState* play) {
    static Vec3f D_8085488C = { 0.0f, 0.0f, 26.0f };
    s32 sp84;
    s32 sp80;
    f32 phi_f0;
    f32 phi_f2;
    Vec3f sp6C;
    s32 sp68;
    Vec3f sp5C;
    f32 temp_f0;
    LinkAnimationHeader* anim1;
    LinkAnimationHeader* anim2;
    
    sp84 = sControlInput->rel.stick_y;
    sp80 = sControlInput->rel.stick_x;
    
    this->fallStartHeight = this->actor.world.pos.y;
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    if ((this->unk_84F != 0) && (ABS(sp84) < ABS(sp80))) {
        phi_f0 = ABS(sp80) * 0.0325f;
        sp84 = 0;
    } else {
        phi_f0 = ABS(sp84) * 0.05f;
        sp80 = 0;
    }
    
    if (phi_f0 < 1.0f) {
        phi_f0 = 1.0f;
    } else if (phi_f0 > 3.35f) {
        phi_f0 = 3.35f;
    }
    
    if (this->skelAnime.playSpeed >= 0.0f) {
        phi_f2 = 1.0f;
    } else {
        phi_f2 = -1.0f;
    }
    
    this->skelAnime.playSpeed = phi_f2 * phi_f0;
    
    if (this->unk_850 >= 0) {
        if ((this->actor.wallPoly != NULL) && (this->actor.wallBgId != BGCHECK_SCENE)) {
            DynaPolyActor* wallPolyActor = DynaPoly_GetActor(&play->colCtx, this->actor.wallBgId);
            if (wallPolyActor != NULL) {
                Math_Vec3f_Diff(&wallPolyActor->actor.world.pos, &wallPolyActor->actor.prevPos, &sp6C);
                Math_Vec3f_Sum(&this->actor.world.pos, &sp6C, &this->actor.world.pos);
            }
        }
        
        Actor_UpdateBgCheckInfo(
            play,
            &this->actor,
            26.0f,
            6.0f,
            this->ageProperties->unk_00,
            UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2
        );
        Player_SetPosYaw_WallClimb(play, this, 26.0f, this->ageProperties->unk_3C, 50.0f, -20.0f);
    }
    
    if ((this->unk_850 < 0) || !Player_SetupClimbingLetGo(this, play)) {
        if (LinkAnimation_Update(play, &this->skelAnime) != 0) {
            if (this->unk_850 < 0) {
                this->unk_850 = ABS(this->unk_850) & 1;
                
                return;
            }
            
            if (sp84 != 0) {
                sp68 = this->unk_84F + this->unk_850;
                
                if (sp84 > 0) {
                    D_8085488C.y = this->ageProperties->unk_40;
                    temp_f0 = Player_RelativeFloorRaycast(play, this, &D_8085488C, &sp5C);
                    
                    if (this->actor.world.pos.y < temp_f0) {
                        if (this->unk_84F != 0) {
                            this->actor.world.pos.y = temp_f0;
                            this->stateFlags1 &= ~PLAYER_STATE1_21;
                            Player_SetupGrabLedge(
                                play,
                                this,
                                this->actor.wallPoly,
                                this->ageProperties->unk_3C,
                                &gPlayerAnim_link_normal_jump_climb_up_free
                            );
                            this->currentYaw += 0x8000;
                            this->actor.shape.rot.y = this->currentYaw;
                            Player_SetupClimbOntoLedge(this, &gPlayerAnim_link_normal_jump_climb_up_free, play);
                            this->stateFlags1 |= PLAYER_STATE1_14;
                        } else {
                            Player_SetupEndClimb(this, this->ageProperties->unk_CC[this->unk_850], play);
                        }
                    } else {
                        this->skelAnime.prevTransl = this->ageProperties->unk_4A[sp68];
                        Player_PlayAnimOnce(play, this, this->ageProperties->unk_AC[sp68]);
                    }
                } else {
                    if ((this->actor.world.pos.y - this->actor.floorHeight) < 15.0f) {
                        if (this->unk_84F != 0) {
                            Player_ClimbingLetGo(this, play);
                        } else {
                            if (this->unk_850 != 0) {
                                this->skelAnime.prevTransl = this->ageProperties->unk_44;
                            }
                            Player_SetupEndClimb(this, this->ageProperties->unk_C4[this->unk_850], play);
                            this->unk_850 = 1;
                        }
                    } else {
                        sp68 ^= 1;
                        this->skelAnime.prevTransl = this->ageProperties->unk_62[sp68];
                        anim1 = this->ageProperties->unk_AC[sp68];
                        LinkAnimation_Change(
                            play,
                            &this->skelAnime,
                            anim1,
                            -1.0f,
                            Animation_GetLastFrame(anim1),
                            0.0f,
                            ANIMMODE_ONCE,
                            0.0f
                        );
                    }
                }
                this->unk_850 ^= 1;
            } else {
                if ((this->unk_84F != 0) && (sp80 != 0)) {
                    anim2 = this->ageProperties->unk_BC[this->unk_850];
                    
                    if (sp80 > 0) {
                        this->skelAnime.prevTransl = this->ageProperties->unk_7A[this->unk_850];
                        Player_PlayAnimOnce(play, this, anim2);
                    } else {
                        this->skelAnime.prevTransl = this->ageProperties->unk_86[this->unk_850];
                        LinkAnimation_Change(
                            play,
                            &this->skelAnime,
                            anim2,
                            -1.0f,
                            Animation_GetLastFrame(anim2),
                            0.0f,
                            ANIMMODE_ONCE,
                            0.0f
                        );
                    }
                } else {
                    this->stateFlags2 |= PLAYER_STATE2_12;
                }
            }
            
            return;
        }
    }
    
    if (this->unk_850 < 0) {
        if (
            ((this->unk_850 == -2) &&
            (LinkAnimation_OnFrame(&this->skelAnime, 14.0f) || LinkAnimation_OnFrame(&this->skelAnime, 29.0f))) ||
            ((this->unk_850 == -4) &&
            (LinkAnimation_OnFrame(&this->skelAnime, 22.0f) || LinkAnimation_OnFrame(&this->skelAnime, 35.0f) ||
            LinkAnimation_OnFrame(&this->skelAnime, 49.0f) || LinkAnimation_OnFrame(&this->skelAnime, 55.0f)))
        ) {
            Player_PlayClimbingSfx(this);
        }
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, (this->skelAnime.playSpeed > 0.0f) ? 20.0f : 0.0f)) {
        Player_PlayClimbingSfx(this);
    }
}

static f32 sClimbEndFrames[] = { 10.0f, 20.0f };
static f32 sClimbLadderEndFrames[] = { 40.0f, 50.0f };

void Player_EndClimb(Player* this, PlayState* play) {
    static struct_80832924 sClimbLadderEndAnimSfx[] = {
        { NA_SE_PL_WALK_LADDER, 0x80A  },
        { NA_SE_PL_WALK_LADDER, 0x814  },
        { NA_SE_PL_WALK_LADDER, -0x81E },
    };
    s32 temp;
    f32* sp38;
    CollisionPoly* sp34;
    s32 sp30;
    Vec3f sp24;
    
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    temp = Player_IsActionInterrupted(play, this, &this->skelAnime, 4.0f);
    
    if (temp == 0) {
        this->stateFlags1 &= ~PLAYER_STATE1_21;
        
        return;
    }
    
    if ((temp > 0) || LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_21;
        
        return;
    }
    
    sp38 = sClimbEndFrames;
    
    if (this->unk_850 != 0) {
        Player_PlayAnimSfx(this, sClimbLadderEndAnimSfx);
        sp38 = sClimbLadderEndFrames;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, sp38[0]) || LinkAnimation_OnFrame(&this->skelAnime, sp38[1])) {
        sp24.x = this->actor.world.pos.x;
        sp24.y = this->actor.world.pos.y + 20.0f;
        sp24.z = this->actor.world.pos.z;
        if (BgCheck_EntityRaycastDown3(&play->colCtx, &sp34, &sp30, &sp24) != 0.0f) {
            this->unk_89E = SurfaceType_GetSfxType(&play->colCtx, sp34, sp30);
            Player_PlayLandingSfx(this);
        }
    }
}

void Player_InsideCrawlspace(Player* this, PlayState* play) {
    static struct_80832924 sCrawlspaceCrawlAnimSfx[] = {
        { 0, 0x3028 }, { 0, 0x3030 }, { 0, 0x3038 }, { 0, 0x3040  },  { 0, 0x3048 },
        { 0, 0x3050 }, { 0, 0x3058 }, { 0, 0x3060 }, { 0, -0x3068 },
    };
    
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_0)) {
            if (this->skelAnime.moveFlags != 0) {
                this->skelAnime.moveFlags = 0;
                
                return;
            }
            
            if (!Player_SetupExitCrawlspace(this, play)) {
                this->linearVelocity = sControlInput->rel.stick_y * 0.03f;
            }
        }
        
        return;
    }
    
    Player_PlayAnimSfx(this, sCrawlspaceCrawlAnimSfx);
}

void Player_ExitCrawlspace(Player* this, PlayState* play) {
    static struct_80832924 sExitCrawlspaceAnimSfx[] = {
        { 0, 0x300A }, { 0, 0x3012 }, { 0, 0x301A }, { 0, 0x3022  },  { 0, 0x3034 },
        { 0, 0x303C }, { 0, 0x3044 }, { 0, 0x304C }, { 0, -0x3054 },
    };
    
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_18;
        
        return;
    }
    
    Player_PlayAnimSfx(this, sExitCrawlspaceAnimSfx);
}

s32 Player_CanDismountHorse(PlayState* play, Player* this, s32 arg2, f32* arg3) {
    static Vec3f sHorseDismountRaycastOffset[] = {
        { 40.0f,  0.0f,  0.0f },
        { -40.0f, 0.0f,  0.0f },
    };
    static Vec3f sHorseLineTestTopOffset[] = {
        { 60.0f,  20.0f,  0.0f },
        { -60.0f, 20.0f,  0.0f },
    };
    static Vec3f sHorseLineTestBottomOffset[] = {
        { 60.0f,  -20.0f,  0.0f },
        { -60.0f, -20.0f,  0.0f },
    };
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    f32 sp50;
    f32 sp4C;
    Vec3f sp40;
    Vec3f sp34;
    CollisionPoly* sp30;
    s32 sp2C;
    
    sp50 = rideActor->actor.world.pos.y + 20.0f;
    sp4C = rideActor->actor.world.pos.y - 20.0f;
    
    *arg3 = Player_RelativeFloorRaycast(play, this, &sHorseDismountRaycastOffset[arg2], &sp40);
    
    return (sp4C < *arg3) && (*arg3 < sp50) && !Player_RelativeLineRaycast(play, this, &sHorseLineTestTopOffset[arg2], &sp30, &sp2C, &sp34) &&
           !Player_RelativeLineRaycast(play, this, &sHorseLineTestBottomOffset[arg2], &sp30, &sp2C, &sp34);
}

s32 Player_SetupDismountHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    s32 sp38;
    f32 sp34;
    
    if (this->unk_850 < 0) {
        this->unk_850 = 99;
    } else {
        sp38 = (this->mountSide < 0) ? 0 : 1;
        if (!Player_CanDismountHorse(play, this, sp38, &sp34)) {
            sp38 ^= 1;
            if (!Player_CanDismountHorse(play, this, sp38, &sp34)) {
                return 0;
            } else {
                this->mountSide = -this->mountSide;
            }
        }
        
        if (
            (play->csCtx.state == CS_STATE_IDLE) && (play->transitionMode == TRANS_MODE_OFF) &&
            (EN_HORSE_CHECK_1(rideActor) || EN_HORSE_CHECK_4(rideActor))
        ) {
            this->stateFlags2 |= PLAYER_STATE2_22;
            
            if (
                EN_HORSE_CHECK_1(rideActor) ||
                (EN_HORSE_CHECK_4(rideActor) && CHECK_BTN_ALL(sControlInput->press.button, BTN_A))
            ) {
                rideActor->actor.child = NULL;
                Player_SetActionFunc_KeepMoveFlags(play, this, Player_DismountHorse, 0);
                this->unk_878 = sp34 - rideActor->actor.world.pos.y;
                Player_PlayAnimOnce(play, this, (this->mountSide < 0) ? &gPlayerAnim_link_uma_left_down : &gPlayerAnim_link_uma_right_down);
                
                return 1;
            }
        }
    }
    
    return 0;
}

void func_8084CBF4(Player* this, f32 arg1, f32 arg2) {
    f32 temp;
    f32 dir;
    
    if ((this->unk_878 != 0.0f) && (arg2 <= this->skelAnime.curFrame)) {
        if (arg1 < fabsf(this->unk_878)) {
            if (this->unk_878 >= 0.0f) {
                dir = 1;
            } else {
                dir = -1;
            }
            temp = dir * arg1;
        } else {
            temp = this->unk_878;
        }
        this->actor.world.pos.y += temp;
        this->unk_878 -= temp;
    }
}

static LinkAnimationHeader* sAnims_HorseMove[] = {
    &gPlayerAnim_link_uma_anim_stop,
    &gPlayerAnim_link_uma_anim_stand,
    &gPlayerAnim_link_uma_anim_walk,
    &gPlayerAnim_link_uma_anim_slowrun,
    &gPlayerAnim_link_uma_anim_fastrun,
    &gPlayerAnim_link_uma_anim_jump100,
    &gPlayerAnim_link_uma_anim_jump200,
    NULL,
    NULL,
};

static LinkAnimationHeader* sAnims_HorseWhip[] = {
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_walk_muti,
    &gPlayerAnim_link_uma_anim_slowrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    &gPlayerAnim_link_uma_anim_fastrun_muti,
    NULL,
    NULL,
};

static LinkAnimationHeader* sAnims_HorseIdle[] = {
    &gPlayerAnim_link_uma_wait_3,
    &gPlayerAnim_link_uma_wait_1,
    &gPlayerAnim_link_uma_wait_2,
};

static struct_80832924 sAnimSfx_HorseIdle[] = {
    { NA_SE_PL_CALM_HIT, 0x830 }, { NA_SE_PL_CALM_HIT, 0x83A  },  { NA_SE_PL_CALM_HIT, 0x844 },
    { NA_SE_PL_CALM_PAT, 0x85C }, { NA_SE_PL_CALM_PAT, 0x86E  },  { NA_SE_PL_CALM_PAT, 0x87E },
    { NA_SE_PL_CALM_PAT, 0x884 }, { NA_SE_PL_CALM_PAT, -0x888 },
};

static struct_80832924 sAnimSfx_HorseDismount[] = {
    { 0,                      0x2800                      },
    { NA_SE_PL_GET_OFF_HORSE, 0x80A                       },
    { NA_SE_PL_SLIPDOWN,      -0x819                      },
};

static u8 sMountSfxFrames[2][2] = {
    { 32, 58 },
    { 25, 42 },
};

static Vec3s sRideHorsePrevTransl = { -69, 7146, -266 };

void Player_RideHorse(Player* this, PlayState* play) {
    EnHorse* rideActor = (EnHorse*)this->rideActor;
    u8* arr;
    
    this->stateFlags2 |= PLAYER_STATE2_6;
    
    func_8084CBF4(this, 1.0f, 10.0f);
    
    if (this->unk_850 == 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->skelAnime.animation = &gPlayerAnim_link_uma_wait_1;
            this->unk_850 = 99;
            
            return;
        }
        
        arr = sMountSfxFrames[(this->mountSide < 0) ? 0 : 1];
        
        if (LinkAnimation_OnFrame(&this->skelAnime, arr[0])) {
            func_8002F7DC(&this->actor, NA_SE_PL_CLIMB_CLIFF);
            
            return;
        }
        
        if (LinkAnimation_OnFrame(&this->skelAnime, arr[1])) {
            func_8002DE74(play, this);
            func_8002F7DC(&this->actor, NA_SE_PL_SIT_ON_HORSE);
            
            return;
        }
        
        return;
    }
    
    func_8002DE74(play, this);
    this->skelAnime.prevTransl = sRideHorsePrevTransl;
    
    if ((rideActor->animationIdx != this->unk_850) && ((rideActor->animationIdx >= 2) || (this->unk_850 >= 2))) {
        if ((this->unk_850 = rideActor->animationIdx) < 2) {
            f32 rand = Rand_ZeroOne();
            s32 temp = 0;
            
            this->unk_850 = 1;
            
            if (rand < 0.1f) {
                temp = 2;
            } else if (rand < 0.2f) {
                temp = 1;
            }
            Player_PlayAnimOnce(play, this, sAnims_HorseIdle[temp]);
        } else {
            this->skelAnime.animation = sAnims_HorseMove[this->unk_850 - 2];
            Animation_SetMorph(play, &this->skelAnime, 8.0f);
            if (this->unk_850 < 4) {
                Player_SetupHeldItemUpperActionFunc(play, this);
                this->unk_84F = 0;
            }
        }
    }
    
    if (this->unk_850 == 1) {
        if ((D_808535E0 != 0) || Player_CheckActorTalkRequested(play)) {
            Player_PlayAnimOnce(play, this, &gPlayerAnim_link_uma_wait_3);
        } else if (LinkAnimation_Update(play, &this->skelAnime)) {
            this->unk_850 = 99;
        } else if (this->skelAnime.animation == &gPlayerAnim_link_uma_wait_1) {
            Player_PlayAnimSfx(this, sAnimSfx_HorseIdle);
        }
    } else {
        this->skelAnime.curFrame = rideActor->curFrame;
        LinkAnimation_AnimateFrame(play, &this->skelAnime);
    }
    
    AnimationContext_SetCopyAll(
        play,
        this->skelAnime.limbCount,
        this->skelAnime.morphTable,
        this->skelAnime.jointTable
    );
    
    if ((play->csCtx.state != CS_STATE_IDLE) || (this->csMode != 0)) {
        if (this->csMode == 7) {
            this->csMode = 0;
        }
        this->unk_6AD = 0;
        this->unk_84F = 0;
    } else if ((this->unk_850 < 2) || (this->unk_850 >= 4)) {
        D_808535E0 = Player_SetupCurrentUpperAction(this, play);
        if (D_808535E0 != 0) {
            this->unk_84F = 0;
        }
    }
    
    this->actor.world.pos.x = rideActor->actor.world.pos.x + rideActor->riderPos.x;
    this->actor.world.pos.y = (rideActor->actor.world.pos.y + rideActor->riderPos.y) - 27.0f;
    this->actor.world.pos.z = rideActor->actor.world.pos.z + rideActor->riderPos.z;
    
    this->currentYaw = this->actor.shape.rot.y = rideActor->actor.shape.rot.y;
    
    if (
        (this->csMode != 0) ||
        (!Player_CheckActorTalkRequested(play) && ((rideActor->actor.speedXZ != 0.0f) || !Player_SetupSpeakOrCheck(this, play)) &&
        !Player_SetupRollOrPutAway(this, play))
    ) {
        if (D_808535E0 == 0) {
            if (this->unk_84F != 0) {
                if (LinkAnimation_Update(play, &this->skelAnime2)) {
                    rideActor->stateFlags &= ~ENHORSE_FLAG_8;
                    this->unk_84F = 0;
                }
                
                if (this->skelAnime2.animation == &gPlayerAnim_link_uma_stop_muti) {
                    if (LinkAnimation_OnFrame(&this->skelAnime2, 23.0f)) {
                        func_8002F7DC(&this->actor, NA_SE_IT_LASH);
                        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
                    }
                    
                    AnimationContext_SetCopyAll(
                        play,
                        this->skelAnime.limbCount,
                        this->skelAnime.jointTable,
                        this->skelAnime2.jointTable
                    );
                } else {
                    if (LinkAnimation_OnFrame(&this->skelAnime2, 10.0f)) {
                        func_8002F7DC(&this->actor, NA_SE_IT_LASH);
                        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_LASH);
                    }
                    
                    AnimationContext_SetCopyTrue(
                        play,
                        this->skelAnime.limbCount,
                        this->skelAnime.jointTable,
                        this->skelAnime2.jointTable,
                        D_80853410
                    );
                }
            } else {
                LinkAnimationHeader* anim = NULL;
                
                if (EN_HORSE_CHECK_3(rideActor)) {
                    anim = &gPlayerAnim_link_uma_stop_muti;
                } else if (EN_HORSE_CHECK_2(rideActor)) {
                    if ((this->unk_850 >= 2) && (this->unk_850 != 99)) {
                        anim = sAnims_HorseWhip[this->unk_850 - 2];
                    }
                }
                
                if (anim != NULL) {
                    LinkAnimation_PlayOnce(play, &this->skelAnime2, anim);
                    this->unk_84F = 1;
                }
            }
        }
        
        if (this->stateFlags1 & PLAYER_STATE1_20) {
            if (
                !Player_SetupCameraMode(play, this) || CHECK_BTN_ANY(sControlInput->press.button, BTN_A) ||
                Player_IsZTargeting(this)
            ) {
                this->unk_6AD = 0;
                this->stateFlags1 &= ~PLAYER_STATE1_20;
            } else {
                this->unk_6BE = Player_SetFirstPersonAimLookAngles(play, this, 1, -5000) - this->actor.shape.rot.y;
                this->unk_6BE += 5000;
                this->unk_6B0 = -5000;
            }
            
            return;
        }
        
        if ((this->csMode != 0) || (!Player_SetupDismountHorse(this, play) && !Player_SetupItemCsOrFirstPerson(this, play))) {
            if (this->unk_664 != NULL) {
                if (func_8002DD78(this) != 0) {
                    this->unk_6BE = Player_LookAtTargetActor(this, 1) - this->actor.shape.rot.y;
                    this->unk_6BE = CLAMP(this->unk_6BE, -0x4AAA, 0x4AAA);
                    this->actor.focus.rot.y = this->actor.shape.rot.y + this->unk_6BE;
                    this->unk_6BE += 5000;
                    this->unk_6AE |= 0x80;
                } else {
                    Player_LookAtTargetActor(this, 0);
                }
            } else {
                if (func_8002DD78(this) != 0) {
                    this->unk_6BE = Player_SetFirstPersonAimLookAngles(play, this, 1, -5000) - this->actor.shape.rot.y;
                    this->unk_6BE += 5000;
                    this->unk_6B0 = -5000;
                }
            }
        }
    }
}

void Player_DismountHorse(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_6;
    func_8084CBF4(this, 1.0f, 10.0f);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        EnHorse* rideActor = (EnHorse*)this->rideActor;
        
        Player_SetupStandingStillNoMorph(this, play);
        this->stateFlags1 &= ~PLAYER_STATE1_23;
        this->actor.parent = NULL;
        AREG(6) = 0;
        
        if (Flags_GetEventChkInf(EVENTCHKINF_18) || (DREG(1) != 0)) {
            gSaveContext.horseData.pos.x = rideActor->actor.world.pos.x;
            gSaveContext.horseData.pos.y = rideActor->actor.world.pos.y;
            gSaveContext.horseData.pos.z = rideActor->actor.world.pos.z;
            gSaveContext.horseData.angle = rideActor->actor.shape.rot.y;
        }
    } else {
        Camera_ChangeSetting(Play_GetCamera(play, CAM_ID_MAIN), CAM_SET_NORMAL0);
        
        if (this->mountSide < 0) {
            sAnimSfx_HorseDismount[0].field = 0x2828;
        } else {
            sAnimSfx_HorseDismount[0].field = 0x281D;
        }
        Player_PlayAnimSfx(this, sAnimSfx_HorseDismount);
    }
}

static struct_80832924 sAnimSfx_Swim[] = {
    { NA_SE_PL_SWIM, -0x800 },
};

void Player_SetupSwimMovement(Player* this, f32* arg1, f32 arg2, s16 arg3) {
    Player_UpdateSwimMovement(this, arg1, arg2, arg3);
    Player_PlayAnimSfx(this, sAnimSfx_Swim);
}

void Player_SetupSwim(PlayState* play, Player* this, s16 arg2) {
    Player_SetActionFunc(play, this, Player_Swim, 0);
    this->actor.shape.rot.y = this->currentYaw = arg2;
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_SetupZTargetSwimming(PlayState* play, Player* this) {
    Player_SetActionFunc(play, this, Player_ZTargetSwimming, 0);
    Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_UpdateSwimIdle(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    
    Player_LoopAnimContinuously(play, this, &gPlayerAnim_link_swimer_swim_wait);
    Player_SetVerticalWaterVelocity(this);
    
    if (
        !Player_CheckActorTalkRequested(play) && !Player_SetupAction(play, this, sAction_Swim, 1) &&
        !Player_SetupDive(play, this, sControlInput)
    ) {
        if (this->unk_6AD != 1) {
            this->unk_6AD = 0;
        }
        
        if (this->currentBoots == PLAYER_BOOTS_IRON) {
            sp34 = 0.0f;
            sp32 = this->actor.shape.rot.y;
            
            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                Player_SetupReturnToStandStillSetAnim(this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_15, this->modelAnimType), play);
                Player_PlayLandingSfx(this);
            }
        } else {
            Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.0f, play);
            
            if (sp34 != 0.0f) {
                s16 temp = this->actor.shape.rot.y - sp32;
                
                if ((ABS(temp) > 0x6000) && !Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
                    return;
                }
                
                if (Player_IsZTargetingSetupStartEnemy(this)) {
                    Player_SetupZTargetSwimming(play, this);
                } else {
                    Player_SetupSwim(play, this, sp32);
                }
            }
        }
        
        Player_UpdateSwimMovement(this, &this->linearVelocity, sp34, sp32);
    }
}

void Player_SpawnSwimming(Player* this, PlayState* play) {
    if (!Player_SetupItemCsOrFirstPerson(this, play)) {
        this->stateFlags2 |= PLAYER_STATE2_5;
        
        Player_PlaySwimAnim(play, this, NULL, this->linearVelocity);
        Player_SetVerticalWaterVelocity(this);
        
        if (DECR(this->unk_850) == 0) {
            Player_SetupSwimIdle(play, this);
        }
    }
}

void Player_Swim(Player* this, PlayState* play) {
    f32 sp34;
    s16 sp32;
    s16 temp;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    Player_PlaySwimAnim(play, this, sControlInput, this->linearVelocity);
    Player_SetVerticalWaterVelocity(this);
    
    if (!Player_SetupAction(play, this, sAction_Swim, 1) && !Player_SetupDive(play, this, sControlInput)) {
        Player_GetTargetVelAndYaw(this, &sp34, &sp32, 0.0f, play);
        
        temp = this->actor.shape.rot.y - sp32;
        if ((sp34 == 0.0f) || (ABS(temp) > 0x6000) || (this->currentBoots == PLAYER_BOOTS_IRON)) {
            Player_SetupSwimIdle(play, this);
        } else if (Player_IsZTargetingSetupStartEnemy(this)) {
            Player_SetupZTargetSwimming(play, this);
        }
        
        Player_SetupSwimMovement(this, &this->linearVelocity, sp34, sp32);
    }
}

s32 Player_SetupZTargetSwimAnims(PlayState* play, Player* this, f32* arg2, s16* arg3) {
    LinkAnimationHeader* anim;
    s16 temp1;
    s32 temp2;
    
    temp1 = this->currentYaw - *arg3;
    
    if (ABS(temp1) > 0x6000) {
        anim = &gPlayerAnim_link_swimer_swim_wait;
        
        if (Math_StepToF(&this->linearVelocity, 0.0f, 1.0f)) {
            this->currentYaw = *arg3;
        } else {
            *arg2 = 0.0f;
            *arg3 = this->currentYaw;
        }
    } else {
        temp2 = Player_GetFriendlyZTargetingMoveDirection(this, arg2, arg3, play);
        
        if (temp2 > 0) {
            anim = &gPlayerAnim_link_swimer_swim;
        } else if (temp2 < 0) {
            anim = &gPlayerAnim_link_swimer_back_swim;
        } else if ((temp1 = this->actor.shape.rot.y - *arg3) > 0) {
            anim = &gPlayerAnim_link_swimer_Rside_swim;
        } else {
            anim = &gPlayerAnim_link_swimer_Lside_swim;
        }
    }
    
    if (anim != this->skelAnime.animation) {
        Player_ChangeAnimLongMorphLoop(play, this, anim);
        
        return 1;
    }
    
    return 0;
}

void Player_ZTargetSwimming(Player* this, PlayState* play) {
    f32 sp2C;
    s16 sp2A;
    
    Player_PlaySwimAnim(play, this, sControlInput, this->linearVelocity);
    Player_SetVerticalWaterVelocity(this);
    
    if (!Player_SetupAction(play, this, sAction_Swim, 1) && !Player_SetupDive(play, this, sControlInput)) {
        Player_GetTargetVelAndYaw(this, &sp2C, &sp2A, 0.0f, play);
        
        if (sp2C == 0.0f) {
            Player_SetupSwimIdle(play, this);
        } else if (!Player_IsZTargetingSetupStartEnemy(this)) {
            Player_SetupSwim(play, this, sp2A);
        } else {
            Player_SetupZTargetSwimAnims(play, this, &sp2C, &sp2A);
        }
        
        Player_SetupSwimMovement(this, &this->linearVelocity, sp2C, sp2A);
    }
}

void Player_UpdateDiveMovement(PlayState* play, Player* this, f32 arg2) {
    f32 sp2C;
    s16 sp2A;
    
    Player_GetTargetVelAndYaw(this, &sp2C, &sp2A, 0.0f, play);
    Player_UpdateSwimMovement(this, &this->linearVelocity, sp2C * 0.5f, sp2A);
    Player_UpdateSwimMovement(this, &this->actor.velocity.y, arg2, this->currentYaw);
}

void Player_Dive(Player* this, PlayState* play) {
    f32 sp2C;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    this->actor.gravity = 0.0f;
    Player_SetupCurrentUpperAction(this, play);
    
    if (!Player_SetupItemCsOrFirstPerson(this, play)) {
        if (this->currentBoots == PLAYER_BOOTS_IRON) {
            Player_SetupSwimIdle(play, this);
            
            return;
        }
        
        if (this->unk_84F == 0) {
            if (this->unk_850 == 0) {
                if (
                    LinkAnimation_Update(play, &this->skelAnime) ||
                    ((this->skelAnime.curFrame >= 22.0f) && !CHECK_BTN_ALL(sControlInput->cur.button, BTN_A))
                ) {
                    Player_RiseFromDive(play, this);
                } else if (LinkAnimation_OnFrame(&this->skelAnime, 20.0f) != 0) {
                    this->actor.velocity.y = -2.0f;
                }
                
                Player_StepLinearVelToZero(this);
                
                return;
            }
            
            Player_PlaySwimAnim(play, this, sControlInput, this->actor.velocity.y);
            this->unk_6C2 = 16000;
            
            if (
                CHECK_BTN_ALL(sControlInput->cur.button, BTN_A) && !Player_SetupGetItemOrHoldBehavior(this, play) &&
                !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) &&
                (this->actor.yDistToWater < sScaleDiveDists[CUR_UPG_VALUE(UPG_SCALE)])
            ) {
                Player_UpdateDiveMovement(play, this, -2.0f);
            } else {
                this->unk_84F++;
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
            }
        } else if (this->unk_84F == 1) {
            LinkAnimation_Update(play, &this->skelAnime);
            Player_SetVerticalWaterVelocity(this);
            
            if (this->unk_6C2 < 10000) {
                this->unk_84F++;
                this->unk_850 = this->actor.yDistToWater;
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim);
            }
        } else if (!Player_SetupDive(play, this, sControlInput)) {
            sp2C = (this->unk_850 * 0.018f) + 4.0f;
            
            if (this->stateFlags1 & PLAYER_STATE1_11) {
                sControlInput = NULL;
            }
            
            Player_PlaySwimAnim(play, this, sControlInput, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->unk_6C2, -10000, 800);
            
            if (sp2C > 8.0f) {
                sp2C = 8.0f;
            }
            
            Player_UpdateDiveMovement(play, this, sp2C);
        }
    }
}

void Player_EndGetItem(PlayState* play, Player* this) {
    this->unk_862 = 0;
    this->stateFlags1 &= ~(PLAYER_STATE1_10 | PLAYER_STATE1_11);
    this->getItemId = GI_NONE;
    func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
}

void Player_SetupEndGetItem(PlayState* play, Player* this) {
    Player_EndGetItem(play, this);
    Player_AddRootYawToShapeYaw(this);
    Player_SetupStandingStillNoMorph(this, play);
    this->currentYaw = this->actor.shape.rot.y;
}

s32 Player_SetupGetItemText(PlayState* play, Player* this) {
    GetItemEntry* giEntry;
    s32 temp1;
    s32 temp2;
    
    if (this->getItemId == GI_NONE) {
        return 1;
    }
    
    if (this->unk_84F == 0) {
        giEntry = &sGetItemTable[this->getItemId - 1];
        this->unk_84F = 1;
        
        Message_StartTextbox(play, giEntry->textId, &this->actor);
        Item_Give(play, giEntry->itemId);
        
        if (
            ((this->getItemId >= GI_RUPEE_GREEN) && (this->getItemId <= GI_RUPEE_RED)) ||
            ((this->getItemId >= GI_RUPEE_PURPLE) && (this->getItemId <= GI_RUPEE_GOLD)) ||
            ((this->getItemId >= GI_RUPEE_GREEN_LOSE) && (this->getItemId <= GI_RUPEE_PURPLE_LOSE)) ||
            (this->getItemId == GI_RECOVERY_HEART)
        ) {
            Audio_PlaySfxGeneral(
                NA_SE_SY_GET_BOXITEM,
                &gSfxDefaultPos,
                4,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultFreqAndVolScale,
                &gSfxDefaultReverb
            );
        } else {
            if (
                (this->getItemId == GI_HEART_CONTAINER_2) || (this->getItemId == GI_HEART_CONTAINER) ||
                ((this->getItemId == GI_HEART_PIECE) &&
                ((gSaveContext.inventory.questItems & 0xF0000000) == (4 << QUEST_HEART_PIECE_COUNT)))
            ) {
                temp1 = NA_BGM_HEART_GET | 0x900;
            } else {
                temp1 = temp2 = (this->getItemId == GI_HEART_PIECE) ? NA_BGM_SMALL_ITEM_GET : NA_BGM_ITEM_GET | 0x900;
            }
            Audio_PlayFanfare(temp1);
        }
    } else {
        if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
#if Patch_SilverGauntletObtainCS == true
            if (this->getItemId == GI_GAUNTLETS_SILVER) {
                gExitParam.nextEntranceIndex = ENTR_SPOT11_0;
                play->transitionTrigger = TRANS_TRIGGER_START;
                gSaveContext.nextCutsceneIndex = 0xFFF1;
                play->transitionType = TRANS_TYPE_SANDSTORM_END;
                this->stateFlags1 &= ~PLAYER_STATE1_29;
                Player_SetupPlayerCutscene(play, NULL, 8);
            }
#endif
            this->getItemId = GI_NONE;
        }
    }
    
    return 0;
}

void Player_GetItemInWater(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (!(this->stateFlags1 & PLAYER_STATE1_10) || Player_SetupGetItemText(play, this)) {
            Player_EndGetItem(play, this);
            Player_SetupSwimIdle(play, this);
            Player_ResetSubCam(play, this);
        }
    } else {
        if ((this->stateFlags1 & PLAYER_STATE1_10) && LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
            Player_SetGetItemID(this, play);
            Player_ResetSubCam(play, this);
            Player_SetCameraTurnAround(play, 8);
        } else if (LinkAnimation_OnFrame(&this->skelAnime, 5.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_BREATH_DRINK);
        }
    }
    
    Player_SetVerticalWaterVelocity(this);
    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_DamagedSwim(Player* this, PlayState* play) {
    Player_SetVerticalWaterVelocity(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupSwimIdle(play, this);
    }
    
    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_Drown(Player* this, PlayState* play) {
    Player_SetVerticalWaterVelocity(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndDie(play, this);
    }
    
    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

static s16 sWarpSongEntrances[] = {
    ENTR_SPOT05_2, ENTR_SPOT17_4, ENTR_SPOT06_8, ENTR_SPOT11_5, ENTR_SPOT02_7, ENTR_TOKINOMA_7,
};

void Player_PlayOcarina(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_normal_okarina_swing);
        this->unk_850 = 1;
        if (this->stateFlags2 & (PLAYER_STATE2_23 | PLAYER_STATE2_25)) {
            this->stateFlags2 |= PLAYER_STATE2_24;
        } else {
            func_8010BD58(play, OCARINA_ACTION_FREE_PLAY);
        }
        
        return;
    }
    
    if (this->unk_850 == 0) {
        return;
    }
    
    if (play->msgCtx.ocarinaMode == OCARINA_MODE_04) {
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
        
        if ((this->targetActor != NULL) && (this->targetActor == this->unk_6A8)) {
            Player_StartTalkingWithActor(play, this->targetActor);
        } else if (this->naviTextId < 0) {
            this->targetActor = this->naviActor;
            this->naviActor->textId = -this->naviTextId;
            Player_StartTalkingWithActor(play, this->targetActor);
        } else if (!Player_SetupItemCsOrFirstPerson(this, play)) {
            Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_okarina_end, play);
        }
        
        this->stateFlags2 &= ~(PLAYER_STATE2_23 | PLAYER_STATE2_24 | PLAYER_STATE2_25);
        this->unk_6A8 = NULL;
    } else if (play->msgCtx.ocarinaMode == OCARINA_MODE_02) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex = sWarpSongEntrances[play->msgCtx.lastPlayedSong];
        gSaveContext.respawn[RESPAWN_MODE_RETURN].playerParams = 0x5FF;
        gSaveContext.respawn[RESPAWN_MODE_RETURN].data = play->msgCtx.lastPlayedSong;
        
        this->csMode = 0;
        this->stateFlags1 &= ~PLAYER_STATE1_29;
        
        Player_SetupPlayerCutscene(play, NULL, 8);
        play->mainCamera.unk_14C &= ~8;
        
        this->stateFlags1 |= PLAYER_STATE1_28 | PLAYER_STATE1_29;
        this->stateFlags2 |= PLAYER_STATE2_27;
        
        if (Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0xF) == NULL) {
            Environment_WarpSongLeave(play);
        }
        
        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
    }
}

void Player_ThrowDekuNut(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_normal_light_bom_end, play);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, 3.0f)) {
        Inventory_ChangeAmmo(ITEM_NUT, -1);
        Actor_Spawn(
            &play->actorCtx,
            play,
            ACTOR_EN_ARROW,
            this->bodyPartsPos[PLAYER_BODYPART_R_HAND].x,
            this->bodyPartsPos[PLAYER_BODYPART_R_HAND].y,
            this->bodyPartsPos[PLAYER_BODYPART_R_HAND].z,
            4000,
            this->actor.shape.rot.y,
            0,
            ARROW_NUT
        );
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
    }
    
    Player_StepLinearVelToZero(this);
}

void Player_GetItem(Player* this, PlayState* play) {
    static struct_80832924 sAnimSfx_ChildOpenChest[] = {
        { 0,                     0x3857                      },
        { NA_SE_VO_LI_CLIMB_END, 0x2057                      },
        { NA_SE_VO_LI_AUTO_JUMP, 0x2045                      },
        { 0,                     -0x287B                     },
    };
    s32 cond;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_850 != 0) {
            if (this->unk_850 >= 2) {
                this->unk_850--;
            }
            
            if (Player_SetupGetItemText(play, this) && (this->unk_850 == 1)) {
                cond = ((this->targetActor != NULL) && (this->exchangeItemId < 0)) ||
                    (this->stateFlags3 & PLAYER_STATE3_5);
                
                if (cond || (gSaveContext.healthAccumulator == 0)) {
                    if (cond) {
                        Player_EndGetItem(play, this);
                        this->exchangeItemId = EXCH_ITEM_NONE;
                        
                        if (Player_SetupForcePullOcarina(play, this) == 0) {
                            Player_StartTalkingWithActor(play, this->targetActor);
                        }
                    } else {
                        Player_SetupEndGetItem(play, this);
                    }
                }
            }
        } else {
            Player_EndAnimMovement(this);
            
            if (this->getItemId == GI_ICE_TRAP) {
                this->stateFlags1 &= ~(PLAYER_STATE1_10 | PLAYER_STATE1_11);
                
                if (this->getItemId != GI_ICE_TRAP) {
                    Actor_Spawn(
                        &play->actorCtx,
                        play,
                        ACTOR_EN_CLEAR_TAG,
                        this->actor.world.pos.x,
                        this->actor.world.pos.y + 100.0f,
                        this->actor.world.pos.z,
                        0,
                        0,
                        0,
                        0
                    );
                    Player_SetupStandingStillNoMorph(this, play);
                } else {
                    this->actor.colChkInfo.damage = 0;
                    Player_SetupDamage(play, this, 3, 0.0f, 0.0f, 0, 20);
                }
                
                return;
            }
            
            if (this->skelAnime.animation == &gPlayerAnim_link_normal_box_kick) {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_get_itemB);
            } else {
                Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_get_itemA);
            }
            
            this->unk_850 = 2;
            Player_SetCameraTurnAround(play, 9);
        }
    } else {
        if (this->unk_850 == 0) {
            if (!LINK_IS_ADULT) {
                Player_PlayAnimSfx(this, sAnimSfx_ChildOpenChest);
            }
            
            return;
        }
        
        if (this->skelAnime.animation == &gPlayerAnim_link_demo_get_itemB) {
            Math_ScaledStepToS(&this->actor.shape.rot.y, Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)) + 0x8000, 4000);
        }
        
        if (LinkAnimation_OnFrame(&this->skelAnime, 21.0f)) {
            Player_SetGetItemID(this, play);
        }
    }
}

void Player_PlaySwingSwordSfx(Player* this) {
    static struct_80832924 sAnimSfx_SwingSword[] = {
        { NA_SE_IT_MASTER_SWORD_SWING, -0x83C },
    };
    
    Player_PlayAnimSfx(this, sAnimSfx_SwingSword);
}

void Player_EndTimeTravel(Player* this, PlayState* play) {
    static struct_80832924 sAnimSfx_ChildEndTimeTravel[] = {
        { NA_SE_VO_LI_AUTO_JUMP, 0x2005                      },
        { 0,                     -0x280F                     },
    };
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_84F == 0) {
            if (DECR(this->unk_850) == 0) {
                this->unk_84F = 1;
                this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
            }
        } else {
            Player_SetupStandingStillNoMorph(this, play);
        }
    } else {
        if (LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 158.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_N);
            
            return;
        }
        
        if (!LINK_IS_ADULT) {
            Player_PlayAnimSfx(this, sAnimSfx_ChildEndTimeTravel);
        } else {
            Player_PlaySwingSwordSfx(this);
        }
    }
}

typedef enum {
    PLAYER_DRINK_HEAL_MAJOR = 1 << 0,
    PLAYER_DRINK_MAGIC      = 1 << 1,
    PLAYER_DRING_HEAL_MINOR = 1 << 2,
} PlayerDrinkEffect;

static u8 sBottleDrinkEffects[] = {
    PLAYER_DRINK_HEAL_MAJOR,
    PLAYER_DRINK_HEAL_MAJOR | PLAYER_DRINK_MAGIC,
    PLAYER_DRINK_MAGIC,
    PLAYER_DRING_HEAL_MINOR,
    PLAYER_DRING_HEAL_MINOR,
};

void Player_DrinkFromBottle(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_850 == 0) {
            if (this->itemActionParam == PLAYER_AP_BOTTLE_POE) {
                s32 rand = Rand_S16Offset(-1, 3);
                
                if (rand == 0) {
                    rand = 3;
                }
                
                if ((rand < 0) && (gSaveContext.health <= 0x10)) {
                    rand = 3;
                }
                
                if (rand < 0) {
                    Health_ChangeBy(play, -0x10);
                } else {
                    gSaveContext.healthAccumulator = rand * 0x10;
                }
            } else {
                s32 sp28 = sBottleDrinkEffects[this->itemActionParam - PLAYER_AP_BOTTLE_POTION_RED];
                
                if (sp28 & 1) {
                    gSaveContext.healthAccumulator = 0x140;
                }
                
                if (sp28 & 2) {
                    Magic_Fill(play);
                }
                
                if (sp28 & 4) {
                    gSaveContext.healthAccumulator = 0x50;
                }
            }
            
            Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_link_bottle_drink_demo_wait);
            this->unk_850 = 1;
            
            return;
        }
        
        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    } else if (this->unk_850 == 1) {
        if ((gSaveContext.healthAccumulator == 0) && (gSaveContext.magicState != MAGIC_STATE_FILL)) {
            Player_ChangeAnimSlowedMorphToLastFrame(play, this, &gPlayerAnim_link_bottle_drink_demo_end);
            this->unk_850 = 2;
            Player_UpdateBottleHeld(play, this, ITEM_BOTTLE, PLAYER_AP_BOTTLE);
        }
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_DRINK - SFX_FLAG);
    } else if ((this->unk_850 == 2) && LinkAnimation_OnFrame(&this->skelAnime, 29.0f)) {
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_BREATH_DRINK);
    }
}

static BottleCatchInfo sBottleCatchInfos[] = {
    { ACTOR_EN_ELF,      ITEM_FAIRY,      0x2A,       0x46 },
    { ACTOR_EN_FISH,     ITEM_FISH,       0x1F,       0x47 },
    { ACTOR_EN_ICE_HONO, ITEM_BLUE_FIRE,  0x20,       0x5D },
    { ACTOR_EN_INSECT,   ITEM_BUG,        0x21,       0x7A },
};

void Player_SwingBottle(Player* this, PlayState* play) {
    struct_80854554* sp24;
    BottleCatchInfo* catchInfo;
    s32 temp;
    s32 i;
    
    sp24 = &sAnims_BottleSwing[this->unk_850];
    Player_StepLinearVelToZero(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_84F != 0) {
            if (this->unk_850 == 0) {
                Message_StartTextbox(play, sBottleCatchInfos[this->unk_84F - 1].textId, &this->actor);
                Audio_PlayFanfare(NA_BGM_ITEM_GET | 0x900);
                this->unk_850 = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->unk_84F = 0;
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            Player_SetupStandingStillNoMorph(this, play);
        }
    } else {
        if (this->unk_84F == 0) {
            temp = this->skelAnime.curFrame - sp24->unk_08;
            
            if (temp >= 0) {
                if (sp24->unk_09 >= temp) {
                    if (this->unk_850 != 0) {
                        if (temp == 0) {
                            func_8002F7DC(&this->actor, NA_SE_IT_SCOOP_UP_WATER);
                        }
                    }
                    
                    if (this->interactRangeActor != NULL) {
                        catchInfo = &sBottleCatchInfos[0];
                        for (i = 0; i < 4; i++, catchInfo++) {
                            if (this->interactRangeActor->id == catchInfo->actorId) {
                                break;
                            }
                        }
                        
                        if (i < 4) {
                            this->unk_84F = i + 1;
                            this->unk_850 = 0;
                            this->stateFlags1 |= PLAYER_STATE1_28 | PLAYER_STATE1_29;
                            this->interactRangeActor->parent = &this->actor;
                            Player_UpdateBottleHeld(play, this, catchInfo->itemId, ABS(catchInfo->actionParam));
                            Player_PlayAnimOnceSlowed(play, this, sp24->unk_04);
                            Player_SetCameraTurnAround(play, 4);
                        }
                    }
                }
            }
        }
    }
    
    if (this->skelAnime.curFrame <= 7.0f) {
        this->stateFlags1 |= PLAYER_STATE1_SWINGING_BOTTLE;
    }
}

static Vec3f sBottleFairyPos = { 0.0f, 0.0f, 5.0f };

void Player_HealWithFairy(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 37.0f)) {
        Player_SpawnFairy(play, this, &this->leftHandPos, &sBottleFairyPos, FAIRY_REVIVE_BOTTLE);
        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE, PLAYER_AP_BOTTLE);
        func_8002F7DC(&this->actor, NA_SE_EV_BOTTLE_CAP_OPEN);
        func_8002F7DC(&this->actor, NA_SE_EV_FIATY_HEAL - SFX_FLAG);
    } else if (LinkAnimation_OnFrame(&this->skelAnime, 47.0f)) {
        gSaveContext.healthAccumulator = 0x140;
    }
}

static BottleDropInfo D_80854A28[] = {
    { ACTOR_EN_FISH,     FISH_DROPPED                },
    { ACTOR_EN_ICE_HONO, 0                           },
    { ACTOR_EN_INSECT,   INSECT_TYPE_FIRST_DROPPED   },
};

static struct_80832924 sAnimSfx_BottleDrop[] = {
    { NA_SE_VO_LI_AUTO_JUMP,    0x2026    },
    { NA_SE_EV_BOTTLE_CAP_OPEN, -0x828    },
};

void Player_DropItemFromBottle(Player* this, PlayState* play) {
    Player_StepLinearVelToZero(this);
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupStandingStillNoMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
        
        return;
    }
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 76.0f)) {
        BottleDropInfo* dropInfo = &D_80854A28[this->itemActionParam - PLAYER_AP_BOTTLE_FISH];
        
        Actor_Spawn(
            &play->actorCtx,
            play,
            dropInfo->actorId,
            (Math_SinS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.x,
            this->leftHandPos.y,
            (Math_CosS(this->actor.shape.rot.y) * 5.0f) + this->leftHandPos.z,
            0x4000,
            this->actor.shape.rot.y,
            0,
            dropInfo->actorParams
        );
        
        Player_UpdateBottleHeld(play, this, ITEM_BOTTLE, PLAYER_AP_BOTTLE);
        
        return;
    }
    
    Player_PlayAnimSfx(this, sAnimSfx_BottleDrop);
}

static struct_80832924 sAnimSfx_ExchangeItem[] = {
    { NA_SE_PL_PUT_OUT_ITEM, -0x81E },
};

void Player_PresentExchangeItem(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_850 < 0) {
            Player_SetupStandingStillNoMorph(this, play);
        } else if (this->exchangeItemId == EXCH_ITEM_NONE) {
            Actor* targetActor = this->targetActor;
            
            this->unk_862 = 0;
            if (targetActor->textId != 0xFFFF) {
                this->actor.flags |= ACTOR_FLAG_8;
            }
            
            Player_StartTalkingWithActor(play, targetActor);
        } else {
            GetItemEntry* giEntry = &sGetItemTable[sGetItemID_ExchangeItem[this->exchangeItemId - 1] - 1];
            
            if (this->itemActionParam >= PLAYER_AP_LETTER_ZELDA) {
                if (giEntry->gi >= 0) {
                    this->unk_862 = giEntry->gi;
                } else {
                    this->unk_862 = -giEntry->gi;
                }
            }
            
            if (this->unk_850 == 0) {
                Message_StartTextbox(play, this->actor.textId, &this->actor);
                
                if ((this->itemActionParam == PLAYER_AP_CHICKEN) || (this->itemActionParam == PLAYER_AP_POCKET_CUCCO)) {
                    func_8002F7DC(&this->actor, NA_SE_EV_CHICKEN_CRY_M);
                }
                
                this->unk_850 = 1;
            } else if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
                this->actor.flags &= ~ACTOR_FLAG_8;
                this->unk_862 = 0;
                
                if (this->unk_84F == 1) {
                    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_bottle_read_end);
                    this->unk_850 = -1;
                } else {
                    Player_SetupStandingStillNoMorph(this, play);
                }
                
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        }
    } else if (this->unk_850 >= 0) {
        Player_PlayAnimSfx(this, sAnimSfx_ExchangeItem);
    }
    
    if ((this->unk_84F == 0) && (this->unk_664 != NULL)) {
        this->currentYaw = this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_RestrainedByEnemy(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5 | PLAYER_STATE2_6;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_normal_re_dead_attack_wait);
    }
    
    if (Player_CanBreakFree(this, 0, 100)) {
        Player_SetupStandingStillType(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_7;
    }
}

void Player_SlipOnSlope(Player* this, PlayState* play) {
    CollisionPoly* floorPoly;
    f32 sp50;
    f32 sp4C;
    f32 sp48;
    s16 downwardSlopeYaw;
    s16 sp44;
    Vec3f slopeNormal;
    
    this->stateFlags2 |= PLAYER_STATE2_5 | PLAYER_STATE2_6;
    LinkAnimation_Update(play, &this->skelAnime);
    Player_SetupSpawnDustAtFeet(play, this);
    func_800F4138(&this->actor.projectedPos, NA_SE_PL_SLIP_LEVEL - SFX_FLAG, this->actor.speedXZ);
    
    if (Player_SetupItemCsOrFirstPerson(this, play) == 0) {
        floorPoly = this->actor.floorPoly;
        
        if (floorPoly == NULL) {
            Player_SetupFallFromLedge(this, play);
            
            return;
        }
        
        Player_GetSlopeDirection(floorPoly, &slopeNormal, &downwardSlopeYaw);
        
        sp44 = downwardSlopeYaw;
        if (this->unk_84F != 0) {
            sp44 = downwardSlopeYaw + 0x8000;
        }
        
        if (this->linearVelocity < 0) {
            downwardSlopeYaw += 0x8000;
        }
        
        sp50 = (1.0f - slopeNormal.y) * 40.0f;
        sp50 = CLAMP(sp50, 0, 10.0f);
        sp4C = (sp50 * sp50) * 0.015f;
        sp48 = slopeNormal.y * 0.01f;
        
        if (SurfaceType_GetFloorEffect(&play->colCtx, floorPoly, this->actor.floorBgId) != 1) {
            sp50 = 0;
            sp48 = slopeNormal.y * 10.0f;
        }
        
        if (sp4C < 1.0f) {
            sp4C = 1.0f;
        }
        
        if (Math_AsymStepToF(&this->linearVelocity, sp50, sp4C, sp48) && (sp50 == 0)) {
            LinkAnimationHeader* anim;
            
            if (this->unk_84F == 0) {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_42, this->modelAnimType);
            } else {
                anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_43, this->modelAnimType);
            }
            Player_SetupReturnToStandStillSetAnim(this, anim, play);
        }
        
        Math_SmoothStepToS(&this->currentYaw, downwardSlopeYaw, 10, 4000, 800);
        Math_ScaledStepToS(&this->actor.shape.rot.y, sp44, 2000);
    }
}

void Player_SetDrawAndStartCutsceneAfterTimer(Player* this, PlayState* play) {
    if ((DECR(this->unk_850) == 0) && Player_SetupCutscene(play, this)) {
        Player_Cutscene_SetDraw(play, this, NULL);
        Player_SetActionFunc(play, this, Player_StartCutscene, 0);
        Player_StartCutscene(this, play);
    }
}

void Player_SpawnFromWarpSong(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_SetDrawAndStartCutsceneAfterTimer, 0);
    this->unk_850 = 40;
    Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0x10);
}

void Player_SpawnFromBlueWarp(Player* this, PlayState* play) {
    if ((this->unk_84F != 0) && (play->csCtx.frames < 0x131)) {
        this->actor.gravity = 0.0f;
        this->actor.velocity.y = 0.0f;
    } else if (sFloorDistY < 150.0f) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            if (this->unk_850 == 0) {
                if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                    this->skelAnime.endFrame = this->skelAnime.animLength - 1.0f;
                    Player_PlayLandingSfx(this);
                    this->unk_850 = 1;
                }
            } else {
                if ((play->sceneId == SCENE_SPOT04) && Player_SetupCutscene(play, this)) {
                    return;
                }
                Player_SetupStandingStillMorph(this, play);
            }
        }
        Math_SmoothStepToF(&this->actor.velocity.y, 2.0f, 0.3f, 8.0f, 0.5f);
    }
    
    if ((play->sceneId == SCENE_KENJYANOMA) && Player_SetupCutscene(play, this)) {
        return;
    }
    
    if ((play->csCtx.state != CS_STATE_IDLE) && (play->csCtx.linkAction != NULL)) {
        f32 sp28 = this->actor.world.pos.y;
        Player_CutsceneSetPosAndYaw(play, this, play->csCtx.linkAction);
        this->actor.world.pos.y = sp28;
    }
}

void Player_EnterGrotto(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);
    
    if ((this->unk_850++ > 8) && (play->transitionTrigger == TRANS_TRIGGER_OFF)) {
        if (this->unk_84F != 0) {
            if (play->sceneId == 9) {
                Play_TriggerRespawn(play);
                gExitParam.nextEntranceIndex = ENTR_ICE_DOUKUTO_0;
            } else if (this->unk_84F < 0) {
                Play_TriggerRespawn(play);
            } else {
                Play_TriggerVoidOut(play);
            }
            
            play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
            func_80078884(NA_SE_OC_ABYSS);
        } else {
            play->transitionType = TRANS_TYPE_FADE_BLACK;
            gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
            gSaveContext.seqId = (u8)NA_BGM_DISABLED;
            gSaveContext.natureAmbienceId = 0xFF;
        }
        
        play->transitionTrigger = TRANS_TRIGGER_START;
    }
}

void Player_SetupOpenDoorFromSpawn(Player* this, PlayState* play) {
    Player_SetupOpenDoor(this, play);
}

void Player_JumpFromGrotto(Player* this, PlayState* play) {
    this->actor.gravity = -1.0f;
    
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (this->actor.velocity.y < 0.0f) {
        Player_SetupFallFromLedge(this, play);
    } else if (this->actor.velocity.y < 6.0f) {
        Math_StepToF(&this->linearVelocity, 3.0f, 0.5f);
    }
}

void Player_ShootingGalleryPlay(Player* this, PlayState* play) {
    this->unk_6AD = 2;
    
    Player_SetupCameraMode(play, this);
    LinkAnimation_Update(play, &this->skelAnime);
    Player_SetupCurrentUpperAction(this, play);
    
    this->unk_6BE = Player_SetFirstPersonAimLookAngles(play, this, 1, 0) - this->actor.shape.rot.y;
    this->unk_6AE |= 0x80;
    
    if (play->shootingGalleryStatus < 0) {
        play->shootingGalleryStatus++;
        if (play->shootingGalleryStatus == 0) {
            Player_ClearLookAndAttention(this, play);
        }
    }
}

void Player_FrozenInIce(Player* this, PlayState* play) {
    if (this->unk_84F >= 0) {
        if (this->unk_84F < 6) {
            this->unk_84F++;
        }
        
        if (Player_CanBreakFree(this, 1, 100)) {
            this->unk_84F = -1;
            EffectSsIcePiece_SpawnBurst(play, &this->actor.world.pos, this->actor.scale.x);
            func_8002F7DC(&this->actor, NA_SE_PL_ICE_BROKEN);
        } else {
            this->stateFlags2 |= PLAYER_STATE2_14;
        }
        
        if ((play->gameplayFrames % 4) == 0) {
            Player_InflictDamage(play, -1);
        }
    } else {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_SetupStandingStillType(this, play);
            Player_SetupInvincibility_NoDamageFlash(this, -20);
        }
    }
}

void Player_SetupElectricShock(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_RoundUpInvincibilityTimer(this);
    
    if (((this->unk_850 % 25) != 0) || Player_ApplyDamage(play, this, -1)) {
        if (DECR(this->unk_850) == 0) {
            Player_SetupStandingStillType(this, play);
        }
    }
    
    this->shockTimer = 40;
    func_8002F8F0(&this->actor, NA_SE_VO_LI_TAKEN_AWAY - SFX_FLAG + this->ageProperties->unk_92);
}

s32 Player_DebugMode(Player* this, PlayState* play) {
#ifdef DEV_BUILD
    static s32 cameraMode;
    sControlInput = &play->state.input[0];
    
    if (
        (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A | BTN_L | BTN_R) &&
        CHECK_BTN_ALL(sControlInput->press.button, BTN_B)) ||
        (CHECK_BTN_ALL(sControlInput->cur.button, BTN_L) && CHECK_BTN_ALL(sControlInput->press.button, BTN_DRIGHT))
    ) {
        sDebugModeFlag ^= 1;
        cameraMode = 0;
    }
    
    if (sDebugModeFlag) {
        f32 speed;
        
        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_R)) {
            speed = 250.0f;
        } else {
            speed = 50.0f;
        }
        
        if (CHK_ALL(press, BTN_Z)) {
            if (cameraMode == 0)
                Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_TARGET);
            
            else
                Camera_ChangeMode(Play_GetCamera(play, CAM_ID_MAIN), CAM_MODE_NORMAL);
            
            cameraMode ^= 1;
        }
        
        if (cameraMode == 0) {
            Camera* cam = GET_ACTIVE_CAM(play);
            s32 mod = 0;
            
            if (CHK_ALL(cur, BTN_CRIGHT)) {
                cam->eye.x += Math_SinS(Camera_GetCamDirYaw(cam) + 0x3FFF) * Math_Vec3f_DistXYZ(&cam->eye, &cam->at) * 0.15f;
                cam->eye.z += Math_CosS(Camera_GetCamDirYaw(cam) + 0x3FFF) * Math_Vec3f_DistXYZ(&cam->eye, &cam->at) * 0.15f;
                cam->eyeNext = cam->eye;
                mod = true;
            }
            
            if (CHK_ALL(cur, BTN_CLEFT)) {
                cam->eye.x += Math_SinS(Camera_GetCamDirYaw(cam) - 0x3FFF) * Math_Vec3f_DistXYZ(&cam->eye, &cam->at) * 0.15f;
                cam->eye.z += Math_CosS(Camera_GetCamDirYaw(cam) - 0x3FFF) * Math_Vec3f_DistXYZ(&cam->eye, &cam->at) * 0.15f;
                cam->eyeNext = cam->eye;
                mod = true;
            }
            
            if (mod)
                this->actor.shape.rot.y = Camera_GetCamDirYaw(cam);
        }
        
        func_8006375C(3, 2, "DEBUG MODE");
        
        if (!CHECK_BTN_ALL(sControlInput->cur.button, BTN_L)) {
            if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_B)) {
                this->actor.world.pos.y += speed * 0.45;
            } else if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_A)) {
                this->actor.world.pos.y -= speed * 0.45;
            }
            
            OSContPad* ctrl = &play->state.input[0].cur;
            Vec3f zero = {};
            Vec3f conpos = {
                .x = -((f32)ctrl->stick_x / 128),
                .z = (f32)ctrl->stick_y / 128,
            };
            s16 angle;
            f32 speedC = Math_Vec3f_DistXZ(&zero, &conpos);
            
            if (speedC <= 0.1f)
                speedC = 0.0f;
            
            angle = Camera_GetInputDirYaw(GET_ACTIVE_CAM(play));
            if (CHK_ANY(cur, BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT)) {
                if (CHK_ANY(cur, BTN_DUP))
                    conpos.z = 1.0f;
                if (CHK_ANY(cur, BTN_DDOWN))
                    conpos.z = -1.0f;
                if (CHK_ANY(cur, BTN_DLEFT))
                    conpos.x = 1.0f;
                if (CHK_ANY(cur, BTN_DRIGHT))
                    conpos.x = -1.0f;
                speedC = 1.0f;
            }
            
            angle += Math_Vec3f_Yaw(&zero, &conpos);
            
            this->actor.world.pos.x += speed * Math_SinS(angle) * speedC;
            this->actor.world.pos.z += speed * Math_CosS(angle) * speedC;
        }
        
        Player_StopMovement(this);
        
        this->actor.gravity = 0.0f;
        this->actor.velocity.z = 0.0f;
        this->actor.velocity.y = 0.0f;
        this->actor.velocity.x = 0.0f;
        
        if (CHECK_BTN_ALL(sControlInput->cur.button, BTN_L) && CHECK_BTN_ALL(sControlInput->press.button, BTN_DLEFT)) {
            Flags_SetTempClear(play, play->roomCtx.curRoom.num);
        }
        
        Math_Vec3f_Copy(&this->actor.home.pos, &this->actor.world.pos);
        
        return 0;
    }
#endif
    
    return 1;
}

void Player_BowStringMoveAfterShot(Player* this) {
    this->unk_858 += this->unk_85C;
    this->unk_85C -= this->unk_858 * 5.0f;
    this->unk_85C *= 0.3f;
    
    if (ABS(this->unk_85C) < 0.00001f) {
        this->unk_85C = 0.0f;
        if (ABS(this->unk_858) < 0.00001f) {
            this->unk_858 = 0.0f;
        }
    }
}

void Player_BunnyHoodPhysics(Player* this) {
    s16 sp2A;
    s16 sp28;
    s16 sp26;
    
    D_80858AC8.unk_06 -= D_80858AC8.unk_06 >> 3;
    D_80858AC8.unk_08 -= D_80858AC8.unk_08 >> 3;
    D_80858AC8.unk_06 += -D_80858AC8.unk_00 >> 2;
    D_80858AC8.unk_08 += -D_80858AC8.unk_02 >> 2;
    
    sp26 = this->actor.world.rot.y - this->actor.shape.rot.y;
    
    sp28 = (s32)(this->actor.speedXZ * -200.0f * Math_CosS(sp26) * (Rand_CenteredFloat(2.0f) + 10.0f)) & 0xFFFF;
    sp2A = (s32)(this->actor.speedXZ * 100.0f * Math_SinS(sp26) * (Rand_CenteredFloat(2.0f) + 10.0f)) & 0xFFFF;
    
    D_80858AC8.unk_06 += sp28 >> 2;
    D_80858AC8.unk_08 += sp2A >> 2;
    
    if (D_80858AC8.unk_06 > 6000) {
        D_80858AC8.unk_06 = 6000;
    } else if (D_80858AC8.unk_06 < -6000) {
        D_80858AC8.unk_06 = -6000;
    }
    
    if (D_80858AC8.unk_08 > 6000) {
        D_80858AC8.unk_08 = 6000;
    } else if (D_80858AC8.unk_08 < -6000) {
        D_80858AC8.unk_08 = -6000;
    }
    
    D_80858AC8.unk_00 += D_80858AC8.unk_06;
    D_80858AC8.unk_02 += D_80858AC8.unk_08;
    
    if (D_80858AC8.unk_00 < 0) {
        D_80858AC8.unk_04 = D_80858AC8.unk_00 >> 1;
    } else {
        D_80858AC8.unk_04 = 0;
    }
}

s32 Player_SetupStartMeleeWeaponAttack(Player* this, PlayState* play) {
    if (Player_CanSwingBottleOrCastFishingRod(play, this) == 0) {
        if (Player_CanJumpSlash(this) != 0) {
            s32 sp24 = Player_GetMeleeAttackAnimID(this);
            
            Player_StartMeleeWeaponAttack(play, this, sp24);
            
            if (sp24 >= PLAYER_MWA_SPIN_ATTACK_1H) {
                this->stateFlags2 |= PLAYER_STATE2_17;
                Player_SetupSpinAttackActor(play, this, 0);
                
                return 1;
            }
        } else {
            return 0;
        }
    }
    
    return 1;
}

static Vec3f sShockwaveRaycastPos = { 0.0f, 40.0f, 45.0f };

void Player_MeleeWeaponAttack(Player* this, PlayState* play) {
    struct_80854190* sp44 = &sAnims_MeleeAttack[this->meleeWeaponAnimation];
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    if (!func_80842DF4(play, this)) {
        Player_SetupMeleeAttack(this, 0.0f, sp44->unk_0C, sp44->unk_0D);
        
        if (
            (this->stateFlags2 & PLAYER_STATE2_30) && (this->heldItemActionParam != PLAYER_AP_HAMMER) &&
            LinkAnimation_OnFrame(&this->skelAnime, 0.0f)
        ) {
            this->linearVelocity = 15.0f;
            this->stateFlags2 &= ~PLAYER_STATE2_30;
        }
        
        if (this->linearVelocity > 12.0f) {
            Player_SetupSpawnDustAtFeet(play, this);
        }
        
        Math_StepToF(&this->linearVelocity, 0.0f, 5.0f);
        Player_DeactivateComboTimer(this);
        
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            if (!Player_SetupStartMeleeWeaponAttack(this, play)) {
                u8 sp43 = this->skelAnime.moveFlags;
                LinkAnimationHeader* sp3C;
                
                if (func_8008E9C4(this)) {
                    sp3C = sp44->unk_08;
                } else {
                    sp3C = sp44->unk_04;
                }
                
                Player_InactivateMeleeWeapon(this);
                this->skelAnime.moveFlags = 0;
                
                if ((sp3C == &gPlayerAnim_link_fighter_Lpower_jump_kiru_end) && (this->modelAnimType != PLAYER_ANIMTYPE_3)) {
                    sp3C = &gPlayerAnim_link_fighter_power_jump_kiru_end;
                }
                
                Player_SetupReturnToStandStillSetAnim(this, sp3C, play);
                
                this->skelAnime.moveFlags = sp43;
                this->stateFlags3 |= PLAYER_STATE3_3;
            }
        } else if (this->heldItemActionParam == PLAYER_AP_HAMMER) {
            if (
                (this->meleeWeaponAnimation == PLAYER_MWA_HAMMER_FORWARD) ||
                (this->meleeWeaponAnimation == PLAYER_MWA_JUMPSLASH_FINISH)
            ) {
                static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
                Vec3f shockwavePos;
                f32 sp2C;
                
                shockwavePos.y = Player_RelativeFloorRaycast(play, this, &sShockwaveRaycastPos, &shockwavePos);
                sp2C = this->actor.world.pos.y - shockwavePos.y;
                
                Math_ScaledStepToS(&this->actor.focus.rot.x, Math_Atan2S(45.0f, sp2C), 800);
                Player_UpdateLookRot(this, 1);
                
                if (
                    (((this->meleeWeaponAnimation == PLAYER_MWA_HAMMER_FORWARD) &&
                    LinkAnimation_OnFrame(&this->skelAnime, 7.0f)) ||
                    ((this->meleeWeaponAnimation == PLAYER_MWA_JUMPSLASH_FINISH) &&
                    LinkAnimation_OnFrame(&this->skelAnime, 2.0f))) &&
                    (sp2C > -40.0f) && (sp2C < 40.0f)
                ) {
                    Player_SetupHammerHit(play, this);
                    EffectSsBlast_SpawnWhiteShockwave(play, &shockwavePos, &zeroVec, &zeroVec);
                }
            }
        }
    }
}

void Player_MeleeWeaponRebound(Player* this, PlayState* play) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_StepLinearVelToZero(this);
    
    if (this->skelAnime.curFrame >= 6.0f) {
        Player_ReturnToStandStill(this, play);
    }
}

void Player_ChooseFaroresWindOption(Player* this, PlayState* play) {
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    LinkAnimation_Update(play, &this->skelAnime);
    Player_SetupCurrentUpperAction(this, play);
    
    if (this->unk_850 == 0) {
        Message_StartTextbox(play, 0x3B, &this->actor);
        this->unk_850 = 1;
        
        return;
    }
    
    if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING) {
        s32 respawnData = gSaveContext.respawn[RESPAWN_MODE_TOP].data;
        
        if (play->msgCtx.choiceIndex == 0) {
            gSaveContext.respawnFlag = 3;
            play->transitionTrigger = TRANS_TRIGGER_START;
            
            gExitParam.nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_TOP].entranceIndex;
            
            if (gExitParam.isExit) {
                gExitParam.exit = gExitParam.respawn[RESPAWN_MODE_TOP];
            }
            
            play->transitionType = TRANS_TYPE_FADE_WHITE_FAST;
            func_80088AF0(play);
            
            return;
        }
        
        if (play->msgCtx.choiceIndex == 1) {
            gSaveContext.respawn[RESPAWN_MODE_TOP].data = -respawnData;
            gSaveContext.fw.set = 0;
            func_80078914(&gSaveContext.respawn[RESPAWN_MODE_TOP].pos, NA_SE_PL_MAGIC_WIND_VANISH);
        }
        
        Player_SetupStandingStillMorph(this, play);
        func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
    }
}

void Player_SpawnFromFaroresWind(Player* this, PlayState* play) {
    s32 respawnData = gSaveContext.respawn[RESPAWN_MODE_TOP].data;
    
    if (this->unk_850 > 20) {
        this->actor.draw = Player_Draw;
        this->actor.world.pos.y += 60.0f;
        Player_SetupFallFromLedge(this, play);
        
        return;
    }
    
    if (this->unk_850++ == 20) {
        gSaveContext.respawn[RESPAWN_MODE_TOP].data = respawnData + 1;
        func_80078914(&gSaveContext.respawn[RESPAWN_MODE_TOP].pos, NA_SE_PL_MAGIC_WIND_WARP);
    }
}

static LinkAnimationHeader* sAnims_MagicSpellStart[] = {
    &gPlayerAnim_link_magic_kaze1,
    &gPlayerAnim_link_magic_honoo1,
    &gPlayerAnim_link_magic_tamashii1,
};

static LinkAnimationHeader* sAnims_MagicSpellCast[] = {
    &gPlayerAnim_link_magic_kaze2,
    &gPlayerAnim_link_magic_honoo2,
    &gPlayerAnim_link_magic_tamashii2,
};

static LinkAnimationHeader* sAnims_MagicSpellEnd[] = {
    &gPlayerAnim_link_magic_kaze3,
    &gPlayerAnim_link_magic_honoo3,
    &gPlayerAnim_link_magic_tamashii3,
};

static u8 sMagicSpellTimeLimits[] = { 70, 10, 10 };

static struct_80832924 sAnimSfx_MagicSpellCast[] = {
    { NA_SE_PL_SKIP,       0x814                     },
    { NA_SE_VO_LI_SWORD_N, 0x2014                    },
    { 0,                   -0x301A                   },
};

static struct_80832924 sAnimSfx_MagicSpell[][2] = {
    {
        { 0,                         0x4014                         },
        { NA_SE_VO_LI_MAGIC_FROL,    -0x201E                        },
    },
    {
        { 0,                         0x4014                         },
        { NA_SE_VO_LI_MAGIC_NALE,    -0x202C                        },
    },
    {
        { NA_SE_VO_LI_MAGIC_ATTACK,  0x2014                         },
        { NA_SE_IT_SWORD_SWING_HARD, -0x814                         },
    },
};

void Player_UpdateMagicSpell(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_84F < 0) {
            if ((this->itemActionParam == PLAYER_AP_NAYRUS_LOVE) || (gSaveContext.magicState == MAGIC_STATE_IDLE)) {
                Player_ReturnToStandStill(this, play);
                func_8005B1A4(Play_GetCamera(play, CAM_ID_MAIN));
            }
        } else {
            if (this->unk_850 == 0) {
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sAnims_MagicSpellStart[this->unk_84F], 0.83f);
                
                if (Player_SpawnMagicSpellActor(play, this, this->unk_84F) != NULL) {
                    this->stateFlags1 |= PLAYER_STATE1_28 | PLAYER_STATE1_29;
                    if ((this->unk_84F != 0) || (gSaveContext.respawn[RESPAWN_MODE_TOP].data <= 0)) {
                        gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
                    }
                } else {
                    Magic_Reset(play);
                }
            } else {
                LinkAnimation_PlayLoopSetSpeed(play, &this->skelAnime, sAnims_MagicSpellCast[this->unk_84F], 0.83f);
                
                if (this->unk_84F == 0) {
                    this->unk_850 = -10;
                }
            }
            
            this->unk_850++;
        }
    } else {
        if (this->unk_850 < 0) {
            this->unk_850++;
            
            if (this->unk_850 == 0) {
                gSaveContext.respawn[RESPAWN_MODE_TOP].data = 1;
                Play_SetupRespawnPoint(play, RESPAWN_MODE_TOP, 0x6FF);
                gSaveContext.fw.set = 1;
                gSaveContext.fw.pos.x = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.x;
                gSaveContext.fw.pos.y = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.y;
                gSaveContext.fw.pos.z = gSaveContext.respawn[RESPAWN_MODE_DOWN].pos.z;
                gSaveContext.fw.yaw = gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw;
                gSaveContext.fw.playerParams = 0x6FF;
                gSaveContext.fw.entranceIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex;
                gSaveContext.fw.roomIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].roomIndex;
                gSaveContext.fw.tempSwchFlags = gSaveContext.respawn[RESPAWN_MODE_DOWN].tempSwchFlags;
                gSaveContext.fw.tempCollectFlags = gSaveContext.respawn[RESPAWN_MODE_DOWN].tempCollectFlags;
                gExitParam.respawn[RESPAWN_MODE_DOWN] = gExitParam.exit;
                
                this->unk_850 = 2;
            }
        } else if (this->unk_84F >= 0) {
            if (this->unk_850 == 0) {
                Player_PlayAnimSfx(this, sAnimSfx_MagicSpellCast);
            } else if (this->unk_850 == 1) {
                Player_PlayAnimSfx(this, sAnimSfx_MagicSpell[this->unk_84F]);
                if ((this->unk_84F == 2) && LinkAnimation_OnFrame(&this->skelAnime, 30.0f)) {
                    this->stateFlags1 &= ~(PLAYER_STATE1_28 | PLAYER_STATE1_29);
                }
            } else if (sMagicSpellTimeLimits[this->unk_84F] < this->unk_850++) {
                LinkAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sAnims_MagicSpellEnd[this->unk_84F], 0.83f);
                this->currentYaw = this->actor.shape.rot.y;
                this->unk_84F = -1;
            }
        }
    }
    
    Player_StepLinearVelToZero(this);
}

void Player_MoveAlongHookshotPath(Player* this, PlayState* play) {
    f32 temp;
    
    this->stateFlags2 |= PLAYER_STATE2_5;
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_hook_fly_wait);
    }
    
    Math_Vec3f_Sum(&this->actor.world.pos, &this->actor.velocity, &this->actor.world.pos);
    
    if (Player_EndHookshotMove(this)) {
        Math_Vec3f_Copy(&this->actor.prevPos, &this->actor.world.pos);
        Player_UpdateBgcheck(play, this);
        
        temp = this->actor.world.pos.y - this->actor.floorHeight;
        if (temp > 20.0f) {
            temp = 20.0f;
        }
        
        this->actor.world.rot.x = this->actor.shape.rot.x = 0;
        this->actor.world.pos.y -= temp;
        this->linearVelocity = 1.0f;
        this->actor.velocity.y = 0.0f;
        Player_SetupFallFromLedge(this, play);
        this->stateFlags2 &= ~PLAYER_STATE2_10;
        this->actor.bgCheckFlags |= BGCHECKFLAG_GROUND;
        this->stateFlags1 |= PLAYER_STATE1_2;
        
        return;
    }
    
    if ((this->skelAnime.animation != &gPlayerAnim_link_hook_fly_start) || (4.0f <= this->skelAnime.curFrame)) {
        this->actor.gravity = 0.0f;
        Math_ScaledStepToS(&this->actor.shape.rot.x, this->actor.world.rot.x, 0x800);
        Player_SetRumble(this, 100, 2, 100, 0);
    }
}

void Player_CastFishingRod(Player* this, PlayState* play) {
    if ((this->unk_850 != 0) && ((this->unk_858 != 0.0f) || (this->unk_85C != 0.0f))) {
        f32 updateScale = R_UPDATE_RATE * 0.5f;
        
        this->skelAnime.curFrame += this->skelAnime.playSpeed * updateScale;
        if (this->skelAnime.curFrame >= this->skelAnime.animLength) {
            this->skelAnime.curFrame -= this->skelAnime.animLength;
        }
        
        LinkAnimation_BlendToJoint(
            play,
            &this->skelAnime,
            &gPlayerAnim_link_fishing_wait,
            this->skelAnime.curFrame,
            (this->unk_858 < 0.0f) ? &gPlayerAnim_link_fishing_reel_left : &gPlayerAnim_link_fishing_reel_right,
            5.0f,
            fabsf(this->unk_858),
            this->blendTable
        );
        LinkAnimation_BlendToMorph(
            play,
            &this->skelAnime,
            &gPlayerAnim_link_fishing_wait,
            this->skelAnime.curFrame,
            (this->unk_85C < 0.0f) ? &gPlayerAnim_link_fishing_reel_up : &gPlayerAnim_link_fishing_reel_down,
            5.0f,
            fabsf(this->unk_85C),
            D_80858AD8
        );
        LinkAnimation_InterpJointMorph(play, &this->skelAnime, 0.5f);
    } else if (LinkAnimation_Update(play, &this->skelAnime)) {
        this->unk_860 = 2;
        Player_PlayAnimLoop(play, this, &gPlayerAnim_link_fishing_wait);
        this->unk_850 = 1;
    }
    
    Player_StepLinearVelToZero(this);
    
    if (this->unk_860 == 0) {
        Player_SetupStandingStillMorph(this, play);
    } else if (this->unk_860 == 3) {
        Player_SetActionFunc(play, this, Player_ReleaseCaughtFish, 0);
        Player_ChangeAnimMorphToLastFrame(play, this, &gPlayerAnim_link_fishing_fish_catch);
    }
}

void Player_ReleaseCaughtFish(Player* this, PlayState* play) {
    if (LinkAnimation_Update(play, &this->skelAnime) && (this->unk_860 == 0)) {
        Player_SetupReturnToStandStillSetAnim(this, &gPlayerAnim_link_fishing_fish_catch_end, play);
    }
}

static void (*sCutsceneModePlaybackFunc[])(PlayState*, Player*, void*) = {
    NULL,                      Player_AnimPlaybackType0,          Player_AnimPlaybackType1,           Player_AnimPlaybackType2,  Player_AnimPlaybackType3,  Player_AnimPlaybackType4,  Player_AnimPlaybackType5,
    Player_AnimPlaybackType6,  Player_AnimPlaybackType7,          Player_AnimPlaybackType8,           Player_AnimPlaybackType9,  Player_AnimPlaybackType10, Player_AnimPlaybackType11, Player_AnimPlaybackType12,
    Player_AnimPlaybackType13, Player_AnimPlaybackType14,         Player_AnimPlaybackType15,          Player_AnimPlaybackType16, Player_AnimPlaybackType17,
};

static struct_80832924 sAnimSfx_GetUpFromDekuTreeStory[] = {
    { 0,                 0x2822                 },
    { NA_SE_PL_CALM_HIT, 0x82D                  },
    { NA_SE_PL_CALM_HIT, 0x833                  },
    { NA_SE_PL_CALM_HIT, -0x840                 },
};

static struct_80832924 sAnimSfx_SurprisedStumbleBackFall[] = {
    { NA_SE_VO_LI_SURPRISE, 0x2003 }, { 0, 0x300F }, { 0, 0x3018 }, { 0, 0x301E }, { NA_SE_VO_LI_FALL_L, -0x201F },
};

static struct_80832924 sAnimSfx_FallToKnee[] = {
    { 0, -0x300A },
};

static struct_80854B18 sCutsceneModeInitFuncs[] = {
    { 0,   { NULL                                         } },
    { -1,  { Player_Cutscene_SetupIdle                    } },
    { 2,   { &gPlayerAnim_link_demo_goma_furimuki         } },
    { 0,   { NULL                                         } },
    { 0,   { NULL                                         } },
    { 3,   { &gPlayerAnim_link_demo_bikkuri               } },
    { 0,   { NULL                                         } },
    { 0,   { NULL                                         } },
    { -1,  { Player_Cutscene_SetupIdle                    } },
    { 2,   { &gPlayerAnim_link_demo_furimuki              } },
    { -1,  { Player_Cutscene_SetupEnterWarp               } },
    { 3,   { &gPlayerAnim_link_demo_warp                  } },
    { -1,  { Player_Cutscene_SetupFightStance             } },
    { 7,   { &gPlayerAnim_clink_demo_get1                 } },
    { 5,   { &gPlayerAnim_clink_demo_get2                 } },
    { 5,   { &gPlayerAnim_clink_demo_get3                 } },
    { 5,   { &gPlayerAnim_clink_demo_standup              } },
    { 7,   { &gPlayerAnim_clink_demo_standup_wait         } },
    { -1,  { Player_Cutscene_SetupSwordPedestal           } },
    { 2,   { &gPlayerAnim_link_demo_baru_op1              } },
    { 2,   { &gPlayerAnim_link_demo_baru_op3              } },
    { 0,   { NULL                                         } },
    { -1,  { Player_Cutscene_SetupWarpToSages             } },
    { 3,   { &gPlayerAnim_link_demo_jibunmiru             } },
    { 9,   { &gPlayerAnim_link_normal_back_downA          } },
    { 2,   { &gPlayerAnim_link_normal_back_down_wake      } },
    { -1,  { Player_Cutscene_SetupStartPlayOcarina        } },
    { 2,   { &gPlayerAnim_link_normal_okarina_end         } },
    { 3,   { &gPlayerAnim_link_demo_get_itemA             } },
    { -1,  { Player_Cutscene_SetupIdle                    } },
    { 2,   { &gPlayerAnim_link_normal_normal2fighter_free } },
    { 0,   { NULL                                         } },
    { 0,   { NULL                                         } },
    { 5,   { &gPlayerAnim_clink_demo_atozusari            } },
    { -1,  { Player_Cutscene_SetupSwimIdle                } },
    { -1,  { Player_Cutscene_SetupGetItemInWater          } },
    { 5,   { &gPlayerAnim_clink_demo_bashi                } },
    { 16,  { &gPlayerAnim_link_normal_hang_up_down        } },
    { -1,  { Player_Cutscene_SetupSleepingRestless        } },
    { -1,  { Player_Cutscene_SetupSleeping                } },
    { 6,   { &gPlayerAnim_clink_op3_okiagari              } },
    { 6,   { &gPlayerAnim_clink_op3_tatiagari             } },
    { -1,  { Player_Cutscene_SetupBlownBackward           } },
    { 5,   { &gPlayerAnim_clink_demo_miokuri              } },
    { -1,  { Player_Cutscene_SetupIdle3                   } },
    { -1,  { Player_Cutscene_SetupStop                    } },
    { -1,  { Player_Cutscene_SetDraw                      } },
    { 5,   { &gPlayerAnim_clink_demo_nozoki               } },
    { 5,   { &gPlayerAnim_clink_demo_koutai               } },
    { -1,  { Player_Cutscene_SetupIdle                    } },
    { 5,   { &gPlayerAnim_clink_demo_koutai_kennuki       } },
    { 5,   { &gPlayerAnim_link_demo_kakeyori              } },
    { 5,   { &gPlayerAnim_link_demo_kakeyori_mimawasi     } },
    { 5,   { &gPlayerAnim_link_demo_kakeyori_miokuri      } },
    { 3,   { &gPlayerAnim_link_demo_furimuki2             } },
    { 3,   { &gPlayerAnim_link_demo_kaoage                } },
    { 4,   { &gPlayerAnim_link_demo_kaoage_wait           } },
    { 3,   { &gPlayerAnim_clink_demo_mimawasi             } },
    { 3,   { &gPlayerAnim_link_demo_nozokikomi            } },
    { 6,   { &gPlayerAnim_kolink_odoroki_demo             } },
    { 6,   { &gPlayerAnim_link_shagamu_demo               } },
    { 14,  { &gPlayerAnim_link_okiru_demo                 } },
    { 3,   { &gPlayerAnim_link_okiru_demo                 } },
    { 5,   { &gPlayerAnim_link_fighter_power_kiru_start   } },
    { 16,  { &gPlayerAnim_demo_link_nwait                 } },
    { 15,  { &gPlayerAnim_demo_link_tewatashi             } },
    { 15,  { &gPlayerAnim_demo_link_orosuu                } },
    { 3,   { &gPlayerAnim_d_link_orooro                   } },
    { 3,   { &gPlayerAnim_d_link_imanodare                } },
    { 3,   { &gPlayerAnim_link_hatto_demo                 } },
    { 6,   { &gPlayerAnim_o_get_mae                       } },
    { 6,   { &gPlayerAnim_o_get_ato                       } },
    { 6,   { &gPlayerAnim_om_get_mae                      } },
    { 6,   { &gPlayerAnim_nw_modoru                       } },
    { 3,   { &gPlayerAnim_link_demo_gurad                 } },
    { 3,   { &gPlayerAnim_link_demo_look_hand             } },
    { 4,   { &gPlayerAnim_link_demo_sita_wait             } },
    { 3,   { &gPlayerAnim_link_demo_ue                    } },
    { 3,   { &gPlayerAnim_Link_muku                       } },
    { 3,   { &gPlayerAnim_Link_miageru                    } },
    { 6,   { &gPlayerAnim_Link_ha                         } },
    { 3,   { &gPlayerAnim_L_1kyoro                        } },
    { 3,   { &gPlayerAnim_L_2kyoro                        } },
    { 3,   { &gPlayerAnim_L_sagaru                        } },
    { 3,   { &gPlayerAnim_L_bouzen                        } },
    { 3,   { &gPlayerAnim_L_kamaeru                       } },
    { 3,   { &gPlayerAnim_L_hajikareru                    } },
    { 3,   { &gPlayerAnim_L_ken_miru                      } },
    { 3,   { &gPlayerAnim_L_mukinaoru                     } },
    { -1,  { Player_Cutscene_SetupSpinAttackIdle          } },
    { 3,   { &gPlayerAnim_link_wait_itemD1_20f            } },
    { -1,  { Player_SetupDoNothing4                       } },
    { -1,  { Player_Cutscene_SetupKnockedToGroundDamaged  } },
    { 3,   { &gPlayerAnim_link_normal_wait_typeB_20f      } },
    { -1,  { Player_Cutscene_SetupGetSwordBack            } },
    { 3,   { &gPlayerAnim_link_demo_kousan                } },
    { 3,   { &gPlayerAnim_link_demo_return_to_past        } },
    { 3,   { &gPlayerAnim_link_last_hit_motion1           } },
    { 3,   { &gPlayerAnim_link_last_hit_motion2           } },
    { 3,   { &gPlayerAnim_link_demo_zeldamiru             } },
    { 3,   { &gPlayerAnim_link_demo_kenmiru1              } },
    { 3,   { &gPlayerAnim_link_demo_kenmiru2              } },
    { 3,   { &gPlayerAnim_link_demo_kenmiru2_modori       } },
};

static struct_80854B18 sCutsceneModeUpdateFuncs[] = {
    { 0,   { NULL                                         } },
    { -1,  { Player_Cutscene_Idle                         } },
    { -1,  { Player_Cutscene_TurnAroundSurprisedShort     } },
    { -1,  { Player_Cutscene_Unk3Update                   } },
    { -1,  { Player_Cutscene_Unk4Update                   } },
    { 11,  { NULL                                         } },
    { -1,  { Player_CutsceneUnk6Update                    } },
    { -1,  { Player_CutsceneEnd                           } },
    { -1,  { Player_Cutscene_Wait                         } },
    { -1,  { Player_Cutscene_TurnAroundSurprisedLong      } },
    { -1,  { Player_Cutscene_EnterWarp                    } },
    { -1,  { Player_Cutscene_RaisedByWarp                 } },
    { -1,  { Player_Cutscene_FightStance                  } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 18,  { sAnimSfx_GetUpFromDekuTreeStory              } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_SwordPedestal                } },
    { 12,  { &gPlayerAnim_link_demo_baru_op2              } },
    { 11,  { NULL                                         } },
    { 0,   { NULL                                         } },
    { -1,  { Player_Cutscene_WarpToSages                  } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_KnockedToGround              } },
    { 11,  { NULL                                         } },
    { 17,  { &gPlayerAnim_link_normal_okarina_swing       } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_DrawAndBrandishSword         } },
    { -1,  { Player_Cutscene_CloseEyes                    } },
    { -1,  { Player_Cutscene_OpenEyes                     } },
    { 18,  { sAnimSfx_SurprisedStumbleBackFall            } },
    { -1,  { Player_Cutscene_SurfaceFromDive              } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_Sleeping                     } },
    { -1,  { Player_Cutscene_Awaken                       } },
    { -1,  { Player_Cutscene_GetOffBed                    } },
    { -1,  { Player_Cutscene_BlownBackward                } },
    { 13,  { &gPlayerAnim_clink_demo_miokuri_wait         } },
    { -1,  { Player_Cutscene_Idle3                        } },
    { 0,   { NULL                                         } },
    { 0,   { NULL                                         } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_StepBackCautiously           } },
    { -1,  { Player_Cutscene_Wait                         } },
    { -1,  { Player_Cutscene_DrawSwordChild               } },
    { 13,  { &gPlayerAnim_link_demo_kakeyori_wait         } },
    { -1,  { Player_Cutscene_DesperateLookAtZeldasCrystal } },
    { 13,  { &gPlayerAnim_link_demo_kakeyori_miokuri_wait } },
    { -1,  { Player_Cutscene_TurnAroundSlowly             } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 12,  { &gPlayerAnim_clink_demo_mimawasi_wait        } },
    { -1,  { Player_Cutscene_InspectGroundCarefully       } },
    { 11,  { NULL                                         } },
    { 18,  { sAnimSfx_FallToKnee                          } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_StartPassOcarina             } },
    { 17,  { &gPlayerAnim_demo_link_nwait                 } },
    { 12,  { &gPlayerAnim_d_link_orowait                  } },
    { 12,  { &gPlayerAnim_demo_link_nwait                 } },
    { 11,  { NULL                                         } },
    { -1,  { Player_LearnOcarinaSong                      } },
    { 17,  { &gPlayerAnim_sude_nwait                      } },
    { -1,  { Player_LearnOcarinaSong                      } },
    { 17,  { &gPlayerAnim_sude_nwait                      } },
    { 12,  { &gPlayerAnim_link_demo_gurad_wait            } },
    { 12,  { &gPlayerAnim_link_demo_look_hand_wait        } },
    { 11,  { NULL                                         } },
    { 12,  { &gPlayerAnim_link_demo_ue_wait               } },
    { 12,  { &gPlayerAnim_Link_m_wait                     } },
    { 13,  { &gPlayerAnim_Link_ue_wait                    } },
    { 12,  { &gPlayerAnim_Link_otituku_w                  } },
    { 12,  { &gPlayerAnim_L_kw                            } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_SwordKnockedFromHand         } },
    { 11,  { NULL                                         } },
    { 12,  { &gPlayerAnim_L_kennasi_w                     } },
    { -1,  { Player_Cutscene_SpinAttackIdle               } },
    { -1,  { Player_Cutscene_InspectWeapon                } },
    { -1,  { Player_DoNothing5                            } },
    { -1,  { Player_Cutscene_KnockedToGroundDamaged       } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { 11,  { NULL                                         } },
    { -1,  { Player_Cutscene_GetSwordBack                 } },
    { -1,  { Player_Cutscene_GanonKillCombo               } },
    { -1,  { Player_Cutscene_GanonKillCombo               } },
    { 12,  { &gPlayerAnim_link_demo_zeldamiru_wait        } },
    { 12,  { &gPlayerAnim_link_demo_kenmiru1_wait         } },
    { 12,  { &gPlayerAnim_link_demo_kenmiru2_wait         } },
    { 12,  { &gPlayerAnim_demo_link_nwait                 } },
};

void Player_StopAnimAndMovement(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ClearRootLimbRotY(this);
    Player_ChangeAnimMorphToLastFrame(play, this, anim);
    Player_StopMovement(this);
}

void Player_StopAnimAndMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ClearRootLimbRotY(this);
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        anim,
        (2.0f / 3.0f),
        0.0f,
        Animation_GetLastFrame(anim),
        ANIMMODE_ONCE,
        -8.0f
    );
    Player_StopMovement(this);
}

void Player_LoopAnimAndStopMovementSlowed(PlayState* play, Player* this, LinkAnimationHeader* anim) {
    Player_ClearRootLimbRotY(this);
    LinkAnimation_Change(play, &this->skelAnime, anim, (2.0f / 3.0f), 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f);
    Player_StopMovement(this);
}

void Player_AnimPlaybackType0(PlayState* play, Player* this, void* anim) {
    Player_StopMovement(this);
}

void Player_AnimPlaybackType1(PlayState* play, Player* this, void* anim) {
    Player_StopAnimAndMovement(play, this, anim);
}

void Player_AnimPlaybackType13(PlayState* play, Player* this, void* anim) {
    Player_ClearRootLimbRotY(this);
    Player_ChangeAnimOnce(play, this, anim);
    Player_StopMovement(this);
}

void Player_AnimPlaybackType2(PlayState* play, Player* this, void* anim) {
    Player_StopAnimAndMovementSlowed(play, this, anim);
}

void Player_AnimPlaybackType3(PlayState* play, Player* this, void* anim) {
    Player_LoopAnimAndStopMovementSlowed(play, this, anim);
}

void Player_AnimPlaybackType4(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnceWithMovementPresetFlagsSlowed(play, this, anim);
}

void Player_AnimPlaybackType5(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnceWithMovement(play, this, anim, 0x9C);
}

void Player_AnimPlaybackType6(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoopWithMovementPresetFlagsSlowed(play, this, anim);
}

void Player_AnimPlaybackType7(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoopWithMovement(play, this, anim, 0x9C);
}

void Player_AnimPlaybackType8(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnce(play, this, anim);
}

void Player_AnimPlaybackType9(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoop(play, this, anim);
}

void Player_AnimPlaybackType14(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimOnceSlowed(play, this, anim);
}

void Player_AnimPlaybackType15(PlayState* play, Player* this, void* anim) {
    Player_PlayAnimLoopSlowed(play, this, anim);
}

void Player_AnimPlaybackType10(PlayState* play, Player* this, void* anim) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_AnimPlaybackType11(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_LoopAnimAndStopMovementSlowed(play, this, anim);
        this->unk_850 = 1;
    }
}

void Player_AnimPlaybackType16(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_EndAnimMovement(this);
        Player_PlayAnimLoopSlowed(play, this, anim);
    }
}

void Player_AnimPlaybackType12(PlayState* play, Player* this, void* anim) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovementPresetFlagsSlowed(play, this, anim);
        this->unk_850 = 1;
    }
}

void Player_AnimPlaybackType17(PlayState* play, Player* this, void* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, arg2);
}

void Player_LookAtCutsceneTargetActor(Player* this) {
    if ((this->unk_448 == NULL) || (this->unk_448->update == NULL)) {
        this->unk_448 = NULL;
    }
    
    this->unk_664 = this->unk_448;
    
    if (this->unk_664 != NULL) {
        this->actor.shape.rot.y = Player_LookAtTargetActor(this, 0);
    }
}

void Player_Cutscene_SetupSwimIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->stateFlags1 |= PLAYER_STATE1_27;
    this->stateFlags2 |= PLAYER_STATE2_10;
    this->stateFlags1 &= ~(PLAYER_STATE1_18 | PLAYER_STATE1_19);
    
    Player_PlayAnimLoop(play, this, &gPlayerAnim_link_swimer_swim);
}

void Player_Cutscene_SurfaceFromDive(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.gravity = 0.0f;
    
    if (this->unk_84F == 0) {
        if (Player_SetupDive(play, this, NULL)) {
            this->unk_84F = 1;
        } else {
            Player_PlaySwimAnim(play, this, NULL, fabsf(this->actor.velocity.y));
            Math_ScaledStepToS(&this->unk_6C2, -10000, 800);
            Player_UpdateSwimMovement(this, &this->actor.velocity.y, 4.0f, this->currentYaw);
        }
        
        return;
    }
    
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        if (this->unk_84F == 1) {
            Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
        } else {
            Player_PlayAnimLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
        }
    }
    
    Player_SetVerticalWaterVelocity(this);
    Player_UpdateSwimMovement(this, &this->linearVelocity, 0.0f, this->actor.shape.rot.y);
}

void Player_Cutscene_Idle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_LookAtCutsceneTargetActor(this);
    
    if (Player_IsSwimming(this)) {
        Player_Cutscene_SurfaceFromDive(play, this, 0);
        
        return;
    }
    
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (func_8008F128(this) || (this->stateFlags1 & PLAYER_STATE1_11)) {
        Player_SetupCurrentUpperAction(this, play);
        
        return;
    }
    
    if ((this->interactRangeActor != NULL) && (this->interactRangeActor->textId == 0xFFFF)) {
        Player_SetupGetItemOrHoldBehavior(this, play);
    }
}

void Player_Cutscene_TurnAroundSurprisedShort(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Cutscene_SetupIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimationHeader* anim;
    
    if (Player_IsSwimming(this)) {
        Player_Cutscene_SetupSwimIdle(play, this, 0);
        
        return;
    }
    
    anim = GET_PLAYER_ANIM(PLAYER_ANIMGROUP_44, this->modelAnimType);
    
    if ((this->unk_446 == 6) || (this->unk_446 == 0x2E)) {
        Player_PlayAnimOnce(play, this, anim);
    } else {
        Player_ClearRootLimbRotY(this);
        LinkAnimation_Change(
            play,
            &this->skelAnime,
            anim,
            (2.0f / 3.0f),
            0.0f,
            Animation_GetLastFrame(anim),
            ANIMMODE_LOOP,
            -4.0f
        );
    }
    
    Player_StopMovement(this);
}

void Player_Cutscene_Wait(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (Player_SetupShootingGalleryPlay(play, this) == 0) {
        if ((this->csMode == 0x31) && (play->csCtx.state == CS_STATE_IDLE)) {
            func_8002DF54(play, NULL, 7);
            
            return;
        }
        
        if (Player_IsSwimming(this) != 0) {
            Player_Cutscene_SurfaceFromDive(play, this, 0);
            
            return;
        }
        
        LinkAnimation_Update(play, &this->skelAnime);
        
        if (func_8008F128(this) || (this->stateFlags1 & PLAYER_STATE1_11)) {
            Player_SetupCurrentUpperAction(this, play);
        }
    }
}

static struct_80832924 sAnimSfx_TurnAroundSurprised[] = {
    { 0, 0x302A  },
    { 0, -0x3030 },
};

void Player_Cutscene_TurnAroundSurprisedLong(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sAnimSfx_TurnAroundSurprised);
}

void Player_Cutscene_SetupEnterWarp(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->stateFlags1 &= ~PLAYER_STATE1_25;
    
    this->currentYaw = this->actor.shape.rot.y = this->actor.world.rot.y =
        Math_Vec3f_Yaw(&this->actor.world.pos, &this->unk_450);
    
    if (this->linearVelocity <= 0.0f) {
        this->linearVelocity = 0.1f;
    } else if (this->linearVelocity > 2.5f) {
        this->linearVelocity = 2.5f;
    }
}

void Player_Cutscene_EnterWarp(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    f32 sp1C = 2.5f;
    
    Player_CutsceneMoveUsingActionPosIntoRange(play, this, &sp1C, 10);
    
    if (play->sceneId == SCENE_BDAN_BOSS) {
        if (this->unk_850 == 0) {
            if (Message_GetState(&play->msgCtx) == TEXT_STATE_NONE) {
                return;
            }
        } else {
            if (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) {
                return;
            }
        }
    }
    
    this->unk_850++;
    if (this->unk_850 > 20) {
        this->csMode = 0xB;
    }
}

void Player_Cutscene_SetupFightStance(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_SetupUnfriendlyZTarget(this, play);
}

void Player_Cutscene_FightStance(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_LookAtCutsceneTargetActor(this);
    
    if (this->unk_850 != 0) {
        if (LinkAnimation_Update(play, &this->skelAnime)) {
            Player_PlayAnimLoop(play, this, Player_GetAnim_FightRight(this));
            this->unk_850 = 0;
        }
        
        Player_ResetLeftRightBlendWeight(this);
    } else {
        func_808401B0(play, this);
    }
}

void Player_Cutscene_Unk3Update(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_CutsceneMoveUsingActionPos(play, this, arg2, 0.0f, 0, 0);
}

void Player_Cutscene_Unk4Update(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_CutsceneMoveUsingActionPos(play, this, arg2, 0.0f, 0, 1);
}

static Vec3f sStartTimeTravelPos = { -1.0f, 70.0f, 20.0f };

void Player_Cutscene_SetupSwordPedestal(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Math_Vec3f_Copy(&this->actor.world.pos, &sStartTimeTravelPos);
    this->actor.shape.rot.y = -0x8000;
    Player_PlayAnimOnceSlowed(play, this, this->ageProperties->unk_9C);
    Player_SetupAnimMovement(play, this, 0x28F);
}

static struct_808551A4 sSfxID_SwordPedestal[] = {
    { NA_SE_IT_SWORD_PUTAWAY_STN, 0                     },
    { NA_SE_IT_SWORD_STICK_STN,   NA_SE_VO_LI_SWORD_N   },
};

static struct_80832924 sSfxID_StepOntoPedestal[] = {
    { 0, 0x401D  },
    { 0, -0x4027 },
};

void Player_Cutscene_SwordPedestal(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    struct_808551A4* sp2C;
    Gfx** dLists;
    
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (
        (LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 70.0f)) ||
        (!LINK_IS_ADULT && LinkAnimation_OnFrame(&this->skelAnime, 87.0f))
    ) {
        sp2C = &sSfxID_SwordPedestal[gSaveContext.linkAge];
        this->interactRangeActor->parent = &this->actor;
        
        if (!LINK_IS_ADULT) {
            dLists = gPlayerLeftHandBgsDLs;
        } else {
            dLists = gPlayerLeftHandClosedDLs;
        }
        this->leftHandDLists = dLists + gSaveContext.linkAge;
        
        func_8002F7DC(&this->actor, sp2C->unk_00);
        if (!LINK_IS_ADULT) {
            Player_PlayVoiceSfxForAge(this, sp2C->unk_02);
        }
    } else if (LINK_IS_ADULT) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 66.0f)) {
            Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_SWORD_L);
        }
    } else {
        Player_PlayAnimSfx(this, sSfxID_StepOntoPedestal);
    }
}

void Player_Cutscene_SetupWarpToSages(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Change(
        play,
        &this->skelAnime,
        &gPlayerAnim_link_demo_warp,
        -(2.0f / 3.0f),
        12.0f,
        12.0f,
        ANIMMODE_ONCE,
        0.0f
    );
}

static struct_80832924 sAnimSfx_WarpToSages[] = {
    { 0, -0x281E },
};

void Player_Cutscene_WarpToSages(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    
    this->unk_850++;
    
    if (this->unk_850 >= 180) {
        if (this->unk_850 == 180) {
            LinkAnimation_Change(
                play,
                &this->skelAnime,
                &gPlayerAnim_link_okarina_warp_goal,
                (2.0f / 3.0f),
                10.0f,
                Animation_GetLastFrame(&gPlayerAnim_link_okarina_warp_goal),
                ANIMMODE_ONCE,
                -8.0f
            );
        }
        Player_PlayAnimSfx(this, sAnimSfx_WarpToSages);
    }
}

void Player_Cutscene_KnockedToGround(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (
        LinkAnimation_Update(play, &this->skelAnime) && (this->unk_850 == 0) &&
        (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
    ) {
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_back_downB);
        this->unk_850 = 1;
    }
    
    if (this->unk_850 != 0) {
        Player_StepLinearVelToZero(this);
    }
}

void Player_Cutscene_SetupStartPlayOcarina(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_StopAnimAndMovementSlowed(play, this, &gPlayerAnim_link_normal_okarina_start);
    Player_SetOcarinaItemAP(this);
    Player_SetModels(this, Player_ActionToModelGroup(this, this->itemActionParam));
}

static struct_80832924 sAnimSfx_DrawAndBrandishSword[] = {
    { NA_SE_IT_SWORD_PICKOUT, -0x80C },
};

void Player_Cutscene_DrawAndBrandishSword(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 6.0f)) {
        Player_CutsceneDrawSword(play, this, 0);
    } else {
        Player_PlayAnimSfx(this, sAnimSfx_DrawAndBrandishSword);
    }
}

void Player_Cutscene_CloseEyes(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 0, 1);
}

void Player_Cutscene_OpenEyes(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Math_StepToS(&this->actor.shape.face, 2, 1);
}

void Player_Cutscene_SetupGetItemInWater(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, &gPlayerAnim_link_swimer_swim_get, 0x98);
}

void Player_Cutscene_SetupSleeping(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlayAnimOnceWithMovement(play, this, &gPlayerAnim_clink_op3_negaeri, 0x9C);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_GROAN);
}

void Player_Cutscene_Sleeping(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovement(play, this, &gPlayerAnim_clink_op3_wait2, 0x9C);
    }
}

void Player_PlaySlowLoopedCutsceneAnimWithSfx(PlayState* play, Player* this, LinkAnimationHeader* anim, struct_80832924* arg3) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, anim);
        this->unk_850 = 1;
    } else if (this->unk_850 == 0) {
        Player_PlayAnimSfx(this, arg3);
    }
}

void Player_Cutscene_SetupSleepingRestless(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.shape.shadowDraw = NULL;
    Player_AnimPlaybackType7(play, this, &gPlayerAnim_clink_op3_wait1);
}

static struct_80832924 sAnimSfx_Awaken[] = {
    { NA_SE_VO_LI_RELAX, 0x2023 },
    { NA_SE_PL_SLIPDOWN, 0x8EC  },
    { NA_SE_PL_SLIPDOWN, -0x900 },
};

void Player_Cutscene_Awaken(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovement(play, this, &gPlayerAnim_clink_op3_wait3, 0x9C);
        this->unk_850 = 1;
    } else if (this->unk_850 == 0) {
        Player_PlayAnimSfx(this, sAnimSfx_Awaken);
        if (LinkAnimation_OnFrame(&this->skelAnime, 240.0f)) {
            this->actor.shape.shadowDraw = ActorShadow_DrawFeet;
        }
    }
}

static struct_80832924 sAnimSfx_GetOffBed[] = {
    { NA_SE_PL_LAND_LADDER, 0x843                      },
    { 0,                    0x4854                     },
    { 0,                    0x485A                     },
    { 0,                    -0x4860                    },
};

void Player_Cutscene_GetOffBed(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sAnimSfx_GetOffBed);
}

void Player_Cutscene_SetupBlownBackward(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlayAnimOnceWithMovementSlowed(play, this, &gPlayerAnim_clink_demo_futtobi, 0x9D);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

void Player_GetCsPositionByActionLength(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    f32 startX = arg2->startPos.x;
    f32 startY = arg2->startPos.y;
    f32 startZ = arg2->startPos.z;
    f32 distX = (arg2->endPos.x - startX);
    f32 distY = (arg2->endPos.y - startY);
    f32 distZ = (arg2->endPos.z - startZ);
    f32 sp4 = (f32)(play->csCtx.frames - arg2->startFrame) / (f32)(arg2->endFrame - arg2->startFrame);
    
    this->actor.world.pos.x = distX * sp4 + startX;
    this->actor.world.pos.y = distY * sp4 + startY;
    this->actor.world.pos.z = distZ * sp4 + startZ;
}

static struct_80832924 sAnimSfx_BlownBackward[] = {
    { NA_SE_PL_BOUND, 0x1014  },
    { NA_SE_PL_BOUND, -0x101E },
};

void Player_Cutscene_BlownBackward(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_GetCsPositionByActionLength(play, this, arg2);
    LinkAnimation_Update(play, &this->skelAnime);
    Player_PlayAnimSfx(this, sAnimSfx_BlownBackward);
}

void Player_Cutscene_RaisedByWarp(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (arg2 != NULL) {
        Player_GetCsPositionByActionLength(play, this, arg2);
    }
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Cutscene_SetupIdle3(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_ChangeAnimMorphToLastFrame(play, this, GET_PLAYER_ANIM(PLAYER_ANIMGROUP_44, this->modelAnimType));
    Player_StopMovement(this);
}

void Player_Cutscene_Idle3(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
}

void Player_Cutscene_SetupStop(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_SetupAnimMovement(play, this, 0x98);
}

void Player_Cutscene_SetDraw(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.draw = Player_Draw;
}

void Player_Cutscene_DrawSwordChild(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopWithMovementPresetFlagsSlowed(play, this, &gPlayerAnim_clink_demo_koutai_wait);
        this->unk_850 = 1;
    } else if (this->unk_850 == 0) {
        if (LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
            Player_CutsceneDrawSword(play, this, 1);
        }
    }
}

static struct_80832924 sAnimSfx_TurnAroundSlowly[] = {
    { 0, 0x300A  },
    { 0, -0x3018 },
};

void Player_Cutscene_TurnAroundSlowly(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlaySlowLoopedCutsceneAnimWithSfx(play, this, &gPlayerAnim_link_demo_furimuki2_wait, sAnimSfx_TurnAroundSlowly);
}

static struct_80832924 sAnimSfx_InspectGroundCarefully[] = {
    { 0, 0x400F  },
    { 0, -0x4023 },
};

void Player_Cutscene_InspectGroundCarefully(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_PlaySlowLoopedCutsceneAnimWithSfx(play, this, &gPlayerAnim_link_demo_nozokikomi_wait, sAnimSfx_InspectGroundCarefully);
}

void Player_Cutscene_StartPassOcarina(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_PlayAnimLoopSlowed(play, this, &gPlayerAnim_demo_link_twait);
        this->unk_850 = 1;
    }
    
    if ((this->unk_850 != 0) && (play->csCtx.frames >= 900)) {
        this->rightHandType = PLAYER_MODELTYPE_LH_OPEN;
    } else {
        this->rightHandType = PLAYER_MODELTYPE_RH_FF;
    }
}

void Player_AnimPlaybackType12PlaySfx(PlayState* play, Player* this, LinkAnimationHeader* anim, struct_80832924* arg3) {
    Player_AnimPlaybackType12(play, this, anim);
    if (this->unk_850 == 0) {
        Player_PlayAnimSfx(this, arg3);
    }
}

static struct_80832924 sAnimSfx_StepBackCautiously[] = {
    { 0, 0x300F  },
    { 0, -0x3021 },
};

void Player_Cutscene_StepBackCautiously(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_AnimPlaybackType12PlaySfx(play, this, &gPlayerAnim_clink_demo_koutai_wait, sAnimSfx_StepBackCautiously);
}

static struct_80832924 sAnimSfx_DesperateLookAtZeldaCrystal[] = {
    { NA_SE_PL_KNOCK, -0x84E },
};

void Player_Cutscene_DesperateLookAtZeldasCrystal(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_AnimPlaybackType12PlaySfx(play, this, &gPlayerAnim_link_demo_kakeyori_wait, sAnimSfx_DesperateLookAtZeldaCrystal);
}

void Player_Cutscene_SetupSpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_SetupSpinAttackAnims(play, this);
}

void Player_Cutscene_SpinAttackIdle(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    sControlInput->press.button |= BTN_B;
    
    Player_ChargeSpinAttack(this, play);
}

void Player_Cutscene_InspectWeapon(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_ChargeSpinAttack(this, play);
}

void Player_SetupDoNothing4(PlayState* play, Player* this, CsCmdActorAction* arg2) {
}

void Player_DoNothing5(PlayState* play, Player* this, CsCmdActorAction* arg2) {
}

void Player_Cutscene_SetupKnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->stateFlags3 |= PLAYER_STATE3_1;
    this->linearVelocity = 2.0f;
    this->actor.velocity.y = -1.0f;
    
    Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_back_downA);
    Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_FALL_L);
}

static void (*sCsKnockedToGroundDamagedFuncs[])(Player* this, PlayState* play) = {
    Player_StartKnockback,
    Player_DownFromKnockback,
    Player_GetUpFromKnockback,
};

void Player_Cutscene_KnockedToGroundDamaged(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    sCsKnockedToGroundDamagedFuncs[this->unk_850](this, play);
}

void Player_Cutscene_SetupGetSwordBack(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    Player_CutsceneDrawSword(play, this, 0);
    Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_demo_return_to_past);
}

void Player_Cutscene_SwordKnockedFromHand(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    LinkAnimation_Update(play, &this->skelAnime);
    
    if (LinkAnimation_OnFrame(&this->skelAnime, 10.0f)) {
        this->heldItemActionParam = this->itemActionParam = PLAYER_AP_NONE;
        this->heldItemId = ITEM_NONE;
        this->modelGroup = this->nextModelGroup = Player_ActionToModelGroup(this, PLAYER_AP_NONE);
        this->leftHandDLists = gPlayerLeftHandOpenDLs;
        Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
        gSaveContext.equips.buttonItems[0] = ITEM_SWORD_MASTER;
        Inventory_DeleteEquipment(play, EQUIP_TYPE_SWORD);
    }
}

static LinkAnimationHeader* sAnims_LearnOcarinaSong[] = {
    &gPlayerAnim_L_okarina_get,
    &gPlayerAnim_om_get,
};

static Vec3s sBaseSparklePos[2][2] = {
    { { -200, 700, 100 }, { 800, 600, 800 } },
    { { -200, 500, 0   }, { 600, 400, 600 } },
};

void Player_LearnOcarinaSong(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    static Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    static Color_RGBA8 primColor = { 255, 255, 255, 0 };
    static Color_RGBA8 envColor = { 0, 128, 128, 0 };
    s32 linkAge = gSaveContext.linkAge;
    Vec3f sparklePos;
    Vec3f sp34;
    Vec3s* ptr;
    
    Player_AnimPlaybackType12(play, this, sAnims_LearnOcarinaSong[linkAge]);
    
    if (this->rightHandType != PLAYER_MODELTYPE_RH_FF) {
        this->rightHandType = PLAYER_MODELTYPE_RH_FF;
        
        return;
    }
    
    ptr = sBaseSparklePos[gSaveContext.linkAge];
    
    sp34.x = ptr[0].x + Rand_CenteredFloat(ptr[1].x);
    sp34.y = ptr[0].y + Rand_CenteredFloat(ptr[1].y);
    sp34.z = ptr[0].z + Rand_CenteredFloat(ptr[1].z);
    
    SkinMatrix_Vec3fMtxFMultXYZ(&this->shieldMf, &sp34, &sparklePos);
    
    EffectSsKiraKira_SpawnDispersed(play, &sparklePos, &zeroVec, &zeroVec, &primColor, &envColor, 600, -10);
}

void Player_Cutscene_GetSwordBack(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_CutsceneEnd(play, this, arg2);
    } else if (this->unk_850 == 0) {
        Item_Give(play, ITEM_SWORD_MASTER);
        Player_CutsceneDrawSword(play, this, 0);
    } else {
        Player_PlaySwingSwordSfx(this);
    }
}

void Player_Cutscene_GanonKillCombo(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (LinkAnimation_Update(play, &this->skelAnime)) {
        Player_SetupMeleeAttack(this, 0.0f, 99.0f, this->skelAnime.endFrame - 8.0f);
    }
    
    if (this->heldItemActionParam != PLAYER_AP_SWORD_MASTER) {
        Player_CutsceneDrawSword(play, this, 1);
    }
}

void Player_CutsceneEnd(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    if (Player_IsSwimming(this)) {
        Player_SetupSwimIdle(play, this);
        Player_ResetSubCam(play, this);
    } else {
        Player_ClearLookAndAttention(this, play);
        if (!Player_SetupSpeakOrCheck(this, play)) {
            Player_SetupGetItemOrHoldBehavior(this, play);
        }
    }
    
    this->csMode = 0;
    this->unk_6AD = 0;
}

void Player_CutsceneSetPosAndYaw(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    this->actor.world.pos.x = arg2->startPos.x;
    this->actor.world.pos.y = arg2->startPos.y;
    if ((play->sceneId == SCENE_SPOT04) && !LINK_IS_ADULT) {
        this->actor.world.pos.y -= 1.0f;
    }
    this->actor.world.pos.z = arg2->startPos.z;
    this->currentYaw = this->actor.shape.rot.y = arg2->rot.y;
}

void Player_CutsceneSetPosAndYawIfOutsideStartRange(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    f32 dx = arg2->startPos.x - (s32)this->actor.world.pos.x;
    f32 dy = arg2->startPos.y - (s32)this->actor.world.pos.y;
    f32 dz = arg2->startPos.z - (s32)this->actor.world.pos.z;
    f32 dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
    s16 yawDiff = arg2->rot.y - this->actor.shape.rot.y;
    
    if ((this->linearVelocity == 0.0f) && ((dist > 50.0f) || (ABS(yawDiff) > 0x4000))) {
        Player_CutsceneSetPosAndYaw(play, this, arg2);
    }
    
    this->skelAnime.moveFlags = 0;
    Player_ClearRootLimbRotY(this);
}

void Player_CsModePlayback(PlayState* play, Player* this, CsCmdActorAction* arg2, struct_80854B18* arg3) {
    if (arg3->type > 0) {
        sCutsceneModePlaybackFunc[arg3->type](play, this, arg3->ptr);
    } else if (arg3->type < 0) {
        arg3->func(play, this, arg2);
    }
    
    if ((sPrevSkelAnimeMoveFlags & 4) && !(this->skelAnime.moveFlags & 4)) {
        this->skelAnime.morphTable[0].y /= this->ageProperties->unk_08;
        sPrevSkelAnimeMoveFlags = 0;
    }
}

void Player_CutsceneDetatchHeldActor(PlayState* play, Player* this, s32 csMode) {
    if ((csMode != 1) && (csMode != 8) && (csMode != 0x31) && (csMode != 7)) {
        Player_DetatchHeldActor(play, this);
    }
}

void Player_CutsceneUnk6Update(PlayState* play, Player* this, CsCmdActorAction* arg2) {
    CsCmdActorAction* linkCsAction = play->csCtx.linkAction;
    s32 sp24;
    
    if (play->csCtx.state == CS_STATE_UNSKIPPABLE_INIT) {
        func_8002DF54(play, NULL, 7);
        this->unk_446 = 0;
        Player_StopMovement(this);
        
        return;
    }
    
    if (linkCsAction == NULL) {
        this->actor.flags &= ~ACTOR_FLAG_6;
        
        return;
    }
    
    if (this->unk_446 != linkCsAction->action) {
        sp24 = sLinkActionCsCmds[linkCsAction->action];
        if (sp24 >= 0) {
            if ((sp24 == 3) || (sp24 == 4)) {
                Player_CutsceneSetPosAndYawIfOutsideStartRange(play, this, linkCsAction);
            } else {
                Player_CutsceneSetPosAndYaw(play, this, linkCsAction);
            }
        }
        
        sPrevSkelAnimeMoveFlags = this->skelAnime.moveFlags;
        
        Player_EndAnimMovement(this);
        osSyncPrintf("TOOL MODE=%d\n", sp24);
        Player_CutsceneDetatchHeldActor(play, this, ABS(sp24));
        Player_CsModePlayback(play, this, linkCsAction, &sCutsceneModeInitFuncs[ABS(sp24)]);
        
        this->unk_850 = 0;
        this->unk_84F = 0;
        this->unk_446 = linkCsAction->action;
    }
    
    sp24 = sLinkActionCsCmds[this->unk_446];
    Player_CsModePlayback(play, this, linkCsAction, &sCutsceneModeUpdateFuncs[ABS(sp24)]);
}

void Player_StartCutscene(Player* this, PlayState* play) {
    if (this->csMode != this->prevCsMode) {
        sPrevSkelAnimeMoveFlags = this->skelAnime.moveFlags;
        
        Player_EndAnimMovement(this);
        this->prevCsMode = this->csMode;
        osSyncPrintf("DEMO MODE=%d\n", this->csMode);
        Player_CutsceneDetatchHeldActor(play, this, this->csMode);
        Player_CsModePlayback(play, this, NULL, &sCutsceneModeInitFuncs[this->csMode]);
    }
    
    Player_CsModePlayback(play, this, NULL, &sCutsceneModeUpdateFuncs[this->csMode]);
}

s32 Player_IsDroppingFish(PlayState* play) {
    Player* this = GET_PLAYER(play);
    
    return (Player_DropItemFromBottle == this->func_674) && (this->itemActionParam == PLAYER_AP_BOTTLE_FISH);
}

s32 Player_StartFishing(PlayState* play) {
    Player* this = GET_PLAYER(play);
    
    Player_ResetAttributesAndHeldActor(play, this);
    Player_UseItem(play, this, ITEM_FISHING_POLE);
    
    return 1;
}

s32 Player_SetupRestrainedByEnemy(PlayState* play, Player* this) {
    if (
        !Player_InBlockingCsMode(play, this) && (this->invincibilityTimer >= 0) && !func_8008F128(this) &&
        !(this->stateFlags3 & PLAYER_STATE3_7)
    ) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetActionFunc(play, this, Player_RestrainedByEnemy, 0);
        Player_PlayAnimOnce(play, this, &gPlayerAnim_link_normal_re_dead_attack);
        this->stateFlags2 |= PLAYER_STATE2_7;
        Player_ClearAttentionModeAndStopMoving(this);
        Player_PlayVoiceSfxForAge(this, NA_SE_VO_LI_HELD);
        
        return true;
    }
    
    return false;
}

// Sets up player cutscene
s32 Player_SetupPlayerCutscene(PlayState* play, Actor* actor, s32 csMode) {
    Player* this = GET_PLAYER(play);
    
    if (!Player_InBlockingCsMode(play, this)) {
        Player_ResetAttributesAndHeldActor(play, this);
        Player_SetActionFunc(play, this, Player_StartCutscene, 0);
        this->csMode = csMode;
        this->unk_448 = actor;
        Player_ClearAttentionModeAndStopMoving(this);
        
        return 1;
    }
    
    return 0;
}

void Player_SetupStandingStillMorph(Player* this, PlayState* play) {
    Player_SetActionFunc(play, this, Player_StandingStill, 1);
    Player_ChangeAnimMorphToLastFrame(play, this, Player_GetAnim_StandingStill(this));
    this->currentYaw = this->actor.shape.rot.y;
}

s32 Player_InflictDamage(PlayState* play, s32 damage) {
    Player* this = GET_PLAYER(play);
    
    if (!Player_InBlockingCsMode(play, this) && !Player_ApplyDamage(play, this, damage)) {
        this->stateFlags2 &= ~PLAYER_STATE2_7;
        
        return 1;
    }
    
    return 0;
}

// Start talking with the given actor
void Player_StartTalkingWithActor(PlayState* play, Actor* actor) {
    Player* this = GET_PLAYER(play);
    
    if (
        (this->targetActor != NULL) || (actor == this->naviActor) ||
        CHECK_FLAG_ALL(actor->flags, ACTOR_FLAG_0 | ACTOR_FLAG_18)
    ) {
        actor->flags |= ACTOR_FLAG_8;
    }
    
    this->targetActor = actor;
    this->exchangeItemId = EXCH_ITEM_NONE;
    
    if (actor->textId == 0xFFFF) {
        func_8002DF54(play, actor, 1);
        actor->flags |= ACTOR_FLAG_8;
        Player_UnequipItem(play, this);
    } else {
        if (this->actor.flags & ACTOR_FLAG_8) {
            this->actor.textId = 0;
        } else {
            this->actor.flags |= ACTOR_FLAG_8;
            this->actor.textId = actor->textId;
        }
        
        if (this->stateFlags1 & PLAYER_STATE1_23) {
            s32 sp24 = this->unk_850;
            
            Player_UnequipItem(play, this);
            Player_SetupTalkWithActor(play, this);
            
            this->unk_850 = sp24;
        } else {
            if (Player_IsSwimming(this)) {
                Player_SetupCsActionFunc(play, this, Player_SetupTalkWithActor);
                Player_ChangeAnimLongMorphLoop(play, this, &gPlayerAnim_link_swimer_swim_wait);
            } else if ((actor->category != ACTORCAT_NPC) || (this->heldItemActionParam == PLAYER_AP_FISHING_POLE)) {
                Player_SetupTalkWithActor(play, this);
                
                if (!func_8008E9C4(this)) {
                    if ((actor != this->naviActor) && (actor->xzDistToPlayer < 40.0f)) {
                        Player_PlayAnimOnceSlowed(play, this, &gPlayerAnim_link_normal_backspace);
                    } else {
                        Player_PlayAnimLoop(play, this, Player_GetAnim_StandingStill(this));
                    }
                }
            } else {
                Player_SetupCsActionFunc(play, this, Player_SetupTalkWithActor);
                Player_PlayAnimOnceSlowed(play, this, (actor->xzDistToPlayer < 40.0f) ? &gPlayerAnim_link_normal_backspace : &gPlayerAnim_link_normal_talk_free);
            }
            
            if (this->skelAnime.animation == &gPlayerAnim_link_normal_backspace) {
                Player_SetupAnimMovement(play, this, 0x19);
            }
            
            Player_ClearAttentionModeAndStopMoving(this);
        }
        
        this->stateFlags1 |= PLAYER_STATE1_6 | PLAYER_STATE1_29;
    }
    
    if ((this->naviActor == this->targetActor) && ((this->targetActor->textId & 0xFF00) != 0x200)) {
        this->naviActor->flags |= ACTOR_FLAG_8;
        Player_SetCameraTurnAround(play, 0xB);
    }
}
