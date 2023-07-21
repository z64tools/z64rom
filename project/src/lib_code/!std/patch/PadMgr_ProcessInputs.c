#include <uLib.h>
#include "vt.h"
#include <code/padmgr.h>

/*
   z64ram = 0x800C7974
   z64rom = 0xB3EB14
   z64next = 0x800C7C14
 */

void PadMgr_ProcessInputs(PadMgr* padMgr) {
    Input* input;
    OSContPad* curPad;
    s32 buttonDiff;
    
    PadMgr_LockPadData(padMgr);
    
    input = &padMgr->inputs[0];
    curPad = &padMgr->pads[0];
    
    for (s32 i = 0; i < padMgr->nControllers; i++, input++, curPad++) {
        input->prev = input->cur;
        
        switch (curPad->errno) {
            case 0:
                input->cur = *curPad;
                if (!padMgr->ctrlrIsConnected[i])
                    padMgr->ctrlrIsConnected[i] = true;
                break;
            case 4:
                input->cur = input->prev;
                break;
            case 8:
                input->cur.button = 0;
                input->cur.stick_x = 0;
                input->cur.stick_y = 0;
                input->cur.errno = curPad->errno;
                if (padMgr->ctrlrIsConnected[i]) {
                    padMgr->ctrlrIsConnected[i] = false;
                    padMgr->pakType[i] = 0;
                    padMgr->rumbleTimer[i] = 0xFF;
                }
                break;
            default:
                Fault_AddHungupAndCrashImpl(__FUNCTION__, "Pad Error");
                break;
        }
        
        buttonDiff = input->prev.button ^ input->cur.button;
        input->press.button |= (u16)(buttonDiff & input->cur.button);
        input->rel.button |= (u16)(buttonDiff & input->prev.button);
        PadUtils_UpdateRelXY(input);
        input->press.stick_x += (s8)(input->cur.stick_x - input->prev.stick_x);
        input->press.stick_y += (s8)(input->cur.stick_y - input->prev.stick_y);
        
        if (i != 0) {
            padMgr->inputs[i].cur.button = 0;
            padMgr->inputs[i].prev.button = 0;
            padMgr->inputs[i].press.button = 0;
            padMgr->inputs[i].rel.button = 0;
        }
    }
    
    PadMgr_UnlockPadData(padMgr);
}
