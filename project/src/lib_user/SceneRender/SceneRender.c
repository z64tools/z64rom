#define __Z64SCENEREDNER_C__
#include "uLib.h"

void func_8009BEEC(GlobalContext* globalCtx);
asm ("func_8009BEEC = 0x8009BEEC");

static SceneAnimContext gSceneAnimCtx;

/* propagate ram segment with pointer to data */
static void segment(GlobalContext* globalCtx, s32 seg, void* data) {
	GraphicsContext* gfx = globalCtx->state.gfxCtx;
	
	gSPSegment(gfx->polyOpa.p++, seg, data);
	gSPSegment(gfx->polyXlu.p++, seg, data);
	// TODO is `overlay` also needed, or does
	//      the above already work for decals?
}

/* returns 1 = flag active; 0 = flag inactive */
static s32 flag(GlobalContext* globalCtx, Flag* f) {
	s32 r = 0;
	
	switch (f->type) {
		case FLAG_TYPE_ROOMCLEAR:
			r = Flags_GetClear(globalCtx, f->flag);
			break;
			
		case FLAG_TYPE_TREASURE:
			r = Flags_GetTreasure(globalCtx, f->flag);
			break;
			
		case FLAG_TYPE_USCENE:
			r = Flags_GetUnknown(globalCtx, f->flag);
			break;
			
		case FLAG_TYPE_TEMP:
			r = Flags_GetTempClear(globalCtx, f->flag);
			break;
			
		case FLAG_TYPE_SCENECOLLECT:
			r = Flags_GetCollectible(globalCtx, f->flag);
			break;
			
		case FLAG_TYPE_SWITCH:
			r = Flags_GetSwitch(globalCtx, f->flag);
			break;
			
		case FLAG_TYPE_EVENTCHKINF:
			r = Flags_GetEventChkInf(f->flag);
			break;
			
		case FLAG_TYPE_INFTABLE:
			r = Flags_GetInfTable(f->flag);
			break;
			
		case FLAG_TYPE_IS_NIGHT:
			r = (*(u32*)(gSaveContext.nightFlag == 1));
			break;
			
		case FLAG_TYPE_SAVE:
			r = !!((*(u32*)((u8*)(&gSaveContext) + f->flag)) & f->and);
			break;
			
		case FLAG_TYPE_GLOBAL:
			r = !!(
				(*(u32*)((u8*)(globalCtx) + f->flag)) & f->and
			);
			break;
			
		case FLAG_TYPE_RAM:
			r = !!((*(u32*)(f->flag)) & f->and);
			break;
	}
	
	return f->eq == r;
}

/* given a color list header and gameplay frame, *
* get the keyframe within the color list        */
static ColorKey* SceneAnim_GetColorKey(ColorList* list, u32* frame) {
	ColorKey* key;
	u32 along = 0;
	
	if (!list->dur)
		return list->key;
	
	/* loop */
	*frame %= list->dur;
	
	/* locate appropriate keyframe */
	for (key = list->key; key->next; ++key) {
		u32 next = along + key->next;
		
		/* keyframe range encompasses frame */
		if (*frame >= along && *frame <= next)
			return key;
		
		/* advance to next frame range */
		along = next;
	}
	
	/* found none; return first keyframe */
	return list->key;
}

/* get next key in list */
static ColorKey* SceneAnim_GetNextColorKey(ColorList* list, ColorKey* key) {
	/* next == 0 indicates end of list */
	if (!key->next)
		return key;
	
	/* advance to next key */
	key += 1;
	
	/* key->next == 0 indicates end of list, so loop to first key */
	if (!key->next)
		return list->key;
	
	/* return next key */
	return key;
}

static s32 SceneAnim_Ease_s32(s32 from, s32 to, float factor) {
	return from + (to - from) * factor;
}

static s32 SceneAnim_Ease_s8(s32 from, s32 to, float factor) {
	return SceneAnim_Ease_s32(from, to, factor) & 0xFF;
}

