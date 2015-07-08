# source C:/Users/ddboline/Workspace_2013.3/eyescanOTC_20140907/otc/Debug/tcl_scripts/load_otc.tcl
cd C:/Users/ddboline/Workspace_2013.3/eyescanOTC_20140907/otc/Debug/
connect mb mdm
# wait for command to finish...
gets stdin
stop
rst
dow otc.elf
gets stdin
terminal
run
