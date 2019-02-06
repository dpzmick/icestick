#include "../catch/catch.hpp"
#include "verilog/sanity.hvv"

#include <memory>

TEST_CASE("sanity", "[sanity]")
{
  auto s = std::make_unique<sanity>();
  s->in = 0;
  s->eval();
  REQUIRE(s->out == 1);
}

// figure out how to get traces when tests fail:
// VerilatedVcdC* tfp = new VerilatedVcdC; // vcd is file format
// t->trace(tfp, 99);  // Trace 99 levels of hierarchy
// Verilated::mkdir("logs");
// tfp->open("logs/test_dump.vcd");
