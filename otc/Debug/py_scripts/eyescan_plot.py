#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
os.sys.path.append('c:\\root_v5.34.25\\bin')
import ROOT


def read_csv_file(fname = None, title = None) :
    canv = ROOT.TCanvas('bathtub%s' % fname, 'bathtub%s' % fname)
    graph = ROOT.TGraph2D()
    graph.SetName('%s' % fname)
    graph.SetTitle(title)
    f = open(fname, 'r')
    idx = 0
    for l in f.readlines() :
        ents = l.strip().split(',')
        if ents[0] == 'Iteration' :
            continue
        if len(ents) == 11 :
            vert_val = int(ents[5])
            horz_val = int(ents[6])
            ber_val = float(ents[10])
        else :
            print 'len(ents)', len(ents)
            continue
        graph.SetPoint(idx, horz_val, vert_val, ber_val)
        idx += 1
    h2 = graph.GetHistogram()
    canv.SetLogz()
    h2.Draw('colz')
    h2.GetXaxis().SetTitle('Horz_offset')
    h2.GetYaxis().SetTitle('Vert_offset')
    return canv, graph

def eyescan_plot(fn = None, title = None) :
    c, g = read_csv_file(fn, title)
    c.Update()
    c.SaveAs('%s.pdf' % fn)
    return

if __name__ == '__main__' :
    csv_file = None
    for arg in os.sys.argv :
        if '.csv' in arg:
            if not csv_file :
                csv_file = arg
    eyescan_plot(csv_file, csv_file)
