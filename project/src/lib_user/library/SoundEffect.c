#include <uLib.h>
#include <z64audio.h>
#include <code/audio_synthesis.h>
#include <code/code_800EC960.h>

#include <uLib_vector.h>
#include <uLib_math.h>

#include <code/audio_heap.h>

#define MAX_SOUND_EFFECTS  32
#define MAX_SOUND_ENTITIES 64

typedef struct {
    /* 0x00 */ s32 order;
    /* 0x04 */ s32 npredictors;
    /* 0x08 */ s16 book[128]; // size 8 * order * npredictors. 8-byte aligned
} AdpcmBookExt; // size >= 0x8

typedef struct STRUCT_ALIGN16 {
    AdpcmBookExt book;
    Sample       sample;
    AdpcmLoop    loop;
    s16 filter[8];
    s8  filterId;
    s8  useFilter;
    
    u32    id;
    void*  origin;
    Vec3f* refPos;
    Vec3f  playPos;
    f32    tuning;
    f32    pitch;
    f32    pitchTarget;
    f32    release;
    f32    stepRelease;
    f32    volume;
    f32    range;
    f32    reverbMod;
    f32    reverbVol;
    
    NoteSynthesisState   synthState;
    NoteSynthesisBuffers synthBuf;
    
    u32 resamplingRate;
    u8  gain;
    u8  pan;
    u8  reverbIndex;
    
    s16 targetVolLeft;
    s16 targetVolRight;
    
    struct {
        vu32 enabled      : 1;
        u32  needsInit    : 1;
        u32  doLoop       : 1;
        u32  hasTwoParts  : 1;
        u32  held         : 1;
        u32  disconnected : 1;
    };
} z64sfx;

typedef struct SoundEntity {
    s8    registered;
    s8    index;
    s8    filterId;
    void* origin;
} SoundEntity;

/*============================================================================*/

static SoundEntity sSoundEntityList[MAX_SOUND_ENTITIES];
static int sNumSoundEntities;

static s16 sSfxFilterFlag;
static s16 sSfxFilter[8];

static z64sfx sSoundEffect[MAX_SOUND_EFFECTS];

/*============================================================================*/

static void SoundEffect_ResamplingRate(z64sfx* this) {
    f32 resamplingRate = 0.0f;
    f32 pitch;
    
    if (this->pitch == this->pitchTarget && this->resamplingRate)
        return;
    
    Math_SmoothStepToF(&this->pitch, this->pitchTarget, 0.25f, 0.5f, 0.01f);
    pitch = this->pitch * this->tuning;
    
    if (pitch < 2.0f) {
        this->hasTwoParts = false;
        resamplingRate = CLAMP_MAX(pitch, 1.99998f);
    } else {
        this->hasTwoParts = true;
        
        if (pitch > 3.99996f)
            resamplingRate = 1.99998f;
        
        else
            resamplingRate = pitch * 0.5f;
    }
    
    this->resamplingRate = (s32)(resamplingRate * 32768.0f);
}

static void SoundEffect_ReleaseHandler(z64sfx* this) {
    f32 target = this->disconnected ? 0 : 1;
    f32 step = this->release > target ? this->stepRelease : 0.1f;
    
    Math_SmoothStepToF(&this->release, target, step, 0.25f, 0.0f);
    
    if (this->release < EPSILON)
        this->enabled = false;
}

static f32 SoundEffect_ComputeVelocity(f32 range, f32 dist) {
    f32 vel;
    
    vel = Remap(dist, CLAMP_MAX(range * 0.2f, 100.0f), range, 1.0f, 0.0f);
    if (dist > range) return 0;
    
    return CLAMP(vel, 0.0f, 1.0f);
}

static f32 SoundEffect_ComputeSoundReverb(f32 range, f32 dist, f32 mod) {
    f32 v = SoundEffect_ComputeVelocity(range, dist);
    
    v = 1.0f - v;
    
    return v * mod;
}

