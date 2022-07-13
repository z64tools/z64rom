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

typedef struct {
	Vec3f pos, prevPos;
	f32   mass;
} Particle;

typedef struct {
	Particle* p1;
	Particle* p2;
	f32 length;
} Chain;

void Audio_PlaySys(u16 flag);
Color_HSL Color_RgbToHsl(f32 r, f32 g, f32 b);
Color_RGB8 Color_HslToRgb(f32 h, f32 s, f32 l);

void Cutscene_ProcessCmds(PlayState* play, CutsceneContext* csCtx, u8* cutscenePtr);
void* CutsceneCmd_ExitParam(PlayState* play, void* ptr);

Vtx* Gui_AllocQuad(PlayState* playState, s16 x, s16 y, s16 width, s16 height, s16 u, s16 v);

s32 Lights_SortLightList(PlayState* play, LightInfo* sortedLightList[7]);
void Lights_SetPointlight(PlayState* play, Lights* lights, LightParams* params, bool isWiiVC);
void Lights_RebindActor(PlayState* play, Actor* actor, Vec3f* bindPos);
void Lights_RebindPointlightsActor(PlayState* play, Actor* actor, bool isWiiVC);

void Matrix_RotateX_s(s16 binang, MatrixMode mtxMode);
void Matrix_RotateY_s(s16 binang, MatrixMode mtxMode);
void Matrix_RotateZ_s(s16 binang, MatrixMode mtxMode);
void Matrix_RotateX_f(f32 binang, MatrixMode mtxMode);
void Matrix_RotateY_f(f32 binang, MatrixMode mtxMode);
void Matrix_RotateZ_f(f32 binang, MatrixMode mtxMode);
void Matrix_MultX(f32 x, Vec3f* dst);
void Matrix_MultY(f32 y, Vec3f* dst);
void Matrix_MultZ(f32 z, Vec3f* dst);

f32 MaxF(f32 a, f32 b);
f32 MinF(f32 a, f32 b);
s32 MaxS(s32 a, s32 b);
s32 MinS(s32 a, s32 b);
s32 WrapS(s32 x, s32 min, s32 max);
f32 WrapF(f32 x, f32 min, f32 max);
f32 Math_Spline1(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
f32 Math_Spline2(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
Vec3f Math_Vec3f_Spline1(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2);
Vec3f Math_Vec3f_Spline2(f32 k, Vec3f xm1, Vec3f x0, Vec3f x1, Vec3f x2);
Vec3f Math_Vec3f_YawDist(f32 dist, s16 yaw);
Vec3f Math_Vec3f_YawPitchDist(f32 dist, s16 yaw, s16 pitch);
Vec3f Math_Vec3f_PosRelativeTo(Vec3f* target, Vec3f* origin, s16 originYaw);
f32 Math_Vec3f_Length(Vec3f* a);
f32 PowF(f32 a, f32 b);

Particle Particle_New(Vec3f pos, f32 mass);
void Particle_Update(Particle* particle, f32 gravity, f32 wind, s16 windYaw, f32 delta);
Chain Chain_New(Particle* p1, Particle* p2, f32 length);
void Chain_Update(Chain* chain);

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
