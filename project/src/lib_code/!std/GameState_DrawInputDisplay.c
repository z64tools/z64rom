#include <uLib.h>
#include <code/audio_synthesis.h>
#include <code/code_800EC960.h>

/*
   z64ram = 0x800C4558
   z64rom = 0xB3B6F8
   z64next = 0x800C46EC
 */

void GameState_DrawInputDisplay(u16 input, Gfx** gfx) {
    u32* func = (void*)AudioSynth_DoOneAudioUpdate;
    
    func[0] = 0x27BDFF08;
    func[1] = 0xAFB5003C;
    func[2] = 0x3C158017;
    func[3] = 0x26B5F180;
    
    func = (void*)func_800F6C34;
    func[0] = 0x3C013F80;
    func[1] = 0x44810000;
    func[2] = 0x3C018013;
    func[3] = 0xA020061C;
}
