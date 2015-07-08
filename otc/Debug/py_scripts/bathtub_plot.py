#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
os.sys.path.append('c:\\root_v5.34.25\\bin')
import ROOT

def fill_graph(fd, gr, factor = 1.0) :
    idx = 0
    for l in fd.readlines() :
        ents = l.strip().split(',')
        if ents[0] == 'Iteration' :
            continue
        if len(ents) == 11 :
            vert_val = int(ents[5])
            horz_val = int(ents[6])
            ber_val = float(ents[10])
        elif len(ents) == 10 :
            vert_val = 0
            horz_val = int(ents[5])
            ber_val = float(ents[9])
        else :
            print 'len(ents)', len(ents) , ents
            continue
        if vert_val != 0 :
            continue
        #horz_val *= (0.5/256.)
        #print horz_val, ber_val
        gr.SetPoint(idx, horz_val, ber_val)
        idx += 1
    return gr

def read_csv_file(fname, title) :
    canv = ROOT.TCanvas('bathtub%s' % fname, 'bathtub%s' % fname)
    graph = ROOT.TGraph()
    graph.SetName('graph%s' % fname)
    graph.SetTitle(fname)
    f = open(fname, 'r')
    fill_graph(f, graph)
    f.close()
    if title :
        graph.SetTitle(title)
    graph.Draw('A*')
    graph.GetXaxis().SetTitle('Horz_offset')
    graph.GetYaxis().SetTitle('ber')
    canv.SetLogy()
    return canv, graph

def do_plot_comparison(fname0, fname1) :
    cn = 'bathtub_comp%s%s' % (fname0.split('/')[-1], fname1.split('/')[-1])
    canv = ROOT.TCanvas(cn, cn)
    graph0 = ROOT.TGraph()
    graph1 = ROOT.TGraph()
    gn0 = 'bathtub_comp_graph%s' % (fname0.split('/')[-1])
    gn1 = 'bathtub_comp_graph%s' % (fname1.split('/')[-1])
    graph0.SetName(gn0)
    graph0.SetTitle(gn0)
    graph1.SetName(gn1)
    f0 = open(fname0, 'r')
    fill_graph(f0, graph0)
    f0.close()
    f1 = open(fname1, 'r')
    fill_graph(f1, graph1)
    f1.close()
    graph0.Draw('A*')
    graph0.GetYaxis().SetRangeUser(1e-10, 1)
    graph0.GetXaxis().SetTitle('Horz_offset')
    graph0.GetYaxis().SetTitle('ber')
    canv.SetLogy()
    graph1.Draw('L')
    return canv, graph0, graph1

def main_single_plot(fn = None, title = None) :
    if not fn :
        return False
    graphs = []
    #print fn
    c, g = read_csv_file(fn, title)
    c.Update()
    c.SaveAs('%s.pdf' % fn)
    #raw_input()
    return True

def bathtub_plot(fn0 = None, fn1 = None, title = None) :
    if not fn1 :
        return main_single_plot(fn0, title)
    c, g0, g1 = do_plot_comparison(fn0, fn1)
    if title :
        g0.SetTitle(title)
    c.Update()
    c.SaveAs('%s%s.pdf' % (fn0.split('/')[-1], fn1.split('/')[-1]))

#if len(os.sys.argv) == 3 and os.sys.argv[2] != '-b' :
    #main_plot_compare()
#else :
    #main_single_plot()

if __name__ == '__main__' :
    csv_file_0 = None
    csv_file_1 = None

    for arg in os.sys.argv :
        if '.csv' in arg:
            if not csv_file_0 :
                csv_file_0 = arg
            else :
                csv_file_1 = arg
    if csv_file_1:
        bathtub_plot(csv_file_0, csv_file_1, '%s_%s' % (csv_file_0.split('/')[-1], csv_file_1.split('/')[-1]))
    else:
        main_single_plot(csv_file_0, '%s' % csv_file_0.split('/')[-1])
