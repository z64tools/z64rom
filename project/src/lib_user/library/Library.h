#include <oot_mq_debug/z64hdr.h>

typedef struct {
	f32 h;
	f32 s;
	f32 l;
} Color_HSL;

typedef struct {
	f32 h;
	f32 s;
	f32 l;
	u8  alpha;
} Color_HSLA;

void Audio_PlaySys(u16 flag);
void Color_ToHSL(Color_HSL* dest, Color_RGB8* src);
void Color_ToRGB(Color_RGB8* dest, Color_HSL* src);

#ifdef DEV_BUILD
void Debug_Text(u8 r, u8 g, u8 b, s32 x, s32 y, char* fmt, ...);
void Debug_DmaLog(DmaRequest* req);
void DebugMenu_Update(PlayState* playState);
s32 DebugMenu_CineCamera(Camera* camera, Normal1* norm1, Player* player);
#else
#define Debug_Text(...)           ((void)0)
#define Debug_DmaLog(...)         ((void)0)
#define DebugMenu_Update(...)     ((void)0)
#define DebugMenu_CineCamera(...) 0
#endif

Vtx* Gui_AllocQuad(PlayState* playState, s16 x, s16 y, s16 width, s16 height, s16 u, s16 v);

s32 Lights_SortLightList(PlayState* play, LightInfo* sortedLightList[7]);
void Lights_SetPointlight(PlayState* play, Lights* lights, LightParams* params, bool isWiiVC);
void Lights_RebindActor(PlayState* play, Actor* actor, Vec3f* bindPos);
void Lights_RebindPointlightsActor(PlayState* play, Actor* actor, bool isWiiVC);

f32 MaxF(f32 a, f32 b);
f32 MinF(f32 a, f32 b);
s16 MaxS(s16 a, s16 b);
s16 MinS(s16 a, s16 b);
f32 Math_Spline1(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
f32 Math_Spline2(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);

void* Segment_Scene_GetHeader(void* segment, s32 setupIndex);
void* Segment_Scene_GetCutscene(void* segment, s32 setupIndex);
void* Segment_Scene_GetPathList(void* segment, s32 setupIndex);
void* Segment_Scene_GetPath(u32* pathListAddr, s16 pathID);
u8 Segment_Scene_GetPathNodeNum(u32* pathAddr);
Vec3f* Segment_Scene_GetPathVec3f(u32* pathListAddr, s16 pathID, s16 nodeID);
CollisionHeader* Segment_Scene_GetCollision(void* segment, s32 setupIndex);
void Segment_Scene_PlayCutscene(void* segment, s32 setupIndex);

void SceneAnim_Update(PlayState* playState);

void SkelAnime_InterpFrameTable_Quat(s32 limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, f32 weight);