static void SoundEffect_VolumeHandler(z64sfx* this) {
    u8 pan = 0x40;
    f32 vel;
    
    if (this->refPos) {
        PlayState* play = Effect_GetPlayState();
        
        if (!play || !gLibCtx.state.isPlayGameMode)
            return;
        
        Camera* cam = GET_ACTIVE_CAM(play);
        Vec3f projPos;
        f32 w;
        
        SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, &this->playPos, &projPos, &w);
        
        vel = SoundEffect_ComputeVelocity(this->range, Math_Vec3f_DistXYZ(&this->playPos, &cam->eye));
        this->reverbVol = SoundEffect_ComputeSoundReverb(this->range, Math_Vec3f_DistXYZ(&this->playPos, &cam->eye), this->reverbMod);
        pan = Audio_ComputeSfxPanSigned(projPos.x, projPos.z, 0);
        
        this->volume = CLAMP_MIN(this->volume, 0.0f);
        this->gain = 0;
        if (this->volume < 1.0f)
            vel *= this->volume;
        else
            this->gain += 0x10 + (this->volume - 1.0f) * 16;
        
        this->targetVolRight = (vel * gStereoPanVolume[0x7F - pan]) * (0x1000 - 0.001f);
        this->targetVolLeft = (vel * gStereoPanVolume[pan]) * (0x1000 - 0.001f);
    } else {
        f32 vel = 1.0f;
        
        this->volume = CLAMP_MIN(this->volume, 0.0f);
        this->gain = 0;
        if (this->volume < 1.0f)
            vel *= this->volume;
        else
            this->gain += 0x10 + (this->volume - 1.0f) * 4;
        
        this->reverbVol = this->reverbMod;
        this->targetVolRight = (vel * gStereoPanVolume[0x7F - pan]) * (0x1000 - 0.001f);
        this->targetVolLeft = (vel * gStereoPanVolume[pan]) * (0x1000 - 0.001f);
    }
}

static void SoundEffect_InitSoundEffect(z64sfx* this, u32 id, f32 vol, f32 reverb, f32 pitch, Vec3f* pos, f32 range, void* origin) {
    u32 rom;
    f32* tuning;
    
#ifdef DEV_BUILD
    u32 i = 0;
    
    for (;; i++)
        if (&sSoundEffect[i] == this)
            break;
    osLibPrintf("Sample: %3d %08X", i, this);
#endif
    
    memset(this, 0, sizeof(*this));
    
    rom = gDmaDataTable[5].romStart + sizeof(Sample) * id;
    DmaMgr_SendRequest0(&this->sample, rom, sizeof(Sample));
    Assert(this->sample.size != 0);
    
    rom = (u32)this->sample.book + gDmaDataTable[5].romStart;
    DmaMgr_SendRequest0(&this->book, rom, sizeof(AdpcmBookExt));
    Assert(this->book.order != 0);
    
    rom = (u32)this->sample.loop + gDmaDataTable[5].romStart;
    DmaMgr_SendRequest0(&this->loop, rom, sizeof(AdpcmLoop));
    Assert(this->loop.end != 0);
    
    this->sample.medium = MEDIUM_CART;
    this->sample.sampleAddr += gDmaDataTable[5].romStart;
    this->sample.book = (void*)&this->book;
    this->sample.loop = &this->loop;
    tuning = (f32*)this->loop.unk_0C;
    
    this->id = id;
    this->release = 1.0f;
    this->reverbMod = reverb;
    this->volume = vol;
    this->range = range;
    
    this->needsInit = true;
    this->enabled = true;
    
    this->synthState.synthesisBuffers = &this->synthBuf;
    
    this->refPos = pos;
    if (pos)
        this->playPos = *pos;
    this->origin = origin;
    this->tuning = *tuning;
    this->pitch = this->pitchTarget = pitch;
    
    this->useFilter = this->filterId = 0;
    
    SoundEffect_VolumeHandler(this);
    
#ifdef DEV_BUILD
    u32 count = 0;
    for (s32 i = 0; i < MAX_SOUND_EFFECTS; i++) {
        if (sSoundEffect[i].enabled)
            count++;
    }
    
    osLibPrintf("Count: %d", count);
#endif
    
    memset(this->loop.unk_0C, 0, 4);
}

static bool SoundEffect_DistanceCheck(f32 range, Vec3f* pos) {
    if (pos) {
        PlayState* play = Effect_GetPlayState();
        
        if (!play || !gLibCtx.state.isPlayGameMode)
            return 1;
        
        Camera* cam = GET_ACTIVE_CAM(play);
        
        if (SoundEffect_ComputeVelocity(range, Math_Vec3f_DistXYZ(pos, &cam->eye)) <= 0.01f)
            return 1;
    }
    
    return 0;
}

/*============================================================================*/

void SoundEffect_PlayOneshot(SoundFile id, f32 vol, f32 pitch, Vec3f* pos, f32 reverb, f32 range, void* origin) {
    z64sfx* sfx;
    
    Assert(id < SOUND_MAX);
    
    if (SoundEffect_DistanceCheck(range, pos))
        return;
    
    for (s32 i = 0; i < MAX_SOUND_EFFECTS; i++) {
        sfx = &sSoundEffect[i];
        
        if (sfx->enabled) continue;
        
        SoundEffect_InitSoundEffect(sfx, id, vol, reverb, pitch, pos, range, origin);
        return;
    }
}

