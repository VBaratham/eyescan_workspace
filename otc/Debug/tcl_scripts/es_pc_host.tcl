 # #####################################################################################################
# Eye Scan with MicroBlaze
# TCL functions to execute test
# #####################################################################################################

# #####################################################################################################
# Function: es_host_write_mb
# Description: Wrapper around XMD's mwr function for writing into MicroBlaze IO bus.
#
# Parameters:
#     base_addr: base address
#     offset: address offset from base address
#     val: data to be written
#     data_size: size of data to be written (w=word, h=halfword)
#
# Returns: read-back value
# #####################################################################################################

proc send_socket {options} {
    set ip_address 192.168.1.99
    set ip_port 7

    set retval $options
    set chan [socket $ip_address $ip_port]
    puts $chan "$options"
    flush $chan
    set retval [gets $chan]
    close $chan
    
    return $retval
}

proc mwr {options} {
    return [send_socket "mwr $options"]
}

proc mrd {options} {
    return [send_socket "mrd $options"]
}

proc esinit {options} {
	puts "esinit $options"
    return [send_socket "esinit $options"]
}

proc esread {options} {
    return [send_socket "esread $options"]
}

proc esdone {options} {
    return [send_socket "esdone $options"]
}

proc esdisable {options} {
	puts "esdisable $options"
    return [send_socket "esdisable $options"]
}

proc globalinit {{options ""}} {
    return [send_socket "globalinit"]
}

proc es_host_write_mb {base_addr offset val {data_size w} } {
    mwr [expr {$base_addr + $offset}] $val 1 $data_size
    set retval [mrd [expr {$base_addr + $offset}] 1 $data_size]
    return $retval
}

# #####################################################################################################
# Function: es_host_read_mb
# Description: Wrapper around XMD's mrd function for reading from MicroBlaze IO bus.
#
# Parameters:
#     base_addr: base address
#     offset: address offset from base address
#     len: number of words to read
#
# Returns: read value
# #####################################################################################################
proc es_host_read_mb {base_addr offset len} {
    
    set retval [mrd [expr {$base_addr + $offset}] $len]
    return $retval
}

# #####################################################################################################
# Function: es_check_range
# Description: Checks user input parameters to ensure they are within expected range.
#
# Parameters: (Note: all arrays must have length of 4, with array names correponding to lane names 0,1,2,3)
#     test_ch_a: array reference to indicate if a lane is being scanned
#     horz_step_a: array reference to indicate horizontal scan step size
#     vert_step_a: array reference to indicate vertical scan step size
#     lpm_mode_a: array reference to indicate equalizer mode 
#     rate_a: array reference to indicate rate mode (e.g. full-rate, half-rate, etc)
#     max_prescale_a: array reference to indicate maximum prescale allowed (for dynamic prescaling)
#     data_width_a: array reference to indicate parallel data width
#
# Returns: 1 if all parameters are within range. 0 if any parameter is out of range.
# 
# #####################################################################################################
proc es_host_check_range {test_ch_a horz_step_a vert_step_a lpm_mode_a rate_a max_prescale_a data_width_a} {
    upvar $test_ch_a  test_ch
    upvar $horz_step_a horz_step
    upvar $vert_step_a vert_step
    upvar $lpm_mode_a lpm_mode
    upvar $rate_a rate
    upvar $max_prescale_a max_prescale
    upvar $data_width_a data_width
    
    
    foreach curr_ch [array names test_ch] {
        set entries [list $curr_ch $test_ch($curr_ch) $horz_step($curr_ch) $vert_step($curr_ch) $lpm_mode($curr_ch) $rate($curr_ch) $max_prescale($curr_ch) $data_width($curr_ch)]
        foreach curr_entry $entries {
            if {[string is integer -strict $curr_entry] == 0} {
                puts "ERROR. All entries must be integer. $curr_entry is not."
                return 0
            }
        }
    }
    set valid_rates {32 64 128 256 512} 
    set valid_widths {16 20 32 40}
    
    foreach curr_ch [array names test_ch] {
        if { $curr_ch > 48 || $curr_ch < 0 } {
            puts "ERROR. test_ch array key/name (channel name) must be 0,1,2,or 3 (Value given $curr_ch)"
            return 0
        }
        
        if { $test_ch($curr_ch) > 1 || $test_ch($curr_ch) < 0 } {
            puts "ERROR. test_ch value must be 0 or 1 (Value given $test_ch($curr_ch))"
            return 0
        }        
        if { $horz_step($curr_ch) < 1 || $horz_step($curr_ch) > $rate($curr_ch) } {
            puts "ERROR. Horizontal step size must be > 0 and <= $rate($curr_ch) (Value given $horz_step($curr_ch))"
            return 0
        }
        
        if { $vert_step($curr_ch) < 1 || $vert_step($curr_ch) > 127 } {
            puts "ERROR. Vertical step size must be > 0 and <= 127 (Value given $vert_step($curr_ch))"
            return 0
        }
        
        if { $lpm_mode($curr_ch) < 0 || $lpm_mode($curr_ch) > 1 } {
            puts "ERROR. LPM mode must be 0 or 1 (Value given $lpm_mode($curr_ch))"
            return 0
        }
        
        if { [lsearch $valid_rates $rate($curr_ch)] == -1 } {
            puts "ERROR. Valid rates are $valid_rates (Value given $rate($curr_ch))"
            return 0
        }
        
        if {$max_prescale($curr_ch) < 0 || $max_prescale($curr_ch) > 31} {
            puts "ERROR. Prescale must be >=0 and <=31 (Value given $max_prescale($curr_ch))"
            return 0
        }
        
        if { [lsearch $valid_widths $data_width($curr_ch)] == -1 } {
            puts "ERROR. Valid widths are $valid_widths (Value given $data_width($curr_ch))"
            return 0
        }
    }
    
    return 1
}