static u32 SceneAnim_Ease_RGBA(u32 from, u32 to, float factor) {
	u8 rgba[4];
	u8* from8 = (u8*)&from;
	u8* to8 = (u8*)&to;
	
	rgba[0] = SceneAnim_Ease_s32(from8[0], to8[0], factor);
	rgba[1] = SceneAnim_Ease_s32(from8[1], to8[1], factor);
	rgba[2] = SceneAnim_Ease_s32(from8[2], to8[2], factor);
	rgba[3] = SceneAnim_Ease_s32(from8[3], to8[3], factor);
	
	return *(u32*)rgba;
}

static void SceneAnim_PutColorKey(u8 which, Gfx** work, ColorKey* key) {
	if (which & COLORKEY_PRIM) {
		u8* rgba = (u8*)&key->prim;
		
		gDPSetPrimColor(
			(*work)++
			,
			key->mlevel
			,
			key->lfrac
			,
			rgba[0]
			,
			rgba[1]
			,
			rgba[2]
			,
			rgba[3]
		);
	}
	
	if (which & COLORKEY_ENV) {
		u8* rgba = (u8*)&key->env;
		
		gDPSetEnvColor(
			(*work)++
			,
			rgba[0]
			,
			rgba[1]
			,
			rgba[2]
			,
			rgba[3]
		);
	}
}

static void SceneAnim_BlendColorKey(
	enum8(ColorKeyTypes) which,
	float factor,
	ColorKey* from,
	ColorKey* to,
	ColorKey* result
) {
	if (from == to) {
		*result = *from;
		
		return;
	}
	
	/* primcolor */
	if (which & COLORKEY_PRIM) {
		result->prim = SceneAnim_Ease_RGBA(from->prim, to->prim, factor);
		
		if (which & COLORKEY_LODFRAC)
			result->lfrac = SceneAnim_Ease_s8(from->lfrac, to->lfrac, factor);
		
		if (which & COLORKEY_MINLEVEL)
			result->mlevel = SceneAnim_Ease_s8(from->mlevel, to->mlevel, factor);
	}
	
	/* envcolor */
	if (which & COLORKEY_ENV) {
		result->env = SceneAnim_Ease_RGBA(from->env, to->env, factor);
	}
}

static float SceneAnim_Interpolate(u32 frame, u32 next, enum8(ease) ease) {
	float factor;
	
	if (!next)
		return 0;
	
	factor = frame;
	
	/* normalize factor */
	factor /= next;
	
	// TODO transform factor with easing transformations
	if (ease != EASE_LINEAR) {
		switch (ease) {
			/* sinusoidal (in) */
			case EASE_SIN_IN:
				break;
				
			/* sinusoidal (out) */
			case EASE_SIN_OUT:
				break;
				
			/* unsupported: use linear */
			default:
				break;
		}
	}
	
	return factor;
}

static void SceneAnim_Color_Loop(GlobalContext* globalCtx, Gfx** work, ColorList* list) {
	ColorKey* from;
	ColorKey* to;
	u32 frame = globalCtx->gameplayFrames;
	float factor;
	
	from = SceneAnim_GetColorKey(list, &frame);
	to = SceneAnim_GetNextColorKey(list, from);
	
	factor = SceneAnim_Interpolate(frame, from->next, list->ease);
	
	/* blend color keys (result goes into Pcolorkey) */
	SceneAnim_BlendColorKey(list->which, factor, from, to, &gSceneAnimCtx.Pcolorkey);
	
	SceneAnim_PutColorKey(list->which, work, &gSceneAnimCtx.Pcolorkey);
}

