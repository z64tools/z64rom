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

typedef void (* PhysicCallback)(s32 limbIndex, void*, void*);

typedef struct {
	Vec3f pos;
	Vec3f rot;
	Vec3f vel;
} PhysicsLimb;

typedef struct {
	s32 numLimbs;
	f32 gravity;
	f32 floorY;            // world.pos.y, won't go through this
	f32 maxVel;            // Clamps velocity value
	f32 velStep; // Values below 1.0f will give it spring like motion
	f32 velMult; // Control the power of velocity
} PhysicsInfo;

typedef struct {
	Vec3f pos;
	Vec3s rot;
	MtxF  mtxF;
} PhysicsHead;

typedef struct {
	u32   dlist;
	Vec3f scale;           // Gfx scale
	u8    segID;           // For matrix
	u8    noDraw; /* If there needs to be calculations
	                 to get in position before drawing */
} PhysicsGfx;

typedef struct {
	s32    num; // amount of spheres
	Vec3f* centers;
	f32    radius;
} PhysicsSpheres; // "collision" spheres, pushes limbs away

typedef struct {
	u8    lockRoot;        // Prevent physics rotating root limb
	Vec2f rotStepCalc; // DEG, limits rot to next limb in main calc
	Vec2f rotStepDraw; // DEG, limits rot on draw, smoothens output
} PhysicsConstraint;

typedef struct {
	s32   num;             // How many limbs will be smoothed with push
	Vec3f push; // direction Z, pushes based on rot[0]
	f32   mult; // How much the pushing fill affect
	Vec3f rot;             // DEG, rigids towards, relative rot
} PhysicsRigidity;

typedef struct {
	PhysicsInfo info;
	PhysicsGfx  gfx;
	PhysicsConstraint constraint;
	PhysicsRigidity   rigidity;
	f32 limbsLength[];
} PhysicsStrandInit;

typedef struct {
	PhysicsInfo       info;
	PhysicsHead       head;
	PhysicsGfx        gfx;
	PhysicsSpheres    spheres;
	PhysicsConstraint constraint;
	PhysicsRigidity   rigidity;
	f32* limbsLength;
} PhysicsStrand;

void Audio_PlaySys(u16 flag);
Color_HSL Color_RgbToHsl(f32 r, f32 g, f32 b);
Color_RGB8 Color_HslToRgb(f32 h, f32 s, f32 l);

void Cutscene_ProcessCmds(PlayState* play, CutsceneContext* csCtx, u8* cutscenePtr);
void* CutsceneCmd_ExitParam(PlayState* play, void* ptr);

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
f32 PowF(f32 a, f32 b);

void Physics_GetHeadProperties(PhysicsStrand* strand, Vec3f* headPosModel);
void Physics_SetPhysicsStrand(PhysicsStrandInit* init, PhysicsStrand* dest, f32* limbLengthDest, Vec3f* spheresCenters, s32 spheresArrayCount, f32 spheresRadius);
void Physics_DrawDynamicStrand(GraphicsContext* gfxCtx, TwoHeadGfxArena* disp, PhysicsLimb* jointTable, PhysicsStrand* strand, void* callback, void* callbackArg1, void* callbackArg2);

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