# #####################################################################################################
# Function: es_host_run
# Description: Runs the eye scan
# 
# Parameters: (Note: all arrays must have length of 4, with array names correponding to lane names 0,1,2,3)
#     test_ch_a: array reference to indicate if a lane is being scanned
#     horz_step_a: array reference to indicate horizontal scan step size
#     vert_step_a: array reference to indicate vertical scan step size
#     lpm_mode_a: array reference to indicate equalizer mode 
#     rate_a: array reference to indicate rate mode (e.g. full-rate, half-rate, etc)
#     max_prescale_a: array reference to indicate maximum prescale allowed (for dynamic prescaling)
#     data_width_a: array reference to indicate parallel data width
#     out_file_a: array reference to indicate file names for output raw dump
#
# Returns: 1 when test is completed
# 
# #####################################################################################################
proc es_host_run {test_ch_a horz_step_a vert_step_a lpm_mode_a rate_a max_prescale_a data_width_a out_file_a} {

    upvar $test_ch_a  test_ch
    upvar $horz_step_a horz_step
    upvar $vert_step_a vert_step
    upvar $lpm_mode_a lpm_mode
    upvar $rate_a rate
    upvar $out_file_a out_file
    upvar $max_prescale_a max_prescale
    upvar $data_width_a data_width

    set valid_entry [es_host_check_range test_ch horz_step vert_step lpm_mode rate max_prescale data_width]
    if {$valid_entry == 0} {
        return 0
    }

    # first do a global reset
    globalinit
    # now write settings to each channel, start dump files
    foreach curr_ch [array names test_ch] {
        if {$test_ch($curr_ch) == 1} {
            puts "Writing settings to channel $curr_ch ..."

            esinit "$curr_ch $max_prescale($curr_ch) $horz_step($curr_ch) $data_width($curr_ch) $vert_step($curr_ch) $lpm_mode($curr_ch) $rate($curr_ch)"
        }
    }

    ### write debugging information to file, after esinit is run, but before actual scan starts
    set outfile [ open "debug.output" w ]
    set chan [socket 192.168.1.99 7]
    puts $chan "dbgeyescan 0"
    flush $chan
    while { 1 } {
        set es_debug [gets $chan]
        flush $chan
        if { [ eof $chan ] } {
            close $chan
            break
        }
        puts $outfile $es_debug
    }
    close $outfile

    
    ### Now run the actual scan
    esinit "run"
    
    set wait_time 10000
    
    while {1} {
        # Poll status for each channel
        puts "Sleeping for $wait_time ms"
        after $wait_time

        set isalldone [esdone "all"]
        if { $isalldone == 0 } {
            set npixels [esread "0"]
            puts "Finished $npixels pixels"
            continue;
        }

        if {$isalldone == 1} {
            set dumpfile [ open "all.dump" w ]
            set chan [socket 192.168.1.99 7]
            puts $chan "esread all"
            while {1} {
                flush $chan
                set es_data [gets $chan]
                if { [ eof $chan ] } {
                    close $chan
                    break
                }
                puts $dumpfile $es_data
            }
            close $dumpfile
            puts "SCAN IS DONE"
            esdisable "all"
            break;
        }
    }
                                
    puts "ALL TEST COMPLETED!"
    return 1

}

