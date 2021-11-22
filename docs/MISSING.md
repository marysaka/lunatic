The following instructions are currently **NOT** emulated:
- PLD (ARMv5)
- BKPT (ARMv5)
- LDRT/STRT
- VFP instructions
- CDP (coprocessor data processing)
- STC (store coprocessor)
- LDC (load coprocessor)
- CDP2, STC2, LDC2, MCR2, MRC2 (ARMv5?)
- MCRR (ARMv5TE)
- MRRC (ARMv5TE)

Undefined instructions currently throw a runtime exception and do not jump to the ARM undefined exception vector.