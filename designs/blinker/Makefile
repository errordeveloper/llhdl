all: ise/blinker.bit

blinker.lhd: blinker.v
	llhdl-verilog blinker.v

blinker.edf: blinker.lhd
	llhdl-spartan6-map -e blinker.edf -s blinker.sym blinker.lhd

blinker.sym: blinker.edf

blinker-resolved.ucf: blinker.ucf blinker.sym
	llhdl-resolveucf blinker.ucf

ise/blinker.ngd: blinker.edf blinker-resolved.ucf
	cd ise && ngdbuild -uc ../blinker-resolved.ucf ../blinker.edf

ise/blinker.ncd: ise/blinker.ngd
	cd ise && map -ol std -w blinker.ngd

ise/blinker-routed.ncd: ise/blinker.ncd
	cd ise && par -ol std -w blinker.ncd blinker-routed.ncd

ise/blinker.bit: ise/blinker-routed.ncd
	cd ise && bitgen -w blinker-routed.ncd blinker.bit

load: ise/blinker.bit
	jtag load.rc

.PHONY: clean load

clean:
	rm -f blinker.edf blinker.lhd blinker.sym blinker-resolved.ucf
	rm -rf ise/*