void SoundEffect_PlayHeld(SoundFile id, f32 vol, f32 pitch, f32 stepRelease, Vec3f* pos, f32 reverb, f32 range, void* origin) {
    z64sfx* sfx;
    z64sfx* initable = NULL;
    
    Assert(id < SOUND_MAX);
    
    if (SoundEffect_DistanceCheck(range, pos))
        return;
    
    for (s32 i = 0; i < MAX_SOUND_EFFECTS; i++) {
        sfx = &sSoundEffect[i];
        
        if (sfx->enabled) {
            if (sfx->origin == origin && sfx->refPos == pos && sfx->id == id) {
                sfx->held = true;
                sfx->pitchTarget = pitch;
                sfx->stepRelease = stepRelease;
                if (pos) sfx->playPos = *pos;
                
                return;
            }
        } else if (initable == NULL)
            initable = sfx;
    }
    
    if (initable) {
        SoundEffect_InitSoundEffect(initable, id, vol, reverb, pitch, pos, range, origin);
        
        initable->held = true;
        initable->doLoop = true;
        initable->stepRelease = stepRelease;
    }
}

void SoundEffect_PlayDefaultOneshot(SoundFile id, Vec3f* pos, void* origin) {
    SoundEffect_PlayOneshot(id, 1.0f, 1.0f, pos, DEFAULT_REVERB, DEFAULT_RANGE, origin);
}

void SoundEffect_PlayDefaultHeld(SoundFile id, f32 stepRelease, Vec3f* pos, void* origin) {
    f32 rev = pos ? DEFAULT_REVERB : 0.0f;
    
    SoundEffect_PlayHeld(id, 1.0f, 1.0f, stepRelease, pos, rev, DEFAULT_RANGE, origin);
}

void SoundEffect_StopOneshot(SoundFile id, Vec3f* pos, void* origin) {
    for (s32 i = 0; i < MAX_SOUND_EFFECTS; i++) {
        if (!sSoundEffect[i].enabled)
            continue;
        
        if (sSoundEffect[i].origin == origin &&
            sSoundEffect[i].refPos == pos &&
            sSoundEffect[i].id == id) {
            
            sSoundEffect[i].held = false;
            sSoundEffect[i].doLoop = true;
            sSoundEffect[i].stepRelease = 0.8f;
        }
    }
}

/*============================================================================*/

void SoundEffect_Init() {
    for (int i = 0; i < MAX_SOUND_ENTITIES; i++)
        sSoundEntityList[i].registered = false;
    sNumSoundEntities = 0;
}

static SoundEntity* SoundEffect_UpdateEntity(z64sfx* this) {
    SoundEntity* sen = NULL;
    
    if (!sNumSoundEntities) return NULL;
    
    for (int i = 0; i < MAX_SOUND_ENTITIES; i++) {
        if (sSoundEntityList[i].registered) {
            if (sSoundEntityList[i].origin == this->origin) {
                sen = &sSoundEntityList[i];
                break;
            }
        }
    }
    
    if (!sen) return NULL;
    
    if (this->filterId != sen->filterId) {
        s8 filter = this->filterId = sen->filterId;
        
        if (filter > 0) {
            osLibPrintf("HiPass: %d", filter);
            AudioHeap_LoadHighPassFilter(this->filter, filter);
            this->useFilter = true;
            
        } else if (filter < 0) {
            filter = 16 + filter;
            osLibPrintf("LoPass: %d", filter);
            AudioHeap_LoadLowPassFilter(this->filter, filter);
            this->useFilter = true;
            
        } else {
            if (this->useFilter == true)
                AudioHeap_ClearFilter(this->filter);
            this->useFilter = false;
        }
    }
    
    if (sen->registered == -1) {
        sen->registered = 0;
        sNumSoundEntities--;
    }
    
    return sen;
}

void SoundEffect_Update(PlayState* play) {
    if (sSfxFilter[3] == 0)
        AudioHeap_LoadLowPassFilter(sSfxFilter, 3);
    
    for (s32 i = 0; i < MAX_SOUND_EFFECTS; i++) {
        z64sfx* this = &sSoundEffect[i];
        
        if (this->enabled == false)
            continue;
        
        SoundEffect_VolumeHandler(this);
        SoundEffect_UpdateEntity(this);
        
        this->disconnected = this->held == false ? true : false;
        this->held = false;
    }
}

/*============================================================================*/

