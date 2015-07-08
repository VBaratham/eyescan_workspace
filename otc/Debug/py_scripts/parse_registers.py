#!/usr/bin/python
# -*- coding: utf-8 -*-

import os

regmap_name = {}
regmap_addr = {}
ibert_default = {}

def read_regmap_file(fname = 'register_map.txt') :
    global regmap_name, regmap_addr
    for l in open(fname, 'r').xreadlines() :
        name = l.split()[0].strip()
        regn = l.split()[1]
        regv = int(regn, 16)
        bits = l.split()[2:]
        bit0 = -1
        bit1 = -1
        offset = 0
        if len(bits) == 1 :
            bit0 = int(bits[0])
        elif len(bits) == 2 :
            bit0 = int(bits[0])
            bit1 = int(bits[1])
        elif len(bits) == 3 :
            bit0 = int(bits[0])
            bit1 = int(bits[1])
            offset = int(bits[2])
        if name not in regmap_name :
            regmap_name[name] = [[regv, bit0, bit1, offset]]
        else :
            regmap_name[name].append([regv, bit0, bit1, offset])

    for name, val in regmap_name.items() :
        if len(val) == 1 :
            addr = val[0][0]
            if addr not in regmap_addr :
                regmap_addr[addr] = [name]
            else :
                regmap_addr[addr].append(name)
        else :
            for addr, b0, b1, off in val :
                if addr not in regmap_addr :
                    regmap_addr[addr] = [name]
                else :
                    regmap_addr[addr].append(name)
    return

def read_ibert_default(fname = 'register_default_ibert_640.txt') :
    global ibert_default
    for l in open(fname, 'r').xreadlines() :
        name = l.split()[0].strip()
        regn = l.split()[1]
        if regn[1] == 'x' :
            regv = int(regn, 16)
        elif regn[1] == 'b' :
            regv = int(regn, 2)
        ibert_default[name] = regv


def parse_registers(debug_fname = 'debug.output', regmap_fname = 'register_map.txt', output_fname = 'register_settings.txt', ibert_fname = 'register_default_ibert_640.txt', ibert_comp_outname = 'ibert_comp.txt') :

    outfile = open(output_fname, 'w')
    icfile = open(ibert_comp_outname, 'w')

    read_regmap_file(regmap_fname)
    read_ibert_default(ibert_fname)

    regvals = {}

    for l in open(debug_fname, 'r').xreadlines() :
        if 'Global registers:' in l:
            continue
        if l.find('lane 0') != 0 :
            outfile.write('%-30s %30s\n' % (l.split()[0].upper(), l.split()[1]))
            continue
        addr = int(l.split()[3], 16)
        val = int(l.split()[5], 16)
        regvals[addr] = val

    regkeys = regmap_name.keys()
    regkeys.sort()

    for name in regkeys :
        ibert_val = ibert_default[name]
        if len(regmap_name[name]) == 1 :
            addr, b0, b1, off = regmap_name[name][0]
            if b1 == -1 :
                eyeval = regvals[addr] >> b0 & 0x1
            else :
                left_shift = 15 - b0
                eyeval = ((regvals[addr] << left_shift) & 0xFFFF) >> (b1 + left_shift)
            outfile.write('%-30s %30s\n' % (name, hex(eyeval)))
            if eyeval != ibert_val :
                icfile.write('%-30s %24s %24s\n' % (name, hex(ibert_val), hex(eyeval)))
        else :
            eyeval = 0x0
            for addr, b0, b1, off in regmap_name[name] :
                left_shift = 15 - b0
                eyeval |= (((regvals[addr] << left_shift) & 0xFFFF) >> (b1 + left_shift)) << off
            outfile.write('%-30s %30s\n' % (name, hex(eyeval)))
            if eyeval != ibert_val :
                icfile.write('%-30s %24s %24s\n' % (name, hex(ibert_val), hex(eyeval)))
    return

if __name__ == '__main__' :
    parse_registers()