static void SceneAnim_Color_LoopFlag(GlobalContext* globalCtx, Gfx** work, ColorListFlag* c) {
	Flag* f = &c->flag;
	ColorList* list = &c->list;
	
	ColorKey* key;
	ColorKey Nkey;
	s32 active = flag(globalCtx, f);
	s32 xfading = (f->frames || active) && f->frames <= f->xfade && f->xfade;
	
	enum8(ColorKeyTypes) which = list->which;
	
	// if (flag(globalCtx, &SceneAnim_TexScroll_One->flag))
	//	SceneAnim_TexScroll_One->flag.frames++;
	
	/* if not cross fading, write to Pcolorkey */
	if (!xfading)
		key = &gSceneAnimCtx.Pcolorkey;
	else
		key = &Nkey;
	
	/* not across-fading, and flag is not active */
	if (!active && !f->frames && !f->freeze)
		return;
	
	/* if cross fading or flag is active, compute colors */
	if (active || xfading) {
		ColorKey* from;
		ColorKey* to;
		u32 frame = globalCtx->gameplayFrames;
		float factor;
		
		from = SceneAnim_GetColorKey(list, &frame);
		to = SceneAnim_GetNextColorKey(list, from);
		
		factor = SceneAnim_Interpolate(frame, from->next, list->ease);
		
		/* blend color keys (result goes into key) */
		SceneAnim_BlendColorKey(which, factor, from, to, key);
	}
	
	/* if cross fading, interpolate between old and new colors */
	if (xfading) {
		float factor;
		
		if (active)
			f->frames += f->frames < f->xfade;
		else
			f->frames -= f->frames > 0;
		
		factor = SceneAnim_Interpolate(f->frames, f->xfade, list->ease);
		
		/* blend color keys (result goes into Pcolorkey) */
		SceneAnim_BlendColorKey(which, factor, &gSceneAnimCtx.Pcolorkey, key, &gSceneAnimCtx.Pcolorkey);
		key = &gSceneAnimCtx.Pcolorkey;
	}
	
	SceneAnim_PutColorKey(which, work, key);
}

static s32 SceneAnim_ABS(s32 x) {
	return (x < 0) ? -x : x;
}

/* TODO SEGMENTED_TO_VIRTUAL should do the same thing; test it later */
static void* SceneAnim_Mkabs(GlobalContext* globalCtx, void* ptr) {
	u8* scene = (u8*)globalCtx->sceneSegment;
	u32 ptr32 = (u32)ptr;
	
	if (!ptr)
		return 0;
	
	scene += ptr32 & 0xFFFFFF;
	
	return scene;
}

/* change pointer based on flag */
static void SceneAnim_Pointer_Flag(GlobalContext* globalCtx, Gfx** work, PointerFlag* ptr) {
	// TODO don't forget to uncomment this
	*work = (void*)SEGMENTED_TO_VIRTUAL(ptr->ptr[flag(globalCtx, &ptr->flag)]);
	// testing:
	// *work = (void*)SEGMENTED_TO_VIRTUAL(ptr->ptr[0]);
}

/* change pointer as time progresses (each pointer has its own time) */
static void SceneAnim_Pointer_Timeloop(GlobalContext* globalCtx, Gfx** work, PointerTimeloop* ptr) {
	s32 item;
	s32 num = ptr->num;
	u32* list = (void*)(ptr->each + num + !(num & 1));
	
	/* walk list */
	for (item = ptr->prev; item < num; ++item)
		if (ptr->time >= ptr->each[item])
			break;
	
	/* reached end of animation; roll back to beginning */
	if (item >= num - 1)
		item = ptr->prev = ptr->time = 0;
	
	*work = (void*)SEGMENTED_TO_VIRTUAL(list[item]);
	
	/* increment time elapsed */
	ptr->time += 1;
	if (ptr->time == ptr->each[item + 1])
		ptr->prev += 1;
}

/* change pointer as time progresses (each pointer has its own time) */
/* skipped if flag is undesirable */
static s32 SceneAnim_Pointer_TimeloopFlag(GlobalContext* globalCtx, Gfx** work, PointerTimeloopFlag* _ptr) {
	PointerTimeloop* ptr = &_ptr->list;
	
	if (!flag(globalCtx, &_ptr->flag))
		return 0;
	
	SceneAnim_Pointer_Timeloop(globalCtx, work, ptr);
	
	return 1;
}

/* change pointer as time progresses */
static void SceneAnim_Pointer_Loop(GlobalContext* globalCtx, Gfx** work, PointerLoop* ptr) {
	s32 item;
	
	/* overflow test */
	if (ptr->time >= ptr->dur)
		ptr->time = 0;
	
	item = ptr->time / ptr->each;
	*work = (void*)SEGMENTED_TO_VIRTUAL(ptr->ptr[item]);
	
	/* increment time elapsed */
	ptr->time += 1;
}

