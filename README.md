# Another SERV

SERV running Another World under Verilator

## Prerequisites

Create a directory to keep all the different parts of the project together. We
will refer to this directory as `$WORKSPACE` from now on. All commands will be run from this directory unless otherwise stated.

## Setup hardware

Install FuseSoC

`pip3 install fusesoc`

Add SERV as a library

`fusesoc library add serv https://github.com/olofk/serv`

Add Another SERV as a library

`fusesoc library add another_serv https://github.com/olofk/another_serv`

The Another SERV repo will now be available in $WORKSPACE/fusesoc_libraries/another_serv. To save some typing, we will refer to that directory as `$ASERV`.

## Setup software

The Servant SoC, on which Another SERV is based, can run the [Zephyr RTOS](https://www.zephyrproject.org). The Servant-specific drivers and BSP is located in the zephyr subdirectory of the SERV repository. In order to use Zephyr on Another SERV, a project directory structure must be set up that allows Zephyr to load the Servant-specific files as a module.

First, the Zephyr SDK and the "west" build too must be installed. The [Zephyr getting started guide](https://docs.zephyrproject.org/latest/getting_started/index.html) describes these steps in more detail.

Assuming that SERV was installed into `$WORKSPACE/fusesoc_libraries/serv` as per the prerequisites, run the following command (which might take a while) to make the workspace also work as a Zephyr workspace.

    west init

Specify the SERV repository as the manifest repository, meaning it will be the main entry point when Zephyr is looking for modules.

    west config manifest.path $SERV

Get the right versions of all Zephyr submodules

    west update

A small hack is needed to pretend we have 2MB RAM available on the simulated board. In `$SERV/zephyr/boards/riscv/service/service.dts`, change the `reg = <0x0 0x80000>;` line to `reg = <0x0 0x200000>;`. There's probably a better way to do this.

Finally, you will ned the data files from Another World, specifically the bank0[1-d] files and memlist.bin from the DOS version of Another World (might work with others). These files needs to be placed in $ASERV/sw/src, where the modified source files for the [Another World bytecode interpreter](https://github.com/fabiensanglard/Another-World-Bytecode-Interpreter) also resides. With these in place, it should now be possible to build the Another World VM for the Servant SoC as a Zephyr application with these commands

    cd fusesoc_libraries/another_serv/sw
    west build -b service

After a successful build, Zephyr will create an elf and a bin file of the application in `build/zephyr/zephyr.{elf,bin}`. The bin file can be converted to a verilog hex file, which in turn can be preloaded to FPGA on-chip memories and run on a target board, or loaded into simulated RAM model when running simulations.

To convert the newly built hello world example into a Verilog hex file, run

    python3 $SERV/sw/makehex.py $ASERV/sw/build/zephyr/zephyr.bin 524288 > aw.hex

524288 is the number of 32-bit words to write and must be at least the size of the application binary. `aw.hex` is the resulting hex file. Running a simulation can now be done with

    fusesoc run --target=sim another_serv --uart_baudrate=55600 --firmware=aw.hex

...and then wait a loooong time until the first frame appears. It takes about 15 minutes on my laptop. Subsequent frames are a bit faster though. Seems to be around 5 seconds between each frame. No input is active, so use Ctrl-C in the terminal to abort the simulation. Enjoy!
