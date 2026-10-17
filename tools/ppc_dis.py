"""Dev tool: disassemble the dumped guest image. usage: python dis.py <image.bin> <hex addr> <count>"""
import struct, sys, capstone
img = open(sys.argv[1], 'rb').read(); B = 0x82000000
md = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
a, n = int(sys.argv[2], 16), int(sys.argv[3])
for i in range(n):
    ad = a + 4 * i
    ins = list(md.disasm(img[ad - B:ad - B + 4], ad))
    print(f'{ad:08X}: ' + (f'{ins[0].mnemonic} {ins[0].op_str}' if ins else '.long %08X' % struct.unpack_from('>I', img, ad - B)[0]))
