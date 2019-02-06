include mkrules.mk

TB_TESTERS = verilator_tb/sanity \
			 verilator_tb/spi_simple

TB_VERILOG = verilog/sanity \
			 verilog/spi

TB_OBJS = ${VERILATOR_OBJS} ${TB_TESTERS} verilator_tb/catch_main
$(call add-bin,testbench,${TB_OBJS},$(TB_VERILOG))

tb: ${BUILD_DIR}/bin/testbench
	$^

# testing the sim state machine thing separate from the verilog code
SIM_TEST_OBJS = libsim/test/catch_main libsim/test/testsim
$(call add-bin,libsimtest,$(SIM_TEST_OBJS),)

simtest: ${BUILD_DIR}/bin/libsimtest
	$^
