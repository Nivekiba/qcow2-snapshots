# CS_ARCH_SPARC, CS_MODE_BIG_ENDIAN, None
0x85,0x66,0x40,0x01 = movne %icc, %g1, %g2
0x85,0x64,0x40,0x01 = move %icc, %g1, %g2
0x85,0x66,0x80,0x01 = movg %icc, %g1, %g2
0x85,0x64,0x80,0x01 = movle %icc, %g1, %g2
0x85,0x66,0xc0,0x01 = movge %icc, %g1, %g2
0x85,0x64,0xc0,0x01 = movl %icc, %g1, %g2
0x85,0x67,0x00,0x01 = movgu %icc, %g1, %g2
0x85,0x65,0x00,0x01 = movleu %icc, %g1, %g2
0x85,0x67,0x40,0x01 = movcc %icc, %g1, %g2
0x85,0x65,0x40,0x01 = movcs %icc, %g1, %g2
0x85,0x67,0x80,0x01 = movpos %icc, %g1, %g2
0x85,0x65,0x80,0x01 = movneg %icc, %g1, %g2
0x85,0x67,0xc0,0x01 = movvc %icc, %g1, %g2
0x85,0x65,0xc0,0x01 = movvs %icc, %g1, %g2
0x85,0x66,0x50,0x01 = movne %xcc, %g1, %g2
0x85,0x64,0x50,0x01 = move %xcc, %g1, %g2
0x85,0x66,0x90,0x01 = movg %xcc, %g1, %g2
0x85,0x64,0x90,0x01 = movle %xcc, %g1, %g2
0x85,0x66,0xd0,0x01 = movge %xcc, %g1, %g2
0x85,0x64,0xd0,0x01 = movl %xcc, %g1, %g2
0x85,0x67,0x10,0x01 = movgu %xcc, %g1, %g2
0x85,0x65,0x10,0x01 = movleu %xcc, %g1, %g2
0x85,0x67,0x50,0x01 = movcc %xcc, %g1, %g2
0x85,0x65,0x50,0x01 = movcs %xcc, %g1, %g2
0x85,0x67,0x90,0x01 = movpos %xcc, %g1, %g2
0x85,0x65,0x90,0x01 = movneg %xcc, %g1, %g2
0x85,0x67,0xd0,0x01 = movvc %xcc, %g1, %g2
0x85,0x65,0xd0,0x01 = movvs %xcc, %g1, %g2
0x85,0x61,0xc0,0x01 = movu %fcc0, %g1, %g2
0x85,0x61,0x80,0x01 = movg %fcc0, %g1, %g2
0x85,0x61,0x40,0x01 = movug %fcc0, %g1, %g2
0x85,0x61,0x00,0x01 = movl %fcc0, %g1, %g2
0x85,0x60,0xc0,0x01 = movul %fcc0, %g1, %g2
0x85,0x60,0x80,0x01 = movlg %fcc0, %g1, %g2
0x85,0x60,0x40,0x01 = movne %fcc0, %g1, %g2
0x85,0x62,0x40,0x01 = move %fcc0, %g1, %g2
0x85,0x62,0x80,0x01 = movue %fcc0, %g1, %g2
0x85,0x62,0xc0,0x01 = movge %fcc0, %g1, %g2
0x85,0x63,0x00,0x01 = movuge %fcc0, %g1, %g2
0x85,0x63,0x40,0x01 = movle %fcc0, %g1, %g2
0x85,0x63,0x80,0x01 = movule %fcc0, %g1, %g2
0x85,0x63,0xc0,0x01 = movo %fcc0, %g1, %g2
0x85,0xaa,0x60,0x21 = fmovsne %icc, %f1, %f2
0x85,0xa8,0x60,0x21 = fmovse %icc, %f1, %f2
0x85,0xaa,0xa0,0x21 = fmovsg %icc, %f1, %f2
0x85,0xa8,0xa0,0x21 = fmovsle %icc, %f1, %f2
0x85,0xaa,0xe0,0x21 = fmovsge %icc, %f1, %f2
0x85,0xa8,0xe0,0x21 = fmovsl %icc, %f1, %f2
0x85,0xab,0x20,0x21 = fmovsgu %icc, %f1, %f2
0x85,0xa9,0x20,0x21 = fmovsleu %icc, %f1, %f2
0x85,0xab,0x60,0x21 = fmovscc %icc, %f1, %f2
0x85,0xa9,0x60,0x21 = fmovscs %icc, %f1, %f2
0x85,0xab,0xa0,0x21 = fmovspos %icc, %f1, %f2
0x85,0xa9,0xa0,0x21 = fmovsneg %icc, %f1, %f2
0x85,0xab,0xe0,0x21 = fmovsvc %icc, %f1, %f2
0x85,0xa9,0xe0,0x21 = fmovsvs %icc, %f1, %f2
0x85,0xaa,0x70,0x21 = fmovsne %xcc, %f1, %f2
0x85,0xa8,0x70,0x21 = fmovse %xcc, %f1, %f2
0x85,0xaa,0xb0,0x21 = fmovsg %xcc, %f1, %f2
0x85,0xa8,0xb0,0x21 = fmovsle %xcc, %f1, %f2
0x85,0xaa,0xf0,0x21 = fmovsge %xcc, %f1, %f2
0x85,0xa8,0xf0,0x21 = fmovsl %xcc, %f1, %f2
0x85,0xab,0x30,0x21 = fmovsgu %xcc, %f1, %f2
0x85,0xa9,0x30,0x21 = fmovsleu %xcc, %f1, %f2
0x85,0xab,0x70,0x21 = fmovscc %xcc, %f1, %f2
0x85,0xa9,0x70,0x21 = fmovscs %xcc, %f1, %f2
0x85,0xab,0xb0,0x21 = fmovspos %xcc, %f1, %f2
0x85,0xa9,0xb0,0x21 = fmovsneg %xcc, %f1, %f2
0x85,0xab,0xf0,0x21 = fmovsvc %xcc, %f1, %f2
0x85,0xa9,0xf0,0x21 = fmovsvs %xcc, %f1, %f2
0x85,0xa9,0xc0,0x21 = fmovsu %fcc0, %f1, %f2
0x85,0xa9,0x80,0x21 = fmovsg %fcc0, %f1, %f2
0x85,0xa9,0x40,0x21 = fmovsug %fcc0, %f1, %f2
0x85,0xa9,0x00,0x21 = fmovsl %fcc0, %f1, %f2
0x85,0xa8,0xc0,0x21 = fmovsul %fcc0, %f1, %f2
0x85,0xa8,0x80,0x21 = fmovslg %fcc0, %f1, %f2
0x85,0xa8,0x40,0x21 = fmovsne %fcc0, %f1, %f2
0x85,0xaa,0x40,0x21 = fmovse %fcc0, %f1, %f2
0x85,0xaa,0x80,0x21 = fmovsue %fcc0, %f1, %f2
0x85,0xaa,0xc0,0x21 = fmovsge %fcc0, %f1, %f2
0x85,0xab,0x00,0x21 = fmovsuge %fcc0, %f1, %f2
0x85,0xab,0x40,0x21 = fmovsle %fcc0, %f1, %f2
0x85,0xab,0x80,0x21 = fmovsule %fcc0, %f1, %f2
0x85,0xab,0xc0,0x21 = fmovso %fcc0, %f1, %f2
0x85,0x61,0xc8,0x01 = movu %fcc1, %g1, %g2
0x85,0xa9,0x90,0x21 = fmovsg %fcc2, %f1, %f2
0x87,0x78,0x44,0x02 = movrz %g1, %g2, %g3
0x87,0x78,0x48,0x02 = movrlez %g1, %g2, %g3
0x87,0x78,0x4c,0x02 = movrlz %g1, %g2, %g3
0x87,0x78,0x54,0x02 = movrnz %g1, %g2, %g3
0x87,0x78,0x58,0x02 = movrgz %g1, %g2, %g3
0x87,0x78,0x5c,0x02 = movrgez %g1, %g2, %g3
0x87,0xa8,0x44,0xa2 = fmovrsz %g1, %f2, %f3
0x87,0xa8,0x48,0xa2 = fmovrslez %g1, %f2, %f3
0x87,0xa8,0x4c,0xa2 = fmovrslz %g1, %f2, %f3
0x87,0xa8,0x54,0xa2 = fmovrsnz %g1, %f2, %f3
0x87,0xa8,0x58,0xa2 = fmovrsgz %g1, %f2, %f3
0x87,0xa8,0x5c,0xa2 = fmovrsgez %g1, %f2, %f3
0x81,0xcf,0xe0,0x08 = rett %i7+8
0x91,0xd0,0x20,0x05 = ta %icc, %g0 + 5
0x83,0xd0,0x30,0x03 = te %xcc, %g0 + 3
