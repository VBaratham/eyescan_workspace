#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import socket
import time

valid_rates = (32, 64, 128, 256, 512)
valid_widths = (16, 20, 32, 40)
NUMBER_OF_CHANNELS = 48

def send_command(ostr, host = '192.168.1.99', portno = 7, timeout = 0.1, outfile = None) :
    import socket
    ''' send string to specified socket '''
    retval = ''
    s = socket.socket()
    try :
        s.connect((host, portno))
    except :
        retval = 'failed to open socket at %s:%d' % (host, portno)
        print retval
        return retval
    s.send('%s\n' % ostr)

    t0 = time.time()
    while True :
        stmp = s.recv(2048)
        if not stmp :
            if timeout :
                time.sleep(0.1)
                t1 = time.time()
                if len(retval) > 0 :
                    break
                if t1 - t0 > timeout :
                    break
            else :
                break
        if not outfile :
            retval = '%s%s' % (retval, stmp)
        else :
            outfile.write(stmp)

    s.close()
    return retval

class board_monitor :
    def __init__(self, uptime = 0, temp = 0, intv = 0, auxv = 0, bramv = 0) :
        self.uptime = uptime
        self.temp = temp
        self.intv = intv
        self.auxv = auxv
        self.bramv = bramv

    def parse_string(self, istr) :
        ents = istr.strip().split()
        self.uptime = int(ents[1])
        self.temp = float(ents[4].replace('C',''))
        self.intv = float(ents[7].replace('V',''))
        self.auxv = float(ents[10].replace('V',''))
        self.bramv = float(ents[13].replace('V',''))

class upod_monitor :
    def __init__(self, uptime = 0, addr = 0x0, status = 0x00, temp = 0., v33 = 0, v25 = 0) :
        self.uptime = uptime
        self.addr = addr
        self.status = status
        self.temp = temp
        self.v33 = v33
        self.v25 = v25

    def parse_string(self, istr) :
        ents = istr.strip().split()
        self.addr = int(ents[1],16)
        self.status = int(ents[4],16)
        self.temp = float(ents[7].replace('C',''))
        self.v33 = float(ents[10].replace('uV',''))
        self.v25 = float(ents[13].replace('uV',''))



def run_es_host(horz_step = 1, vert_step = 8, max_prescale = 8, datawidth = 32, lpm_mode = 0, rate = 32, channels = range(NUMBER_OF_CHANNELS)) :
    if rate not in valid_rates or datawidth not in valid_widths :
        return 0

    for idx in range(0, NUMBER_OF_CHANNELS) :
        if idx in channels:
            print 'Writing settings to channel %d' % idx
            send_command('esinit %s %d %d %d %d %d %d"' % (idx, max_prescale, horz_step, datawidth, vert_step, lpm_mode, rate))
        else:
            print 'Disabling channel %d' % idx
            send_command('esdisable %s' % idx)

    fdbg = open('debug_start.output', 'w')
    fdbg.write(send_command('dbgeyescan'))
    fdbg.write(send_command('dbgeyescan 0'))
    fdbg.close()

    send_command('esinit run')

    board_mon = []
    upod_mon = {}


    while True :
        print 'sleep for 10s'
        time.sleep(10)

        is_all_done = int(send_command('esdone all', timeout = 1).strip())
        if is_all_done == 0 :
            npixels = int(send_command('esread 0', timeout = 1).strip())
            print 'finished %d pixels' % npixels

            ostr = send_command('printtemp')
            bm = board_monitor()
            bm.parse_string(ostr)
            board_mon.append([bm.uptime, bm.temp])

            #ostr = send_command('printupod')
            #for line in ostr.split('\r\n') :
                #um = upod_monitor()
                #um.parse_string(line)
                #um.uptime = bm.uptime
                #if um.addr not in upod_mon :
                    #upod_mon[um.addr] = [um]
                #else :
                    #upod_mon[um.addr].append(um)

            #print bm.uptime, bm.temp
            #um = upod_mon[0x30][-1]
            #print um.uptime, um.temp

            continue
        else :
            print 'SCAN IS DONE'
            dumpf = open('all.dump', 'w')
            send_command('esread all', timeout = 60, outfile = dumpf)
            dumpf.close()
            send_command('esdone')
            break

    tmon = open('temperature_mon.output', 'w')
    for ti, tm in board_mon :
        #print ti,tm
        tmon.write('%d %d\n'% (ti, tm))
    tmon.close()

    fdbg = open('debug_end.output', 'w')
    fdbg.write(send_command('dbgeyescan'))
    fdbg.write(send_command('dbgeyescan 0'))
    fdbg.close()

    print 'ALL TEST COMPLETED!'

    send_command('esdisable all')

    return

def get_pixel_key(hoff, voff, utsign) :
    return '%s %s %s' % (hoff, voff, utsign)

class es_lane :
    def __init__(self, curr_lane, pixels = None) :
        self.curr_lane = curr_lane
        self.pixels = pixels
        self.pixel_dict = {}
        self.horz_vals = set()
        self.vert_vals = set()

    def add_pixel(self, pixel) :
        self.pixels.append(pixel)
        pixel_key = get_pixel_key(pixel.horz_offset, pixel.vert_offset, pixel.ut_sign)
        self.pixel_dict[pixel_key] = pixel
        self.horz_vals.add(pixel.horz_offset)
        self.vert_vals.add(pixel.vert_offset)

    def get_pixel(self, idx = -1, hoff = 0, voff = 0, utsign = 0) :
        if idx >= 0 :
            return self.pixels[idx]
        else :
            return self.pixel_dict[get_pixel_key(hoff,voff,utsign)]

    def horz_values(self) :
        hv = [i for i in self.horz_vals]
        hv.sort()
        return hv

    def vert_values(self) :
        vv = [i for i in self.vert_vals]
        vv.sort()
        return vv

