source test_es_pc_host.tcl

set horzstep [ lindex $argv 0 ]
set vertstep [ lindex $argv 1 ]
set maxpresc [ lindex $argv 2 ]
set datawdit [ lindex $argv 3 ]
set lpm_mode [ lindex $argv 4 ]
set rate     [ lindex $argv 5 ]

run_test $horzstep $vertstep $maxpresc $datawdit $lpm_mode $rate
