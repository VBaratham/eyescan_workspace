from run_es_host_pc import send_command

if __name__ == '__main__':
    while True:
        command = raw_input("es> ")
        if command in ("quit", "exit", "q"):
            break
        if command == "help":
            print "Available commands: esinit, esread, esdone, esdisable, mwr, mrd, debug, dbgeyescan, initclk, readclk, printupod, iicr, iicw, printtemp, globalinit.\n\nSee es_interface.c for syntax and descriptions."
            continue
        send_command(command)