/* change pointer as time progresses */
/* skipped if flag is undesirable */
static s32 SceneAnim_Pointer_LoopFlag(GlobalContext* globalCtx, Gfx** work, PointerLoopFlag* _ptr) {
	PointerLoop* ptr = &_ptr->list;
	u8 flagstate = flag(globalCtx, &_ptr->flag);
	Flag* f = &_ptr->flag;
	
	if (!flagstate && f->freeze == 0)
		return 0;
	
	SceneAnim_Pointer_Loop(globalCtx, work, ptr);
	
	// if freeze mode is set, time doesnt advance when flag is not set
	if (!flagstate && f->freeze == 1)
		ptr->time -= 1;
	
	return 1;
}

/* SceneAnim_TexScroll_One one tile layer */
// TODO does MM really SceneAnim_TexScroll_One only one layer using this?
static void SceneAnim_TexScroll_One(GlobalContext* globalCtx, Gfx** work, TexScroll* sc) {
	u32 frame = globalCtx->gameplayFrames;
	
	gDPSetTileSize(
		(*work)++
		,
		0,
		sc->u * frame,
		sc->v * frame,
		sc->w,
		sc->h
	);
}

/* SceneAnim_TexScroll_One two tile layers */
static void SceneAnim_TexScroll_Two(GlobalContext* globalCtx, Gfx** work, TexScroll* sc) {
	TexScroll* sc1 = sc + 1;
	u32 frame = globalCtx->gameplayFrames;
	
	gDPSetTileSize(
		(*work)++
		,
		0,
		sc->u * frame,
		sc->v * frame,
		sc->w,
		sc->h
	);
	
	gDPSetTileSize(
		(*work)++
		,
		1,
		sc1->u * frame,
		sc1->v * frame,
		sc1->w,
		sc1->h
	);
}

/* SceneAnim_TexScroll_One tiles based on flag */
static void SceneAnim_TexScroll_Flag(GlobalContext* globalCtx, Gfx** work, TexScrollFlag* SceneAnim_TexScroll_One) {
	TexScroll* sc = SceneAnim_TexScroll_One->sc;
	TexScroll* sc1 = sc + 1;
	u16 frame = SceneAnim_TexScroll_One->flag.frames;
	
	if (flag(globalCtx, &SceneAnim_TexScroll_One->flag))
		SceneAnim_TexScroll_One->flag.frames++;
	
	gDPSetTileSize(
		(*work)++
		,
		0,
		sc->u * frame,
		sc->v * frame,
		sc->w,
		sc->h
	);
	
	gDPSetTileSize(
		(*work)++
		,
		1,
		sc1->u * frame,
		sc1->v * frame,
		sc1->w,
		sc1->h
	);
}

