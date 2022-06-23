.set noreorder
.set gp=64
.set noat

# z64ram = 0x800D4678
# z64rom = 0xB4B818

addiu   $sp, $sp, -0x18
sw      $ra, 0x14($sp)
move    $a1, $a0
lui     $a0, %hi(gPadMgr)
addiu   $a0, $a0, %lo(gPadMgr)
jal     PadMgr_RequestPadData
move    $a2, $zero
lw      $ra, 0x14($sp)
addiu   $sp, $sp, 0x18
jr      $ra
nop

nop
nop
nop