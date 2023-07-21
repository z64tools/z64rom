#include <uLib.h>
#include <code/code_800EC960.h>

static s8 sNewSpecReverbs[20] = { 0, 0, 0, 0, 0, 0, 0, 40, 0, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

Asm_VanillaHook(func_800F6C34);
void func_800F6C34(void) {
    sPrevSeqMode = 0;
    D_8016B7A8 = 1.0f;
    D_8016B7B0 = 1.0f;
    sAudioBaseFilter = 0;
    sAudioExtraFilter = 0;
    sAudioBaseFilter2 = 0;
    sAudioExtraFilter2 = 0;
    AudioOcarina_SetInstrument(OCARINA_INSTRUMENT_OFF);
    sRiverFreqScaleLerp.remainingFrames = 0;
    sWaterfallFreqScaleLerp.remainingFrames = 0;
    sRiverFreqScaleLerp.value = 1.0f;
    sWaterfallFreqScaleLerp.value = 1.0f;
    D_8016B7D8 = 1.0f;
    sRiverSoundMainBgmVol = 0x7F;
    sRiverSoundMainBgmCurrentVol = 0x7F;
    sRiverSoundMainBgmLower = false;
    sRiverSoundMainBgmRestore = false;
    sGanonsTowerVol = 0xFF;
    D_8016B9D8 = 0;
    sSpecReverb = sNewSpecReverbs[gAudioSpecId];
    D_80130608 = 0;
    sPrevMainBgmSeqId = NA_BGM_DISABLED;
    Audio_QueueCmdS8(0x46 << 24 | SEQ_PLAYER_BGM_MAIN << 16, -1);
    sSariaBgmPtr = NULL;
    D_8016B9F4 = 0;
    D_8016B9F3 = 1;
    D_8016B9F2 = 0;
}