/* change pointer as time progresses (each pointer has its own time) */
/* skipped if flag is undesirable */
static s32 SceneAnim_CameraEffect(GlobalContext* globalCtx, Gfx** work, CameraEffect* cam) { // TODO
	u8 cameratype = cam->cameratype;
	
	if (!flag(globalCtx, &cam->flag))
		return 0;
	
	if (cameratype == 0) {
		func_8009BEEC(globalCtx); // camera shake
	} else { // jabu jabu
		static s16 D_UNK_JAB1 = 538;
		static s16 D_UNK_JAB2 = 4272;
		
		f32 temp;
		
		if (FrameAdvance_IsEnabled(globalCtx) != true) {
			D_UNK_JAB1 += 1820;
			D_UNK_JAB2 += 1820;
			
			temp = 0.020000001f;
			func_800AA76C(
				&globalCtx->view,
				((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_CosS(D_UNK_JAB1),
				((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_SinS(D_UNK_JAB1),
				((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_SinS(D_UNK_JAB2)
			);
			func_800AA78C(
				&globalCtx->view,
				1.f + (0.79999995f * temp * Math_SinS(D_UNK_JAB2)),
				1.f + (0.39999998f * temp * Math_CosS(D_UNK_JAB2)),
				1.f + (1 * temp * Math_CosS(D_UNK_JAB1))
			);
			func_800AA7AC(&globalCtx->view, 0.95f);
		}
	}
	
	return 1;
}

/* change pointer as time progresses (each pointer has its own time) */
/* skipped if flag is undesirable */
static s32 SceneAnim_DrawCondition(GlobalContext* globalCtx, Gfx** work, DrawCondition* _ptr) {
	if (!flag(globalCtx, &_ptr->flag))
		return 0;
	
	return 1;
}

static void* SceneAnim_GetSceneHeader(GlobalContext* globalCtx) {
	u32* header = (u32*)globalCtx->sceneSegment;
	u32 setup = gSaveContext.sceneSetupIndex;
	
	if (setup && *header == 0x18000000) {
		u32* list = SEGMENTED_TO_VIRTUAL(header[1]);
		
		for (s32 i = setup - 1; i >= 0; --i)
			if (list[i])
				return SEGMENTED_TO_VIRTUAL(list[i]);
	}
	
	return header;
}

static AnimInfo* SceneAnim_GetSceneAnimCommand(void* _scene) {
	u8* scene = _scene;
	u8* header = scene;
	
	if (!scene)
		return 0;
	
	/* while current header command is not end command */
	while (*header != 0x14) {
		if (*header == 0x1A) {
			return SEGMENTED_TO_VIRTUAL(*((u32*)(header + 4)));
		}
		
		header += 8;
	}
	
	/* failed to locate 0x1A command */
	return 0;
}

static void SceneAnim_UnusedDL(Gfx** work) {
	gDPNoOp((*work)++);
	gSPEndDisplayList((*work)++);
}

static void SceneAnim_UnusedSegment(GlobalContext* globalCtx, s32 seg) {
	GraphicsContext* gfx = globalCtx->state.gfxCtx;
	Gfx* buf;
	Gfx* Obuf;
	
	buf = Obuf = Graph_Alloc(gfx, 16);
	
	SceneAnim_UnusedDL(&buf);
	
	segment(globalCtx, seg, Obuf);
}

void SceneAnim_Init(GlobalContext* globalCtx) {
	AnimInfo* item;
	
	gSceneAnimCtx.animInfoList = SceneAnim_GetSceneAnimCommand(SceneAnim_GetSceneHeader(globalCtx));
	
	if (gSceneAnimCtx.animInfoList) {
		if (((u32)gSceneAnimCtx.animInfoList->data & 0xFF000000) != 0x80000000) {
			for (item = gSceneAnimCtx.animInfoList; ; ++item) {
				s8 Oseg = item->seg;
				item->seg = SceneAnim_ABS(item->seg) + 7;
				item->data = SceneAnim_Mkabs(globalCtx, item->data);
				
				if (Oseg <= 0) {
					item->seg |= 0x80;
					break;
				}
			}
		}
	}
}

void SceneAnim_Update(GlobalContext* globalCtx) {
	AnimInfo* item;
	TwoHeadGfxArena* buf;
	GraphicsContext* gfxCtx;
	s32 prevSeg = 0;
	Gfx* work = 0;
	Gfx* Owork = 0;
	s32 hasWrittenPointer = 0; /* used to test if pointer written */
	
	gfxCtx = (globalCtx->state).gfxCtx;
	
	SceneAnim_Init(globalCtx);
	
	buf = &(gfxCtx->polyOpa);
	gDPPipeSync(buf->p++);
	gDPSetEnvColor(buf->p++, 128, 128, 128, 128);
	
	buf = &(gfxCtx->polyXlu);
	gDPPipeSync(buf->p++);
	gDPSetEnvColor(buf->p++, 128, 128, 128, 128);
	
	if (!gSceneAnimCtx.animInfoList) {
		/* propagate ram segments with defaults so scene *
		* headers lacking a 0x1A command do not crash   */
		for (s32 i = 0x08; i <= 0x0F; ++i)
			SceneAnim_UnusedSegment(globalCtx, i);
		
		return;
	}
	
	for (item = gSceneAnimCtx.animInfoList; ; ++item) {
		s32 seg = item->seg & 0x7F;
		void* data = item->data;
		
		/* begin work on new dlist */
		if (seg != prevSeg || !work) {
			/* flush generated dlist's contents to ram segment */
			if (work) {
				/* is display list */
				if (work > Owork)
					gSPEndDisplayList(work++);
				
				segment(globalCtx, prevSeg, Owork);
			}
			
			/* request space for 8 opcodes in graphics memory */
			Owork = work = Graph_Alloc(globalCtx->state.gfxCtx, 8 * 8);
			prevSeg = seg;
			
			/* zero-initialize any variables needing it */
			hasWrittenPointer = 0;
		}
		
		switch (item->type) {
			/* SceneAnim_TexScroll_One one layer */
			case 0x0000:
				SceneAnim_TexScroll_One(globalCtx, &work, data);
				break;
				
			/* SceneAnim_TexScroll_One two layers */
			case 0x0001:
				SceneAnim_TexScroll_Two(globalCtx, &work, data);
				break;
				
			/* cycle through color list (advance one per frame) */
			case 0x0002:
				SceneAnim_UnusedDL(&work);
				break;
				
			/* unused in MM */
			case 0x0003:
				SceneAnim_UnusedDL(&work);
				break;
				
			/* color easing with keyframe support */
			case 0x0004:
				SceneAnim_UnusedDL(&work);
				break;
				
			/* flag-based texture scrolling */
			/* used only in Sakon's Hideout in MM */
			case 0x0005:
				SceneAnim_UnusedDL(&work);
				break;
				
			/* nothing; data and seg are always 0 when this is used */
			case 0x0006:
				SceneAnim_UnusedDL(&work);
				break;
				
			/* extended functionality */
				
			/* pointer changes based on flag */
			case 0x0007:
				if (hasWrittenPointer)
					break;
				hasWrittenPointer = 1;
				SceneAnim_Pointer_Flag(globalCtx, &work, data);
				Owork = work;
				break;
				
			/* SceneAnim_TexScroll_One tiles based on flag */
			case 0x0008:
				SceneAnim_TexScroll_Flag(globalCtx, &work, data);
				break;
				
			/* loop through color list */
			case 0x0009:
				SceneAnim_Color_Loop(globalCtx, &work, data);
				break;
				
			/* loop through color list, with flag */
			case 0x000A:
				SceneAnim_Color_LoopFlag(globalCtx, &work, data);
				break;
				
			/* loop through pointer list */
			case 0x000B:
				if (hasWrittenPointer)
					break;
				hasWrittenPointer = 1;
				SceneAnim_Pointer_Loop(globalCtx, &work, data);
				Owork = work;
				break;
				
			/* loop through pointer list */
			/* skipped if flag is undesirable */
			case 0x000C:
				if (hasWrittenPointer)
					break;
				if (SceneAnim_Pointer_LoopFlag(globalCtx, &work, data))
					hasWrittenPointer = 1;
				Owork = work;
				break;
				
			/* loop through pointer list (each frame has its own time) */
			case 0x000D:
				if (hasWrittenPointer)
					break;
				hasWrittenPointer = 1;
				SceneAnim_Pointer_Timeloop(globalCtx, &work, data);
				Owork = work;
				break;
				
			/* loop through pointer list (each frame has its own time) */
			/* skipped if flag is undesirable */
			case 0x000E:
				if (hasWrittenPointer)
					break;
				if (SceneAnim_Pointer_TimeloopFlag(globalCtx, &work, data))
					hasWrittenPointer = 1;
				Owork = work;
				break;
				
			/* camera effect if flag is set */
			case 0x000F:
				SceneAnim_CameraEffect(globalCtx, &work, data);
				break;
				
			/* draw if flag is set */
			case 0x0010:
				SceneAnim_DrawCondition(globalCtx, &work, data);
				break;
				
			default:
				SceneAnim_UnusedDL(&work);
				break;
		}
		
		/* this bit means this is the last item in the list */
		if (item->seg & 0x80) {
			/* flush generated dlist's contents to ram segment */
			if (work) {
				/* is display list */
				if (work > Owork)
					gSPEndDisplayList(work++);
				
				segment(globalCtx, prevSeg, Owork);
			}
			break;
		}
	}
}
