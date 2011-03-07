all: ise/gates.bit

gates.lhd: gates.v
	llhdl-verilog gates.v

gates.edf: gates.lhd
	llhdl-spartan6-map -e gates.edf -s gates.sym gates.lhd

gates.sym: gates.edf

gates-resolved.ucf: gates.ucf gates.sym
	llhdl-resolveucf gates.ucf

ise/gates.ngd: gates.edf gates-resolved.ucf
	cd ise && ngdbuild -uc ../gates-resolved.ucf ../gates.edf

ise/gates.ncd: ise/gates.ngd
	cd ise && map -ol std -w gates.ngd

ise/gates-routed.ncd: ise/gates.ncd
	cd ise && par -ol std -w gates.ncd gates-routed.ncd

ise/gates.bit: ise/gates-routed.ncd
	cd ise && bitgen -w gates-routed.ncd gates.bit

load: ise/gates.bit
	jtag load.rc

.PHONY: clean load

clean:
	rm -f gates.edf gates.lhd gates.sym gates-resolved.ucf
	rm -rf ise/*