class es_pixel :
    def __init__(self, curr_pixel, horz_offset, vert_offset, error_count, sample_count, prescale, ut_sign, center_error) :
        self.curr_pixel = curr_pixel
        self.horz_offset = horz_offset
        self.vert_offset = vert_offset
        self.error_count = error_count
        self.sample_count = sample_count
        self.prescale = prescale
        self.ut_sign = ut_sign
        self.center_error = center_error

    def print_pixel(self) :
        print 'pixel %d %d %d %d %d %d %d' % (self.curr_pixel, self.horz_offset, self.vert_offset, self.error_count, self.sample_count, self.prescale, self.ut_sign)


def process_es_output(horz_step = 1, vert_step = 8, max_prescale = 8, datawidth = 32, lpm_mode = 0, rate = 40, channels = range(NUMBER_OF_CHANNELS), dumpfile = 'all.dump') :

    all_lanes = {}
    for lane in range(0, NUMBER_OF_CHANNELS) :
        all_lanes[lane] = es_lane(lane, [])

    for line in open(dumpfile, 'r').xreadlines() :
        ents = line.replace(':', '').split()
        for idx in range(0,len(ents)) :
            ents[idx] = int(ents[idx])

        lane = ents[0]
        ln = all_lanes[lane]
        ln.add_pixel(es_pixel(ents[1], ents[2], ents[3], ents[4], ents[5], ents[6], ents[7], ents[8]))

    # write ascii eye diagram
    asciif = open('ascii_eye.txt', 'w')
    cerrf = open('center_error.txt', 'w')
    for lane in range(0, NUMBER_OF_CHANNELS) :

        asciif.write('CHANNEL %d\n' % lane)

        if lane not in channels:
            cerrf.write("%s 0 -1 0\n" % lane)
            continue

        idx = 0

        csvf = open('Ch%d.csv' % lane, 'w')
        csvf.write("gt type,\n")
        csvf.write("device,\n")
        csvf.write("sw version,\n")
        csvf.write("samples per ui, %d" % ((2 * rate) + 1))
        csvf.write("ui width,1\n")
        csvf.write("date,\n")
        csvf.write("time,\n")
        csvf.write("voltage interval,$vert_step\n")
        csvf.write("sweep test settings,RX Sampling Point,,,\n")
        csvf.write("sweep test settings,TX Diff Swing,,,\n")
        csvf.write("sweep test settings,TX Pre-Cursor,,,\n")
        csvf.write("sweep test settings,TX Post-Cursor,,,\n")
        csvf.write("==========================================\n")
        csvf.write("Iteration,Elapsed Time,TX Diff Swing,TX Pre-Cursor,TX Post-Cursor,Voltage,RX Sampling Point(tap),Link,ES_VERT_OFFSET,ES_HORZ_OFFSET,BER\n")

        ln = all_lanes[lane]
        cerr = 0
        tsamp = 0

        for vv in ln.vert_values() :
            for hv in ln.horz_values() :
                err = 0
                ber = 0
                if lpm_mode == 1 :
                    px = ln.get_pixel(hoff = hv, voff = vv, utsign = 0)
                    err = px.error_count
                    samp = px.sample_count
                    pres = px.prescale
                    samp_tot = samp * datawidth << (1 + pres)
                    samp = samp_tot
                    ber = float(err) / float(samp_tot)
                    if err == 0 :
                        ber = 1. / float(samp_tot)
                    cerr += px.center_error
                    tsamp += samp_tot
                else :
                    px0 = ln.get_pixel(hoff = hv, voff = vv, utsign = 0)
                    px1 = ln.get_pixel(hoff = hv, voff = vv, utsign = 1)
                    err0 = px0.error_count
                    err1 = px1.error_count
                    samp0 = px0.sample_count
                    samp1 = px1.sample_count
                    pres0 = px0.prescale
                    pres1 = px1.prescale
                    samp_tot0 = samp0 * datawidth << (1 + pres0)
                    samp_tot1 = samp1 * datawidth << (1 + pres1)
                    ber0 = float(err0) / float(samp_tot0)
                    ber1 = float(err1) / float(samp_tot1)
                    ber = (ber0 + ber1) / 2.
                    err = err0 + err1
                    if err == 0 :
                        ber = 1. / float(samp_tot0 + samp_tot1)
                    cerr += px0.center_error + px1.center_error
                    tsamp += samp_tot0 + samp_tot1
                if err == 0 :
                    asciif.write(' ')
                else :
                    asciif.write('*')
                csvf.write("%d,NA,NA,NA,NA,%d,%d,NA,NA,NA,%.2E\n" % (idx, vv, hv, ber))
                idx += 1
            asciif.write('\n\n')
        csvf.close()
        cerrf.write('%d %d %d %s\n' % (lane, cerr, tsamp, float(cerr)/float(tsamp)))
    asciif.close()
    cerrf.close()

    return


if __name__ == '__main__' :

    horzstep = 1
    vertstep = 8
    maxpresc = 8
    datawidt = 40
    lpm_mode = 0
    rate     = 32

    try :
        horzstep = int(os.sys.argv[1])
        vertstep = int(os.sys.argv[2])
        maxpresc = int(os.sys.argv[3])
        datawidt = int(os.sys.argv[4])
        lpm_mode = int(os.sys.argv[5])
        rate     = int(os.sys.argv[6])
    except :
        #exit(0)
        pass

    run_es_host(horzstep, vertstep, maxpresc, datawidt, lpm_mode, rate)
    process_es_output(horzstep, vertstep, maxpresc, datawidt, lpm_mode, rate)
