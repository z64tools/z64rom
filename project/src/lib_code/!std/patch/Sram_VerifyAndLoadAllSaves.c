/*
   z64ram = 0x800A8A20
   z64rom = 0xB1FBC0
   z64next = 0x800A9258
 */

#include <uLib.h>
#include <code/z_sram.h>

#define SLOT_SIZE     (sizeof(SaveContext) + 0x28)
#define CHECKSUM_SIZE (sizeof(Save) / 2)
#define DEATHS        offsetof(SaveContext, deaths)
#define NAME          offsetof(SaveContext, playerName)
#define N64DD         offsetof(SaveContext, n64ddFlag)
#define HEALTH_CAP    offsetof(SaveContext, healthCapacity)
#define QUEST         offsetof(SaveContext, inventory.questItems)
#define DEFENSE       offsetof(SaveContext, inventory.defenseHearts)
#define HEALTH        offsetof(SaveContext, health)
#define SLOT_OFFSET(index) (SRAM_HEADER_SIZE + 0x10 + (index * SLOT_SIZE))

void Sram_VerifyAndLoadAllSaves(FileSelectState* fileChooseCtx, SramContext* sramCtx) {
    u16 i;
    u16 newChecksum;
    u16 slotNum;
    u16 offset;
    u16 j;
    u16 oldChecksum;
    u16* ptr;
    u16 dayTime;
    
    memset(sramCtx->readBuff, 0, SRAM_SIZE);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), sramCtx->readBuff, SRAM_SIZE, OS_READ);
    
    dayTime = ((void)0, gSaveContext.dayTime);
    
    for (slotNum = 0; slotNum < 3; slotNum++) {
        offset = gSramSlotOffsets[slotNum];
        MemCpy(&gSaveContext, sramCtx->readBuff + offset, sizeof(Save));
        
        oldChecksum = gSaveContext.checksum;
        gSaveContext.checksum = 0;
        ptr = (u16*)&gSaveContext;
        
        for (i = newChecksum = j = 0; i < CHECKSUM_SIZE; i++, offset += 2) {
            newChecksum += *ptr++;
        }
        
        if (newChecksum != oldChecksum) {
            offset = gSramSlotOffsets[slotNum + 3];
            MemCpy(&gSaveContext, sramCtx->readBuff + offset, sizeof(Save));
            
            oldChecksum = gSaveContext.checksum;
            gSaveContext.checksum = 0;
            ptr = (u16*)&gSaveContext;
            
            for (i = newChecksum = j = 0; i < CHECKSUM_SIZE; i++, offset += 2)
                newChecksum += *ptr++;
            
            if (newChecksum != oldChecksum) {
                memset(&gSaveContext.entranceIndex, 0, sizeof(s32));
                memset(&gSaveContext.linkAge, 0, sizeof(s32));
                memset(&gSaveContext.cutsceneIndex, 0, sizeof(s32));
                memset(&gSaveContext.dayTime, 0, sizeof(u16));
                memset(&gSaveContext.nightFlag, 0, sizeof(s32));
                memset(&gSaveContext.totalDays, 0, sizeof(s32));
                memset(&gSaveContext.bgsDayCount, 0, sizeof(s32));
                
                Sram_InitNewSave();
                
                ptr = (u16*)&gSaveContext;
                
                for (i = newChecksum = 0; i < CHECKSUM_SIZE; i++)
                    newChecksum += *ptr++;
                
                gSaveContext.checksum = newChecksum;
                
                i = gSramSlotOffsets[slotNum + 3];
                SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + i, &gSaveContext, SLOT_SIZE, OS_WRITE);
            }
            
            i = gSramSlotOffsets[slotNum];
            SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000) + i, &gSaveContext, SLOT_SIZE, OS_WRITE);
        }
    }
    
    memset(sramCtx->readBuff, 0, SRAM_SIZE);
    SsSram_ReadWrite(OS_K1_TO_PHYSICAL(0xA8000000), sramCtx->readBuff, SRAM_SIZE, OS_READ);
    gSaveContext.dayTime = dayTime;
    
    MemCpy(&fileChooseCtx->deaths[0], sramCtx->readBuff + SLOT_OFFSET(0) + DEATHS, sizeof(fileChooseCtx->deaths[0]));
    MemCpy(&fileChooseCtx->deaths[1], sramCtx->readBuff + SLOT_OFFSET(1) + DEATHS, sizeof(fileChooseCtx->deaths[0]));
    MemCpy(&fileChooseCtx->deaths[2], sramCtx->readBuff + SLOT_OFFSET(2) + DEATHS, sizeof(fileChooseCtx->deaths[0]));
    
    MemCpy(
        &fileChooseCtx->fileNames[0],
        sramCtx->readBuff + SLOT_OFFSET(0) + NAME,
        sizeof(fileChooseCtx->fileNames[0])
    );
    MemCpy(
        &fileChooseCtx->fileNames[1],
        sramCtx->readBuff + SLOT_OFFSET(1) + NAME,
        sizeof(fileChooseCtx->fileNames[0])
    );
    MemCpy(
        &fileChooseCtx->fileNames[2],
        sramCtx->readBuff + SLOT_OFFSET(2) + NAME,
        sizeof(fileChooseCtx->fileNames[0])
    );
    
    MemCpy(
        &fileChooseCtx->healthCapacities[0],
        sramCtx->readBuff + SLOT_OFFSET(0) + HEALTH_CAP,
        sizeof(fileChooseCtx->healthCapacities[0])
    );
    MemCpy(
        &fileChooseCtx->healthCapacities[1],
        sramCtx->readBuff + SLOT_OFFSET(1) + HEALTH_CAP,
        sizeof(fileChooseCtx->healthCapacities[0])
    );
    MemCpy(
        &fileChooseCtx->healthCapacities[2],
        sramCtx->readBuff + SLOT_OFFSET(2) + HEALTH_CAP,
        sizeof(fileChooseCtx->healthCapacities[0])
    );
    
    MemCpy(
        &fileChooseCtx->questItems[0],
        sramCtx->readBuff + SLOT_OFFSET(0) + QUEST,
        sizeof(fileChooseCtx->questItems[0])
    );
    MemCpy(
        &fileChooseCtx->questItems[1],
        sramCtx->readBuff + SLOT_OFFSET(1) + QUEST,
        sizeof(fileChooseCtx->questItems[0])
    );
    MemCpy(
        &fileChooseCtx->questItems[2],
        sramCtx->readBuff + SLOT_OFFSET(2) + QUEST,
        sizeof(fileChooseCtx->questItems[0])
    );
    
    MemCpy(
        &fileChooseCtx->n64ddFlags[0],
        sramCtx->readBuff + SLOT_OFFSET(0) + N64DD,
        sizeof(fileChooseCtx->n64ddFlags[0])
    );
    MemCpy(
        &fileChooseCtx->n64ddFlags[1],
        sramCtx->readBuff + SLOT_OFFSET(1) + N64DD,
        sizeof(fileChooseCtx->n64ddFlags[0])
    );
    MemCpy(
        &fileChooseCtx->n64ddFlags[2],
        sramCtx->readBuff + SLOT_OFFSET(2) + N64DD,
        sizeof(fileChooseCtx->n64ddFlags[0])
    );
    
    MemCpy(&fileChooseCtx->defense[0], sramCtx->readBuff + SLOT_OFFSET(0) + DEFENSE, sizeof(fileChooseCtx->defense[0]));
    MemCpy(&fileChooseCtx->defense[1], sramCtx->readBuff + SLOT_OFFSET(1) + DEFENSE, sizeof(fileChooseCtx->defense[0]));
    MemCpy(&fileChooseCtx->defense[2], sramCtx->readBuff + SLOT_OFFSET(2) + DEFENSE, sizeof(fileChooseCtx->defense[0]));
    
    MemCpy(&fileChooseCtx->health[0], sramCtx->readBuff + SLOT_OFFSET(0) + HEALTH, sizeof(fileChooseCtx->health[0]));
    MemCpy(&fileChooseCtx->health[1], sramCtx->readBuff + SLOT_OFFSET(1) + HEALTH, sizeof(fileChooseCtx->health[0]));
    MemCpy(&fileChooseCtx->health[2], sramCtx->readBuff + SLOT_OFFSET(2) + HEALTH, sizeof(fileChooseCtx->health[0]));
}
