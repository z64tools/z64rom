#include <uLib.h>

/*
   z64ram = 0x800C61D8
   z64rom = 0xB3D378
   z64next = 0x800C6844
 */

extern OSTime sGraphUpdateTime;
extern FaultClient sGraphUcodeFaultClient;

void Graph_Update(GraphicsContext* gfxCtx, GameState* gameState) {
    u32 problem;
    
    gameState->unk_A0 = 0;
    Graph_InitTHGA(gfxCtx);
    
    OPEN_DISPS(gfxCtx, 0, 966);
    
    gDPNoOpString(WORK_DISP++, 0, 0);
    gDPNoOpString(POLY_OPA_DISP++, 0, 0);
    gDPNoOpString(POLY_XLU_DISP++, 0, 0);
    gDPNoOpString(OVERLAY_DISP++, 0, 0);
    
    CLOSE_DISPS(gfxCtx, "", 975);
    
    GameState_ReqPadData(gameState);
    GameState_Update(gameState);
    
    OPEN_DISPS(gfxCtx, "", 987);
    
    gDPNoOpString(WORK_DISP++, 0, 0);
    gDPNoOpString(POLY_OPA_DISP++, 0, 0);
    gDPNoOpString(POLY_XLU_DISP++, 0, 0);
    gDPNoOpString(OVERLAY_DISP++, 0, 0);
    
    CLOSE_DISPS(gfxCtx, 0, 996);
    
    OPEN_DISPS(gfxCtx, 0, 999);
    
    gSPBranchList(WORK_DISP++, gfxCtx->polyOpaBuffer);
    gSPBranchList(POLY_OPA_DISP++, gfxCtx->polyXluBuffer);
    gSPBranchList(POLY_XLU_DISP++, gfxCtx->overlayBuffer);
    gDPPipeSync(OVERLAY_DISP++);
    gDPFullSync(OVERLAY_DISP++);
    gSPEndDisplayList(OVERLAY_DISP++);
    
    CLOSE_DISPS(gfxCtx, 0, 1028);
    
    if (HREG(80) == 10 && HREG(93) == 2) {
        HREG(80) = 7;
        HREG(81) = -1;
        HREG(83) = HREG(92);
    }
    
    if (HREG(80) == 7 && HREG(81) != 0) {
        if (HREG(82) == 3) {
            Fault_AddClient(&sGraphUcodeFaultClient, Graph_UCodeFaultClient, gfxCtx->workBuffer, 0);
        }
        
        Graph_DisassembleUCode(gfxCtx->workBuffer);
        
        if (HREG(82) == 3) {
            Fault_RemoveClient(&sGraphUcodeFaultClient);
        }
        
        if (HREG(81) < 0) {
            LogUtils_LogHexDump((void*)&HW_REG(SP_MEM_ADDR_REG, u32), 0x20);
            LogUtils_LogHexDump((void*)&DPC_START_REG, 0x20);
        }
        
        if (HREG(81) < 0) {
            HREG(81) = 0;
        }
    }
    
    problem = false;
    
    {
        GfxPool* pool = &gGfxPools[gfxCtx->gfxPoolIdx & 1];
        
        if (pool->headMagic != 0x1234) {
            problem = true;
            Fault_AddHungupAndCrash(0, 1070);
        }
        if (pool->tailMagic != 0x5678) {
            problem = true;
            Fault_AddHungupAndCrash(0, 1076);
        }
    }
    
    if (THGA_IsCrash(&gfxCtx->polyOpa)) {
        problem = true;
    }
    if (THGA_IsCrash(&gfxCtx->polyXlu)) {
        problem = true;
    }
    if (THGA_IsCrash(&gfxCtx->overlay)) {
        problem = true;
    }
    
    if (!problem) {
        Graph_TaskSet00(gfxCtx);
        gfxCtx->gfxPoolIdx++;
        gfxCtx->fbIdx++;
    }
    
    func_800F3054();
    
    {
        OSTime time = osGetTime();
        
        D_8016A538 = gRSPGFXTotalTime;
        D_8016A530 = gRSPAudioTotalTime;
        D_8016A540 = gRDPTotalTime;
        gRSPGFXTotalTime = 0;
        gRSPAudioTotalTime = 0;
        gRDPTotalTime = 0;
        
        if (sGraphUpdateTime != 0) {
            D_8016A548 = time - sGraphUpdateTime;
        }
        sGraphUpdateTime = time;
    }
    
    if (osMemSize > 0x400000U && gLibCtx.__ctxInitValue == 0xDEADBEEF)
        uLib_Update(gameState);
    
    if (gIsCtrlr2Valid && PreNmiBuff_IsResetting(gAppNmiBufferPtr) && !gameState->unk_A0) {
        SET_NEXT_GAMESTATE(gameState, PreNMI_Init, PreNMIState);
        gameState->running = false;
    }
}