SoundEntity* SoundEntity_Register(void* origin) {
    if (sNumSoundEntities == MAX_SOUND_ENTITIES)
        return NULL;
    
    for (int i = 0; i < MAX_SOUND_ENTITIES; i++) {
        if (sSoundEntityList[i].registered) continue;
        
        sSoundEntityList[i] = (SoundEntity) {
            .registered = true,
            .index = i,
            .origin = origin,
        };
        
        sNumSoundEntities++;
        osLibPrintf("Register SoundEntity: %d / %d", sNumSoundEntities, MAX_SOUND_ENTITIES);
        
        return &sSoundEntityList[i];
    }
    
    return NULL;
}

void SoundEntity_Unregister(SoundEntity* sen) {
    if (!sen || sen->registered < 1) return;
    
    osLibPrintf("Unregister SoundEntity: %d / %d", sNumSoundEntities, MAX_SOUND_ENTITIES);
    sSoundEntityList[sen->index].registered = -1;
}

void SoundEntity_SetFilter(SoundEntity* sen, s8 filter) {
    sen->filterId = CLAMP(filter, -15, 14);
}

/*============================================================================*/

static Acmd* SoundEffect_ProcessEnvelope(z64sfx* this, Acmd* cmd, s32 aiBufLen, u16 inBuf, s32 flags) {
    NoteSynthesisState* synthState = &this->synthState;
    u32 phi_a1;
    u16 curVolLeft;
    u16 targetVolLeft;
    s32 phi_t1;
    s16 reverbVol;
    u16 curVolRight;
    s16 rampLeft;
    s16 rampRight;
    s16 rampReverb;
    s16 sourceReverbVol;
    u16 targetVolRight;
    
    curVolLeft = synthState->curVolLeft;
    targetVolLeft = this->targetVolLeft * this->release;
    targetVolLeft <<= 4;
    reverbVol = this->reverbVol * 0x7F;
    curVolRight = synthState->curVolRight;
    targetVolRight = this->targetVolRight * this->release;
    targetVolRight <<= 4;
    
    if (targetVolLeft != curVolLeft) {
        rampLeft = (targetVolLeft - curVolLeft) / (aiBufLen >> 3);
    } else {
        rampLeft = 0;
    }
    if (targetVolRight != curVolRight) {
        rampRight = (targetVolRight - curVolRight) / (aiBufLen >> 3);
    } else {
        rampRight = 0;
    }
    
    sourceReverbVol = synthState->reverbVol;
    phi_t1 = sourceReverbVol;
    
    if (sourceReverbVol != reverbVol) {
        rampReverb = (((reverbVol & 0x7F) - phi_t1) << 9) / (aiBufLen >> 3);
        synthState->reverbVol = reverbVol;
    } else {
        rampReverb = 0;
    }
    
    synthState->curVolLeft = curVolLeft + (rampLeft * (aiBufLen >> 3));
    synthState->curVolRight = curVolRight + (rampRight * (aiBufLen >> 3));
    
    aEnvSetup1(cmd++, phi_t1 * 2, rampReverb, rampLeft, rampRight);
    aEnvSetup2(cmd++, curVolLeft, curVolRight);
    phi_a1 = sEnvMixerDefaultDmemDests;
    
    aEnvMixer(
        cmd++, inBuf, aiBufLen, ((sourceReverbVol & 0x80) >> 7),
        true, false,
        false, false,
        phi_a1, sEnvMixerOp
    );
    
    return cmd;
}

