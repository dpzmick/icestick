all: blink.blif

blink.blif: blink.v
	yosys -p "synth_ice40 -blif blink.blif" blink.v

blink.pnr: blink.blif blink.pcf
	arachne-pnr -d 1k -o blink.pnr -p blink.pcf blink.blif

blink.bit: blink.pnr
	icepack blink.pnr blink.bit

.PHONY: prog
prog: blink.bit
	iceprog blink.bit
