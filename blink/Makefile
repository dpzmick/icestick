all: sim

build/blink.blif: blink.v
	yosys -p "synth_ice40 -blif build/blink.blif" blink.v

build:
	mkdir build

build/blink.pnr: build build/blink.blif blink.pcf
	arachne-pnr -d 1k -o build/blink.pnr -p blink.pcf build/blink.blif

build/blink.bit: build build/blink.pnr
	icepack build/blink.pnr build/blink.bit

build/simme: blink.v blink_tb.v
	iverilog -o build/simme $^ -DDUMPFILE="\"build/dump.vcd\""

build/dump.vcd: build/simme
	build/simme

.PHONY: prog
prog: build/blink.bit
	iceprog build/blink.bit

.PHONY: sim
sim: build/dump.vcd

.PHONY: waves
waves: build/dump.vcd
	gtkwave build/dump.vcd