static Acmd* SoundEffect_ProcessNote(z64sfx* this, s32 aiBufLen, Acmd* cmd, s32 updateIndex) {
    NoteSynthesisState* synthState = &this->synthState;
    Sample* sampleInfo;
    AdpcmLoop* loopInfo;
    s32 nSamplesUntilLoopEnd;
    s32 nSamplesInThisIteration;
    s32 noteFinished;
    s32 restart;
    s32 flags;
    u16 resamplingRateFixedPoint;
    s32 nSamplesInFirstFrame;
    s32 nTrailingSamplesToIgnore;
    s32 gain;
    s32 frameIndex;
    s32 skipBytes;
    s32 nSamplesToDecode;
    u32 sampleAddr;
    u32 samplesLenFixedPoint;
    s32 samplesLenAdjusted;
    s32 nSamplesProcessed;
    s32 loopEndPos;
    s32 nSamplesToProcess;
    s32 phi_s4;
    s32 nFirstFrameSamplesToIgnore;
    s32 frameSize = 0;
    s32 nFramesToDecode;
    s32 skipInitialSamples;
    s32 sampleDataStart;
    u8* sampleData;
    s32 nParts;
    s32 curPart;
    s32 sampleDataStartPad;
    s32 resampledTempLen;
    u16 noteSamplesDmemAddrBeforeResampling;
    s32 sampleDataOffset;
    s32 s5;
    u32 nSamplesToLoad;
    s32 finished;
    s32 aligned;
    s16 addr;
    
    flags = A_CONTINUE;
    if (this->needsInit == true) {
        flags = A_INIT;
        synthState->restart = 0;
        synthState->samplePosInt = 0;
        synthState->samplePosFrac = 0;
        synthState->curVolLeft = 0;
        synthState->curVolRight = 0;
        synthState->reverbVol = this->reverbVol * 0x7F;
        synthState->numParts = 0;
        finished = false;
    }
    
    SoundEffect_ReleaseHandler(this);
    SoundEffect_ResamplingRate(this);
    
    resamplingRateFixedPoint = this->resamplingRate;
    nParts = this->hasTwoParts + 1;
    samplesLenFixedPoint = (resamplingRateFixedPoint * aiBufLen * 2) + synthState->samplePosFrac;
    nSamplesToLoad = samplesLenFixedPoint >> 16;
    synthState->samplePosFrac = samplesLenFixedPoint & 0xFFFF;
    synthState->numParts = nParts;
    sampleInfo = &this->sample;
    loopInfo = sampleInfo->loop;
    loopEndPos = loopInfo->end;
    sampleAddr = (u32)sampleInfo->sampleAddr;
    resampledTempLen = 0;
    
    for (curPart = 0; curPart < nParts; curPart++) {
        nSamplesProcessed = 0;
        s5 = 0;
        
        if (nParts == 1)
            samplesLenAdjusted = nSamplesToLoad;
        
        else if (nSamplesToLoad & 1)
            samplesLenAdjusted = (nSamplesToLoad & ~1) + (curPart * 2);
        
        else
            samplesLenAdjusted = nSamplesToLoad;
        
        if (sampleInfo->codec == CODEC_ADPCM || sampleInfo->codec == CODEC_SMALL_ADPCM) {
            // if (gAudioContext.curLoadedBook != this->book.book) {
            u32 nEntries;
            
            nEntries = 16 * this->book.order * this->book.npredictors;
            aLoadADPCM(cmd++, nEntries, this->book.book);
            gAudioContext.curLoadedBook = this->book.book;
            // }
        }
        
        while (nSamplesProcessed != samplesLenAdjusted) {
            noteFinished = false;
            restart = false;
            phi_s4 = 0;
            
            nFirstFrameSamplesToIgnore = synthState->samplePosInt & 0xF;
            nSamplesUntilLoopEnd = loopEndPos - synthState->samplePosInt;
            nSamplesToProcess = samplesLenAdjusted - nSamplesProcessed;
            
            if (nFirstFrameSamplesToIgnore == 0 && !synthState->restart)
                nFirstFrameSamplesToIgnore = 16;
            
            nSamplesInFirstFrame = 16 - nFirstFrameSamplesToIgnore;
            
            if (nSamplesToProcess < nSamplesUntilLoopEnd) {
                nFramesToDecode = (s32)(nSamplesToProcess - nSamplesInFirstFrame + 15) / 16;
                nSamplesToDecode = nFramesToDecode * 16;
                nTrailingSamplesToIgnore = nSamplesInFirstFrame + nSamplesToDecode - nSamplesToProcess;
                
            } else {
                nSamplesToDecode = nSamplesUntilLoopEnd - nSamplesInFirstFrame;
                nTrailingSamplesToIgnore = 0;
                
                if (nSamplesToDecode <= 0) {
                    nSamplesToDecode = 0;
                    nSamplesInFirstFrame = nSamplesUntilLoopEnd;
                }
                
                nFramesToDecode = (nSamplesToDecode + 15) / 16;
                
                if (loopInfo->count != 0 && this->doLoop)
                    restart = true;
                
                else
                    noteFinished = true;
                
            }
            
            switch (sampleInfo->codec) {
                case CODEC_ADPCM:
                    frameSize = 9;
                    skipInitialSamples = 16;
                    sampleDataStart = 0;
                    break;
                    
                case CODEC_SMALL_ADPCM:
                    frameSize = 5;
                    skipInitialSamples = 16;
                    sampleDataStart = 0;
                    break;
            }
            
            if (nFramesToDecode != 0) {
                frameIndex = (synthState->samplePosInt + skipInitialSamples - nFirstFrameSamplesToIgnore) / 16;
                sampleDataOffset = frameIndex * frameSize;
                sampleData = AudioLoad_DmaSampleData(
                    sampleDataStart + sampleDataOffset + sampleAddr,
                    ALIGN16((nFramesToDecode * frameSize) + 0x10), flags,
                    &synthState->sampleDmaIndex, sampleInfo->medium
                );
                
                if (sampleData == NULL) {
                    osLibPrintf("sampleDataNull");
                    this->enabled = false;
                    
                    return cmd;
                }
                
                sampleDataStartPad = (u32)sampleData & 0xF;
                aligned = ALIGN16((nFramesToDecode * frameSize) + 16);
                addr = DMEM_COMPRESSED_ADPCM_DATA - aligned;
                aLoadBuffer(cmd++, sampleData - sampleDataStartPad, addr, aligned);
            } else {
                nSamplesToDecode = 0;
                sampleDataStartPad = 0;
            }
            
            if (synthState->restart) {
                aSetLoop(cmd++, sampleInfo->loop->predictorState);
                flags = A_LOOP;
                synthState->restart = false;
            }
            
            nSamplesInThisIteration = nSamplesToDecode + nSamplesInFirstFrame - nTrailingSamplesToIgnore;
            if (nSamplesProcessed == 0) {
                if (1) {
                }
                skipBytes = nFirstFrameSamplesToIgnore * 2;
            } else {
                phi_s4 = ALIGN16(s5 + 16);
            }
            
            switch (sampleInfo->codec) {
                case CODEC_ADPCM:
                    aligned = ALIGN16((nFramesToDecode * frameSize) + 0x10);
                    addr = DMEM_COMPRESSED_ADPCM_DATA - aligned;
                    aSetBuffer(
                        cmd++, 0, addr + sampleDataStartPad, DMEM_UNCOMPRESSED_NOTE + phi_s4,
                        nSamplesToDecode * 2
                    );
                    aADPCMdec(cmd++, flags, synthState->synthesisBuffers->adpcmdecState);
                    break;
                    
                case CODEC_SMALL_ADPCM:
                    aligned = ALIGN16((nFramesToDecode * frameSize) + 0x10);
                    addr = DMEM_COMPRESSED_ADPCM_DATA - aligned;
                    aSetBuffer(
                        cmd++, 0, addr + sampleDataStartPad, DMEM_UNCOMPRESSED_NOTE + phi_s4,
                        nSamplesToDecode * 2
                    );
                    aADPCMdec(cmd++, flags | 4, synthState->synthesisBuffers->adpcmdecState);
                    break;
            }
            
            if (nSamplesProcessed != 0) {
                aDMEMMove(
                    cmd++, DMEM_UNCOMPRESSED_NOTE + phi_s4 + (nFirstFrameSamplesToIgnore * 2),
                    DMEM_UNCOMPRESSED_NOTE + s5, nSamplesInThisIteration * 2
                );
            }
            
            nSamplesProcessed += nSamplesInThisIteration;
            
            switch (flags) {
                case A_INIT:
                    skipBytes = 0x20;
                    s5 = (nSamplesToDecode + 0x10) * 2;
                    break;
                    
                case A_LOOP:
                    s5 = nSamplesInThisIteration * 2 + s5;
                    break;
                    
                default:
                    if (s5 != 0) {
                        s5 = nSamplesInThisIteration * 2 + s5;
                    } else {
                        s5 = (nFirstFrameSamplesToIgnore + nSamplesInThisIteration) * 2;
                    }
                    break;
            }
            
            flags = A_CONTINUE;
            
            if (noteFinished) {
                AudioSynth_ClearBuffer(
                    cmd++, DMEM_UNCOMPRESSED_NOTE + s5,
                    (samplesLenAdjusted - nSamplesProcessed) * 2
                );
                finished = true;
                this->enabled = false;
                break;
            } else {
                if (restart) {
                    synthState->restart = true;
                    synthState->samplePosInt = loopInfo->start;
                } else {
                    synthState->samplePosInt += nSamplesToProcess;
                }
            }
        }
        
        switch (nParts) {
            case 1:
                noteSamplesDmemAddrBeforeResampling = DMEM_UNCOMPRESSED_NOTE + skipBytes;
                break;
                
            case 2:
                switch (curPart) {
                    case 0:
                        AudioSynth_InterL(
                            cmd++, DMEM_UNCOMPRESSED_NOTE + skipBytes, DMEM_TEMP + 0x20,
                            ALIGN8(samplesLenAdjusted / 2)
                        );
                        resampledTempLen = samplesLenAdjusted;
                        noteSamplesDmemAddrBeforeResampling = DMEM_TEMP + 0x20;
                        if (finished) {
                            AudioSynth_ClearBuffer(
                                cmd++, noteSamplesDmemAddrBeforeResampling + resampledTempLen,
                                samplesLenAdjusted + 0x10
                            );
                        }
                        break;
                        
                    case 1:
                        AudioSynth_InterL(
                            cmd++, DMEM_UNCOMPRESSED_NOTE + skipBytes,
                            DMEM_TEMP + 0x20 + resampledTempLen, ALIGN8(samplesLenAdjusted / 2)
                        );
                        break;
                }
        }
        
        if (finished)
            break;
        
    }
    
    flags = A_CONTINUE;
    if (this->needsInit == true) {
        this->needsInit = false;
        flags = A_INIT;
    }
    
    cmd = AudioSynth_FinalResample(
        cmd, synthState, aiBufLen * 2, resamplingRateFixedPoint,
        noteSamplesDmemAddrBeforeResampling, flags
    );
    
    if (this->useFilter == true) {
        AudioSynth_LoadFilterSize(cmd++, aiBufLen * SAMPLE_SIZE, this->filter);
        AudioSynth_LoadFilterBuffer(cmd++, flags, DMEM_TEMP, synthState->synthesisBuffers->mixEnvelopeState);
        
    } else if (sSfxFilterFlag && this->refPos) {
        AudioSynth_LoadFilterSize(cmd++, aiBufLen * SAMPLE_SIZE, sSfxFilter);
        AudioSynth_LoadFilterBuffer(cmd++, flags, DMEM_TEMP, synthState->synthesisBuffers->mixEnvelopeState);
    }
    
    gain = this->gain;
    if (gain != 0) {
        // A gain of 0x10 (a UQ4.4 number) is equivalent to 1.0 and represents no volume change
        if (gain < 0x10) {
            gain = 0x10;
        }
        AudioSynth_HiLoGain(cmd++, gain, DMEM_TEMP, 0, (aiBufLen * 2) + 0x20);
    }
    
    cmd = SoundEffect_ProcessEnvelope(this, cmd, aiBufLen, DMEM_TEMP, flags);
    
    return cmd;
}

