#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import numpy as np
import matplotlib
matplotlib.use('PDF')
import pylab as pl

def read_dump_file(fname = 'center_error.txt') :
    es_points = []
    for l in open(fname, 'r').xreadlines() :
        ents = l.split()
        chan = int(ents[0])
        cerr = int(ents[1])
        samp = int(ents[2])

        if cerr == 0 :
            cerr = 1

        es_points.append( [ chan, cerr, samp ] )

    return es_points

def central_error_plot(fname = 'center_error.txt', title = 'Central Bit Error Rate') :
    es_points = np.array( read_dump_file(fname) , dtype=np.float64 )

    ber = es_points[:, 1]/es_points[:, 2]

    bins = np.arange( 0 , 48 )
    pl.clf()
    pl.bar( bins , ber )
    pl.xlim([ 0 , 48 ])
    pl.ylim([0, np.max(ber)+0.1*abs(np.max(ber))])
    pl.xlabel('channel')
    pl.ylabel('bit error rate')
    pl.title(title)
    pl.savefig('central_ber.pdf')
    
    return

def central_error_frequency_plot(fname = 'all.dump', title = 'Central Bit Error Frequency'):
    center_errors = []
    f = open(fname, 'r')
    for line in f:
        ent = map(int, line.replace(':','').split())
        center_errors.append( [ ent[0] , ent[1] , ent[2], ent[-1] ] )
    center_errors = np.array( center_errors )

    bins = np.linspace(0, 256, 256)
    pl.clf()
    if any(center_errors[:,-1] != 0):
        pl.hist( center_errors[center_errors[:, -1] != 0][:, -1], bins=bins )
        pl.xlim(0,256)
        pl.title(title)
        pl.xlabel('central error count')
        pl.ylabel('number of occurances')
    pl.savefig('central_error_freq.pdf')
    
    return

if __name__ == '__main__' :
    title = 'Central Bit Error Rate'
    fname = 'center_error.txt'
    if len(os.sys.argv) > 1 and os.sys.argv[1] != '-b' :
        title = os.sys.argv[1]
    if len(os.sys.argv) > 2 and os.sys.argv[2] != '-b' :
        fname = os.sys.argv[2]
    central_error_plot(fname, title)
    central_error_frequency_plot('all.dump')
