#include "../catch/catch.hpp"
#include "../common/common.h"

#include "../libsim/Simulator.h"

#include "verilog/spi.hvv"

#include <verilated_vcd_c.h>
#include <fmt/format.h>
#include <algorithm>

using namespace libsim;

template <typename Module>
class VMachine
{
public:
  MAKE_STATE(Running);
  MAKE_STATE(Terminated);

  VMachine(Module* mod,
           bool const& done,
           uint64_t clock_rate)
    : mod_(mod)
    , done_(&done)
    , clkrt_(clock_rate)
    , now_(0)
    , tracer_(new VerilatedVcdC)
    , state_(Uninitialized{})
  {
    // also should be > 1?
    if (!is_pow_two(clock_rate)) { // probably overly restrictive but whatever
      throw std::runtime_error("clock rate not power of two");
    }

    Verilated::traceEverOn(true);

    // attach tracer, must happen before opening the file for some reason
    mod->trace(tracer_.get(), 99);

    // do the thing
    std::string name = fmt::format("logs/{}.vcd",
        Catch::getResultCapture().getCurrentTestName());
    std::replace(name.begin(), name.end(), ' ', '_');
    Verilated::mkdir("logs");
    tracer_->open(name.c_str());
  }

  ~VMachine() {
    tracer_->close();
  }

  Events transition(Uninitialized, InitEvent) {
    // Poll on a timer until done
    state_ = Running{};
    return OneOf{
      Timeout{clkrt_/2},
      RisingEdge{done_},
    };
  }

  Events transition(Running, Timeout) {
    mod_->clk = ~mod_->clk;
    mod_->eval();
    tracer_->dump(now_);
    now_ += 1;
    return OneOf{
      Timeout{clkrt_},
      RisingEdge{done_},
    };
  }

  Events transition(Running, RisingEdge) {
    // once here, nothing will ever trigger again
    return None{};
  }

  auto currentState() const { return state_; }

private:
  Module*                        mod_;
  bool const*                    done_;
  uint64_t                       clkrt_;
  uint64_t                       now_;
  std::unique_ptr<VerilatedVcdC> tracer_;
  States<Running, Terminated>    state_;
};

TEST_CASE("slave does nothing when not selected", "[spi]")
{
  std::unique_ptr<spi> s(new spi);
  bool                 done(false);
  VMachine<spi>        m(s.get(), done, 1);

  auto sim = SimBuilder<>().add(m).get_sim();

  s->SSEL = 0;

  auto prev_miso = s->MISO;
  for (size_t i = 0; i < 100; ++i) {
    sim.poll();
    REQUIRE(prev_miso == s->MISO);
  }

  // setting done to true doesn't matter
}

namespace t1 {
  // need to consider how to do large sends, buffering somewhere?
  struct Master {
    MAKE_STATE(SendingClockUp);
    MAKE_STATE(SendingClockDown);
    MAKE_STATE(Done);

    Events transition(Uninitialized, InitEvent) {
      s->SSEL = 1; // select the spi module

      curr_bit = 32; // msb first
      state = SendingClockDown{}; // next timeout will trigger a rising edge
      return Only{Timeout{10}};
    }

    Events transition(SendingClockDown, Timeout) {
      if (curr_bit == 0) {
        state = Done{}; // not strictly needed
        return None{};
      }

      s->MOSI = (value & (1 << (curr_bit-1))) >> (curr_bit-1); // post a bit
      s->SCK  = 1; // rising edge

      state     = SendingClockUp{};
      curr_bit -= 1; // msb goes first
      return Only{Timeout{10}};
    }

    Events transition(SendingClockUp, Timeout) {
      s->SCK  = 0;
      s->MOSI = 0;
      state = SendingClockDown{};
      return Only{Timeout{10}};
    }

    Master(spi* s, uint32_t value)
      : s(s)
      , value(value)
    { }

    spi*     s;
    uint32_t value;
    size_t   curr_bit;

    // state machine junk
    auto currentState() const { return state; }
    States<SendingClockUp, SendingClockDown, Done> state;
  };

  struct Slave {
    MAKE_STATE(WaitingForData); // told module to post a result somewhere
    MAKE_STATE(Done);           // got a result

    Events transition(Uninitialized, InitEvent) {
      static_assert(sizeof(s->out) == sizeof(uint32_t), "!");

      CHECK(s->out_ready == 0);
      state = WaitingForData{};
      return OneOf{
        RisingEdge{&(s->out_ready)}, // it worked
        Timeout{1000}                // it failed
      };
    }

    Events transition(WaitingForData, RisingEdge) {
      REQUIRE(s->out == value);
      state = Done{};
      done = true;
      return None{};
    }

    template <typename Any>
    Events transition(Any, Timeout) {
      REQUIRE(!(bool)("timeout")); // test failed, we timedout
      return None{};
    }

    spi*     s;
    uint32_t value;
    bool&    done;

    Slave(spi* s, uint32_t value, bool& done)
      : s(s)
      , value(value)
      , done(done)
    { }

    // state machine junk
    auto currentState() const { return state; }
    States<WaitingForData, Done> state;
  };
} // namespace t1

TEST_CASE("master -> slave, single", "[spi]")
{
  std::unique_ptr<spi> s(new spi);
  bool                 done(false);
  VMachine<spi>        m(s.get(), done, 2);
  t1::Master           master(s.get(), 54321);
  t1::Slave            slave(s.get(), 54321, done);

  auto sim = SimBuilder<>().add(m).add(master).add(slave).get_sim();
  while (!done) sim.poll();
}
