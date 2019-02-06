.SUFFIXES:
.PHONY: bin
BUILD_DIR ?= build

CXX                := clang++
IFLAGS             := -isystem/usr/share/verilator/include/ -I${BUILD_DIR}
CXXFLAGS           := -std=c++17 -Wall -Wextra -Werror -O3 -c -g ${IFLAGS} \
	                  -Wno-unused-local-typedefs
VERILATOR_CXXFLAGS := -std=c++17 -O3 -c -isystem/usr/share/verilator/include/
LDFLAGS            :=

all: bin

# ---- verilator stuff ----

${BUILD_DIR}/verilator/verilated.o: /usr/share/verilator/include/verilated.cpp
	@mkdir -p $(shell dirname $@)
	${CXX} ${VERILATOR_CXXFLAGS} $< -o $@

${BUILD_DIR}/verilator/verilated_vcd_c.o: /usr/share/verilator/include/verilated_vcd_c.cpp
	@mkdir -p $(shell dirname $@)
	${CXX} ${VERILATOR_CXXFLAGS} $< -o $@

${BUILD_DIR}/verilator/verilated.d: /usr/share/verilator/include/verilated.cpp
	@mkdir -p $(shell dirname $@)
	@touch $@

${BUILD_DIR}/verilator/verilated_vcd_c.d: /usr/share/verilator/include/verilated_vcd_c.cpp
	@mkdir -p $(shell dirname $@)
	@touch $@

# ---- let the fun begin ---

# use extensions of .av and .hvv for "verilog" files
${BUILD_DIR}/%__ALL.av ${BUILD_DIR}/%.hvv: %.v
	# Creating verilator obj $@
	@mkdir -p $(shell dirname $@)
	verilator -cc --trace --prefix $(shell basename $*) $< --Mdir $(shell dirname $@) -CFLAGS "${VERILATOR_CXXFLAGS}" -O3
	make -C $(shell dirname $@) -f $(shell basename $*).mk
	cp -p ${BUILD_DIR}/$*__ALL.a ${BUILD_DIR}/$*__ALL.av
	cp -p ${BUILD_DIR}/$*.h      ${BUILD_DIR}/$*.hvv

# -MG is important because headers might not all exist
${BUILD_DIR}/%.d: %.cpp
	# Creating deps $@
	@mkdir -p $(shell dirname $@)
	${CXX} ${CXXFLAGS} $< -MM -MG -MT $(patsubst ${BUILD_DIR}/%.d,${BUILD_DIR}/%.o,$@) -o $@
	@# rewrite the hvv files included to be in build dir, since they are generated
	sed -i -E 's/[^ ]+hvv/${BUILD_DIR}\/&/gm' $@
	@# if the compiler picked up the actual file (because it already existed), we
	@# just rewrote it poorly. Undo that..
	sed -i -E 's/${BUILD_DIR}\/${BUILD_DIR}([^ ]+hvv)/${BUILD_DIR}\/\1/gm' $@
	@# this is insane! FIXME is there a better set of g++ flags?

${BUILD_DIR}/%.o: %.cpp
	# Creating obj $@
	@mkdir -p $(shell dirname $@)
	${CXX} ${CXXFLAGS} $< -o $@

${BUILD_DIR}/%.CPP: %.cpp
	# Creating obj $@
	@mkdir -p $(shell dirname $@)
	cpp ${IFLAGS} $< -o $@

${BUILD_DIR}/bin/%:
	# Creating binary $@
	@mkdir -p ${BUILD_DIR}/bin/
	${CXX} ${LDFLAGS} $^ -o $@

# $(1): name of binary
# $(2): list of objects to link in
# $(3): list of verilog modules to link in
# include all of the dependency files explicitly so that make attempts to create them then immediately include them
define _add-bin
include $(foreach obj,${2},$(patsubst %,${BUILD_DIR}/%.d,${2}))
${BUILD_DIR}/bin/${1}: $(foreach obj,${2},$(patsubst %,${BUILD_DIR}/%.o,${obj})) $(foreach obj,${3},$(patsubst %,${BUILD_DIR}/%__ALL.av,${obj}))
bin: ${BUILD_DIR}/bin/${1}
endef
add-bin = $(eval $(call _add-bin,${1},${2},${3}))

# objects needed to link against verilator
VERILATOR_OBJS = verilator/verilated verilator/verilated_vcd_c
