eyescanOTC
==========

This package has 3 directories:
    * hw_platform_0 : Xilinx firmware bitstream files
    * otc_bsp : Board support package configuration
    * otc : C code to run on the microblaze soft-cpu, and python scripts to run on a separate pc.
    
The bitstreams are generated elsewhere, as is the board support package (BSP) configuration.

The otc/src directory contains the C code which runs on top of the microblaze soft-cpu.
monitor.c : monitoring thread
    * periodically checks the board temperature and voltages and saves that information to memory.
network_main_thread.c : networking_thread
    * http server to display information gathered from the monitoring thread
    * echo server which listens for commands and returns relevant responses, calls es_interface.c
es_interface.c : interface to eyescan memoryspace
    * reads and writes to shared eyescan memoryspace.
es_controller.c : main eyescan thread
    * communicates with Finite State Machine (FSM) in firmware using DRP registers
    * reads configuration from shared memoryspace
    * writes output data to shared memoryspace
es_simple_eye_acq.c : eyescan acquisition routine
    * called by eyescan thread
    * initiates data acquisition by FSM
    * check if data acquisition finished
    * increase prescale if insufficient number of errors
    * dump result to shared memory and reconfigure for next sampling point
    
Debug/py_scripts contains python scripts that run on a separate pc.
run_eyescan.py :
    * can be run directly from the command line
    * esrun option runs the data collection, this part has been tested on both linux and windows systems (using mingw32 based bash/python)
        - outputs a tar file containing the raw error rate
    * esplot
        - reads tar file from previous step, outputs nice pdf with plots.
        - requires PyROOT
