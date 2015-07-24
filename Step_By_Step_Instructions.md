eyescanOTC
==========

Step by Step Instructions:

    0. Turn on Crate, make sure JTAG connector is attached, Optical Fiber plugged in, OTC is powered, fan is running and pointed at heatsink, etc.
        * Multimeter is wired in series with the OTC. Make sure it's on and set to ammeter.
        * Wait for green LED on OTC and wait for linux login prompt.
    1. Locate the directory called eyescan_workspace (should be in c:\Users\LArTest\Desktop\)
    2. Open Xilinx SDK 2013.3, choose c:\Users\LArTest\Desktop\eyescan_workspace as workspace directory
    3. Probably helpful to go to Project->Clean to rebuild project
    4. Program FPGA: Xilinx Tools->Program FPGA
        * Check that correct Bitstream and BMM File is used:
            C:\Users\LArTest\Desktop\eyescan_workspace\hw_platform_0\design_1_wrapper_6g4_OK_ch0_20150114.bit
            C:\Users\LArTest\Desktop\eyescan_workspace\hw_platform_0\design_1_wrapper_bd_6g4_OK_ch0_20150114.bmm
        * Other combinations from the same directory will probably work
    5. Run the project: Run->Run.  Also open a browser to http://192.168.1.99 and wait for the System Status page
    6. Open "Git Bash" program on the desktop or windows start menu.
    7. cd into py_scripts directory:
        * cd /c/Users/LArTest/Desktop/eyescan_workspace/otc/Debug/py_scripts
    8. Run the run_eyescan.py script, specifying your own label and whether you wish to run a 1d bathtub or 2d eyescan
        * python run_eyescan.py esrun <label> <1|2> [--channels=<CHANNELS>] [--disable-channels=<DISABLE_CHANNELS>]
	     CHANNELS and DISABLE_CHANNELS are comma separated (no space) lists of numbers. CHANNELS
	     is a list of channels to use, and defaults to all 48. DISABLE_CHANNELS is a list of
	     channels to turn off, and defaults to an empty list. To specify channels 0 through n,
	     just use the number n with no commas ("--channels=n"). To specify the list consisting of
	     only n, use a trailing comma ("--channels=n,").
        * just check SDK console window to make sure there are no reset errors
    9. Run the run_eyescan.py script a second time, this time with the esplot command, specifying the tarball (has the form <label>_<1d|2d>_<date>.tar.gz):
        * python run_eyescan.py esplot <tar.gz file> <1|2>
    10. If everything runs successfully, a summary pdf will appear in the directory:
        C:\Users\LArTest\Desktop\eyescan_workspace\otc\Debug\pdf_files\

OTC Test Suite
==============

Step by Step Instructions:

    1. Locate the directory called OpticalTestCard_20150112 (should be in c:\Users\LArTest\Desktop\)
    2. Open Xilinx SDK 2013.3, choose c:\Users\LArTest\Desktop\OpticalTestCard_20150112 as workspace directory
    3. Probably helpful to go to Project->Clean to rebuild project
    4. Program FPGA: Xilinx Tools->Program FPGA
        * Check that correct Bitstream and BMM File is used:
            C:\Users\LArTest\Desktop\OpticalTestCard_20150112\hw_platform_0\design_1_wrapper.bit
            C:\Users\LArTest\Desktop\OpticalTestCard_20150112\hw_platform_0\design_1_wrapper_bd.bmm
        * Other combinations from the same directory will probably work
    5. Run the project: Run->Run
    6. Open a web browser, navigate to:
        http://192.168.1.99


Vyassa Notes
============

* eyescan commands in eyescanOTC_20140907\otc\src\es_interface.c
