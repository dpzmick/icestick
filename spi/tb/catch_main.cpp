#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"

#include <verilated.h>

struct VerilatorInitalizer
{
  VerilatorInitalizer()
  {
    // globally enable tracing
    Verilated::traceEverOn(true);
  }
};

static VerilatorInitalizer verilatorInit{};