Asm_VanillaHook(AudioSynth_DoOneAudioUpdate);
Acmd* AudioSynth_DoOneAudioUpdate(s16* aiBuf, s32 aiBufLen, Acmd* cmd, s32 updateIndex) {
    u8 noteIndices[0x5C];
    s16 count;
    s16 reverbIndex;
    SynthesisReverb* reverb;
    s32 useReverb;
    s32 t;
    s32 i;
    NoteSubEu* noteSubEu;
    NoteSubEu* noteSubEu2;
    s32 unk14;
    
    t = gAudioContext.numNotes * updateIndex;
    count = 0;
    if (gAudioContext.numSynthesisReverbs == 0) {
        for (i = 0; i < gAudioContext.numNotes; i++) {
            if (gAudioContext.noteSubsEu[t + i].bitField0.enabled) {
                noteIndices[count++] = i;
            }
        }
    } else {
        for (reverbIndex = 0; reverbIndex < gAudioContext.numSynthesisReverbs; reverbIndex++) {
            for (i = 0; i < gAudioContext.numNotes; i++) {
                noteSubEu = &gAudioContext.noteSubsEu[t + i];
                if (noteSubEu->bitField0.enabled && noteSubEu->bitField1.reverbIndex == reverbIndex) {
                    noteIndices[count++] = i;
                }
            }
        }
        
        for (i = 0; i < gAudioContext.numNotes; i++) {
            noteSubEu = &gAudioContext.noteSubsEu[t + i];
            if (noteSubEu->bitField0.enabled && noteSubEu->bitField1.reverbIndex >= gAudioContext.numSynthesisReverbs) {
                noteIndices[count++] = i;
            }
        }
    }
    
    aClearBuffer(cmd++, DMEM_LEFT_CH, DMEM_2CH_SIZE);
    
    i = 0;
    for (reverbIndex = 0; reverbIndex < gAudioContext.numSynthesisReverbs; reverbIndex++) {
        reverb = &gAudioContext.synthesisReverbs[reverbIndex];
        useReverb = reverb->useReverb;
        if (useReverb) {
            
            // Loads reverb samples from RDRAM (ringBuffer) into DMEM (DMEM_WET_LEFT_CH)
            cmd = AudioSynth_LoadReverbSamples(cmd, aiBufLen, reverb, updateIndex);
            
            // Mixes reverb sample into the main dry channel
            // reverb->volume is always set to 0x7FFF (audio spec), and DMEM_LEFT_CH is cleared before the loop.
            // So for the first iteration, this is essentially a DMEMmove from DMEM_WET_LEFT_CH to DMEM_LEFT_CH
            aMix(cmd++, DMEM_2CH_SIZE >> 4, reverb->volume, DMEM_WET_LEFT_CH, DMEM_LEFT_CH);
            
            unk14 = reverb->unk_14;
            if (unk14) {
                aDMEMMove(cmd++, DMEM_WET_LEFT_CH, DMEM_WET_TEMP, DMEM_2CH_SIZE);
            }
            
            // Decays reverb over time. The (+ 0x8000) here is -100%
            aMix(cmd++, DMEM_2CH_SIZE >> 4, reverb->decayRatio + 0x8000, DMEM_WET_LEFT_CH, DMEM_WET_LEFT_CH);
            
            // Leak reverb between the left and right channels
            if (reverb->leakRtl != 0 || reverb->leakLtr != 0) {
                cmd = AudioSynth_LeakReverb(cmd, reverb);
            }
            
            if (unk14) {
                // Saves the wet channel sample from DMEM (DMEM_WET_LEFT_CH) into RDRAM (ringBuffer) for future use
                cmd = AudioSynth_SaveReverbSamples(cmd, reverb, updateIndex);
                if (reverb->unk_05 != -1) {
                    cmd = AudioSynth_MaybeMixRingBuffer1(cmd, reverb, updateIndex);
                }
                cmd = AudioSynth_MaybeLoadRingBuffer2(cmd, aiBufLen, reverb, updateIndex);
                aMix(cmd++, 0x34, reverb->unk_16, DMEM_WET_TEMP, DMEM_WET_LEFT_CH);
            }
        }
        
        while (i < count) {
            noteSubEu2 = &gAudioContext.noteSubsEu[noteIndices[i] + t];
            if (noteSubEu2->bitField1.reverbIndex == reverbIndex) {
                cmd = AudioSynth_ProcessNote(
                    noteIndices[i], noteSubEu2,
                    &gAudioContext.notes[noteIndices[i]].synthesisState, aiBuf, aiBufLen, cmd,
                    updateIndex
                );
            } else {
                break;
            }
            i++;
        }
        
        for (s32 i = 0; i < MAX_SOUND_EFFECTS; i++) {
            z64sfx* sfx = &sSoundEffect[i];
            
            if (!sfx->enabled || sfx->reverbIndex != reverbIndex)
                continue;
            
            cmd = SoundEffect_ProcessNote(sfx, aiBufLen, cmd, updateIndex);
        }
        
        if (useReverb) {
            if (reverb->filterLeft != NULL || reverb->filterRight != NULL) {
                cmd = AudioSynth_FilterReverb(cmd, aiBufLen * 2, reverb);
            }
            
            // Saves the wet channel sample from DMEM (DMEM_WET_LEFT_CH) into RDRAM (ringBuffer) for future use
            if (unk14) {
                cmd = AudioSynth_SaveRingBuffer2(cmd, reverb, updateIndex);
            } else {
                cmd = AudioSynth_SaveReverbSamples(cmd, reverb, updateIndex);
                if (reverb->unk_05 != -1) {
                    cmd = AudioSynth_MaybeMixRingBuffer1(cmd, reverb, updateIndex);
                }
            }
        }
    }
    
    while (i < count) {
        cmd = AudioSynth_ProcessNote(
            noteIndices[i], &gAudioContext.noteSubsEu[t + noteIndices[i]],
            &gAudioContext.notes[noteIndices[i]].synthesisState, aiBuf, aiBufLen, cmd,
            updateIndex
        );
        i++;
    }
    
    for (s32 i = 0; i < MAX_SOUND_EFFECTS; i++) {
        z64sfx* sfx = &sSoundEffect[i];
        
        if (!sfx->enabled)
            continue;
        if (sfx->reverbIndex < gAudioContext.numSynthesisReverbs)
            continue;
        
        cmd = SoundEffect_ProcessNote(sfx, aiBufLen, cmd, updateIndex);
    }
    
    updateIndex = aiBufLen * 2;
    aInterleave(cmd++, DMEM_TEMP, DMEM_LEFT_CH, DMEM_RIGHT_CH, updateIndex);
    aSaveBuffer(cmd++, DMEM_TEMP, aiBuf, updateIndex * 2);
    
    return cmd;
}

Asm_VanillaHook(Audio_SetExtraFilter);
void Audio_SetExtraFilter(u8 filter) {
    u32 t;
    u8 i;
    
    sSfxFilterFlag = filter;
    sAudioExtraFilter2 = filter;
    sAudioExtraFilter = filter;
    if (D_8016E750[SEQ_PLAYER_BGM_MAIN].unk_254 == NA_BGM_NATURE_AMBIENCE) {
        for (i = 0; i < 16; i++) {
            t = i;
            // CHAN_UPD_SCRIPT_IO (seq player 0, all channels, slot 6)
            Audio_QueueCmdS8(0x6 << 24 | SEQ_PLAYER_BGM_MAIN << 16 | ((t & 0xFF) << 8) | 6, filter);
        }
    }
}