# #####################################################################################################
# Function: process_es_dump
# Description: Given raw dump file, extracts data into given arrays.
# 
# Parameters:
#     in_file: name of raw dump file
#     curr_ch: name of current lane
#     horz_arr_a: reference to array to be populated with horizontal offset values
#     vert_arr_a: reference to array to be populated with vertical offset values
#     utsign_arr_a: reference to array to be populated with ut-sign values
#     sample_count_arr_a: reference to array to be populated with sample count values
#     error_count_arr_a: reference to array to be populated with error count values
#     prescale_arr_a: reference to array to be populated with prescale values
#     data width: parallel data width
#
# Returns: none
# 
# #####################################################################################################
proc es_host_process_dump {in_file curr_ch horz_arr_a vert_arr_a utsign_arr_a sample_count_arr_a error_count_arr_a prescale_arr_a data_width} {
    
    upvar $horz_arr_a horz_arr
    upvar $vert_arr_a vert_arr
    upvar $utsign_arr_a utsign_arr
    upvar $sample_count_arr_a sample_count_arr
    upvar $error_count_arr_a error_count_arr
    upvar $prescale_arr_a prescale_arr
    
    set f_in [open $in_file r]

    set count 0
    while {[gets $f_in line] >= 0} {
        if {$line != "" && [regexp {^#} $line] != 1} {
            
            # Remove space characters
            regsub -all "  " $line " " line
            # Remove ":"
            regsub -all ":" $line "" line
            
            # Separate to address and value fields
            #set fields [split $line ":"]
            set fields [split $line " "]
            
            set curr_channel [ lindex $fields 0 ]
            if { $curr_channel != $curr_ch } {
                continue;
            }
            set curr_pixel [ lindex $fields 1 ]
            set horz_offset [ lindex $fields 2 ]
            set vert_offset [ lindex $fields 3 ]
            set error_count [ lindex $fields 4 ]
            set sample_count [ lindex $fields 5 ]
            set prescale [ lindex $fields 6 ]
            set ut_sign [ lindex $fields 7 ]
            
            # Update stored variables
            set sample_count_arr($horz_offset,$vert_offset,$ut_sign) $sample_count
            set error_count_arr($horz_offset,$vert_offset,$ut_sign) $error_count
            set prescale_arr($horz_offset,$vert_offset,$ut_sign) $prescale
            
            # Record horizontal offset, vertical offset, and ut sign values found
            set horz_arr($horz_offset) 1
            set vert_arr($vert_offset) 1
            set utsign_arr($ut_sign) 1
            
        }
    }
    close $f_in
}

# #####################################################################################################
# Function: es_host_gen_csv
# Description: Given arrays containing eye scan data, generates csv file that can be opened in iBERTplotter
#
# Parameters:
#     csv_file: name of output csv file
#     lpm_mode: equalizer mode 
#     vert_step: vertical scan step size
#     horz_arr_a: reference to array populated with horizontal offset values
#     vert_arr_a: reference to array populated with vertical offset values
#     utsign_arr_a: reference to array populated with ut-sign values
#     sample_count_arr_a: reference to array populated with sample count values
#     error_count_arr_a: reference to array populated with error count values
#     prescale_arr_a: reference to array populated with prescale values
#     data width: parallel data width
#     rate: rate mode (e.g. full-rate, half-rate, etc)
#
# Returns: none
# #####################################################################################################
proc es_host_gen_csv {csv_file lpm_mode vert_step horz_arr_a vert_arr_a utsign_arr_a sample_count_arr_a error_count_arr_a prescale_arr_a data_width rate} {

    upvar $horz_arr_a horz_arr
    upvar $prescale_arr_a prescale_arr
    upvar $vert_arr_a vert_arr
    upvar $utsign_arr_a utsign_arr
    upvar $sample_count_arr_a sample_count_arr
    upvar $error_count_arr_a error_count_arr
    
    set f_csv [open $csv_file w]
    
    # Generate CSV Header iBERTplotter
    puts $f_csv "gt type,"
    puts $f_csv "device,"
    puts $f_csv "sw version,"
    set samp_per_ui [expr (2 * $rate) + 1]
    puts $f_csv "samples per ui, $samp_per_ui"
    puts $f_csv "ui width,1"
    puts $f_csv "date,"
    puts $f_csv "time,"
    puts $f_csv "voltage interval,$vert_step"
    puts $f_csv "sweep test settings,RX Sampling Point,,,"
    puts $f_csv "sweep test settings,TX Diff Swing,,,"
    puts $f_csv "sweep test settings,TX Pre-Cursor,,,"
    puts $f_csv "sweep test settings,TX Post-Cursor,,,"
    puts $f_csv "=========================================="
    puts $f_csv "Iteration,Elapsed Time,TX Diff Swing,TX Pre-Cursor,TX Post-Cursor,Voltage,RX Sampling Point(tap),Link,ES_VERT_OFFSET,ES_HORZ_OFFSET,BER"
    
    set ber 0
    set iter 0
    
    foreach curr_vert [lsort -integer [array names vert_arr]] {
        foreach curr_horz [lsort -integer [array names horz_arr]] {
            if {$lpm_mode == 1} {
                # In LPM mode, only process data with ut-sign of 0
                if {[info exists error_count_arr($curr_horz,$curr_vert,0)] == 1} {
                    set curr_err0  $error_count_arr($curr_horz,$curr_vert,0)
                    set curr_samp0 $sample_count_arr($curr_horz,$curr_vert,0)
                    set curr_prescale0 $prescale_arr($curr_horz,$curr_vert,0)
                    
                    # Get total samples by multiplying sample count with data width and 2^(prescale + 1)
                    set curr_tot_samp0 [expr wide($curr_samp0) * (wide($data_width) << (1+$curr_prescale0) )]
                    set ber [format "%.2E" [expr double($curr_err0)/double($curr_tot_samp0)]]
                    
                    # To limit BER floor, assume 1 as minimum number of errors.
                    if {$ber == 0} {
                        set ber [expr 1/double($curr_tot_samp0)]
                    }
                    
                } else {
                    puts $f_csv "Error: X:$curr_horz, Y:$curr_vert, UT:0 data point does not exist!"
                }
            } else {
                # In DFE mode, average BER for ut-sign 0 and 1
                if {[info exists error_count_arr($curr_horz,$curr_vert,0)] == 1 && [info exists error_count_arr($curr_horz,$curr_vert,1)] == 1} {
                    
                    set curr_err0  $error_count_arr($curr_horz,$curr_vert,0)
                    set curr_samp0 $sample_count_arr($curr_horz,$curr_vert,0)
                    set curr_prescale0 $prescale_arr($curr_horz,$curr_vert,0)
                    
                    # Get total samples by multiplying sample count with data width and 2^(prescale + 1)
                    set curr_tot_samp0 [expr wide($curr_samp0) * (wide($data_width) << (1+$curr_prescale0)) ]
                    
                    # Calculate BER for ut-sign of 0
                    set ber0 [expr double($curr_err0)/double($curr_tot_samp0) ]
                    
                    set curr_err1  $error_count_arr($curr_horz,$curr_vert,1)
                    set curr_samp1 $sample_count_arr($curr_horz,$curr_vert,1)
                    set curr_prescale1 $prescale_arr($curr_horz,$curr_vert,1)
                    
                    # Get total samples by multiplying sample count with data width and 2^(prescale + 1)
                    set curr_tot_samp1 [expr wide($curr_samp1) * (wide($data_width) << (1+$curr_prescale1)) ]
                    
                    # Calculate BER for ut-sign of 1
                    set ber1 [expr double($curr_err1)/double($curr_tot_samp1) ]
                    
                    # Calculate final BER
                    set ber [format "%.2E" [expr ($ber0 + $ber1)/2]]
                    
                    if {$ber == 0} {
                        set ber [expr 1/(double($curr_tot_samp0) + double($curr_tot_samp1))]
                    }

                } else {
                    puts $f_csv "Error: X:$curr_horz, Y:$curr_vert data points do not exist or incomplete (not both UT signs present)!"
                }
            }
            
            
            puts $f_csv "$iter,NA,NA,NA,NA,$curr_vert,$curr_horz,NA,NA,NA,$ber"
            
            set iter [expr $iter + 1]

        }
    }
    
    close $f_csv
}

# #####################################################################################################
# Function: es_host_plot_ascii_eye
# Description: Given arrays containing eye scan data, generates a text file containing pass/fail ascii eye
#
# Parameters:
#     f_out: file handle for output file
#     lpm_mode: equalizer mode 
#     horz_arr_a: reference to array populated with horizontal offset values
#     vert_arr_a: reference to array populated with vertical offset values
#     utsign_arr_a: reference to array populated with ut-sign values
#     sample_count_arr_a: reference to array populated with sample count values
#     error_count_arr_a: reference to array populated with error count values
#
# Returns: none
# #####################################################################################################
proc es_host_plot_ascii_eye {f_out lpm_mode horz_arr_a vert_arr_a utsign_arr_a sample_count_arr_a error_count_arr_a} {

    upvar $horz_arr_a horz_arr
    upvar $vert_arr_a vert_arr
    upvar $utsign_arr_a utsign_arr
    upvar $sample_count_arr_a sample_count_arr
    upvar $error_count_arr_a error_count_arr
    
    set curr_pixel ""
    puts "lpm mode $lpm_mode"
    
    foreach curr_col [lsort -integer [array names vert_arr]] {
        foreach curr_row [lsort -integer [array names horz_arr]] {
            if {$lpm_mode == 1} {
                # In LPM mode, only process data with ut-sign of 0
                if {[info exists error_count_arr($curr_row,$curr_col,0)] == 1} {
                    if {$error_count_arr($curr_row,$curr_col,0) == 0} {
                        set curr_pixel " "
                    } else {
                        set curr_pixel "*"
                    }
                } else {
                    puts $f_out "Error: X:$curr_row, Y:$curr_col, UT:0 data point does not exist!"
                }
            } else {
                # In DFE mode, average BER for ut-sign 0 and 1
                if {[info exists error_count_arr($curr_row,$curr_col,0)] == 1 && [info exists error_count_arr($curr_row,$curr_col,1)] == 1} {
                    if {$error_count_arr($curr_row,$curr_col,0) == 0 && $error_count_arr($curr_row,$curr_col,1) == 0} {
                        set curr_pixel " "
                    } else {
                        set curr_pixel "*"
                    }
                } else {
                    puts $f_out "Error: X:$curr_row, Y:$curr_col data points do not exist or incomplete (not both UT signs present)!"
                }
            }
            puts -nonewline $f_out $curr_pixel
            puts -nonewline $curr_pixel

        }
        puts $f_out ""
        puts ""
    }
}

# #####################################################################################################
# Function: es_host_get_hex_char
# Description: Given a string of hex characters, extract number of hex characters indicated by "num_bits"
# starting at bit position indicated by "start_bit". Start bit of zero is LSB.
#
# Parameters:
#     word: input string containing hex word
#     start_bit: starting bit position
#     num_bits: number of bits to return
#
# Returns: sub-string of hex characters
# #####################################################################################################
proc es_host_get_hex_char {word start_bit num_bits} {
    # set fields [split $word {}]
    set len [string length $word]
    # set last_bit [expr $len - 1 - $start_bit]
    # set first_bit [expr $len - $start_bit - $num_bits]
    # set retval [join [lrange $fields $first_bit $last_bit] ""]
    # return $retval

    return [string range $word [expr $len - $start_bit - $num_bits] [expr $len - 1 - $start_bit]]
}
# #####################################################################################################
# Function: es_host_hex_to_dec
# Description: Converts hex to decimal

# Parameters:
#     hex_val: string of hex characters

# Returns: decimal equivalent value
# #####################################################################################################
proc es_host_hex_to_dec {hex_val} {
    return $hex_val
    #return [format %d 0x$hex_val]
}
