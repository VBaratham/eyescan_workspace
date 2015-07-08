set chan [socket 192.168.1.99 7]
puts $chan "dbgeyescan"
flush $chan
while { 1 } {
    puts [gets $chan]
    flush $chan
    if { [ eof $chan ] } {
        close $chan
        break
    }
}

gets stdin

set chan [socket 192.168.1.99 7]
puts $chan "dbgeyescan 0"
flush $chan
while { 1 } {
	puts [gets $chan]
    if { [ eof $chan ] } {
        close $chan
        break
    }
    flush $chan
}

#set chan [socket 192.168.1.99 7]
#puts $chan "esread 0"
#flush $chan
#set number [gets $chan]
#puts $number
#close $chan

#gets stdin

#set chan [socket 192.168.1.99 7]
#puts $chan "esread 0 $number"
#flush $chan
#puts [gets $chan]
#close $chan
