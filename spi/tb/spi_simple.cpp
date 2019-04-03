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

  s->SSEL = 0; // FIXME actually selection works differently than this

  auto prev_miso = s->MISO;
  for (size_t i = 0; i < 100; ++i) {
    sim.poll();
    REQUIRE(prev_miso == s->MISO);
  }

  // setting done to true doesn't matter
}

namespace t1 {
  struct Master {
    MAKE_STATE(SendingClockUp);
    MAKE_STATE(SendingClockDown);
    MAKE_STATE(Done);

    Events transition(Uninitialized, InitEvent) {
      s->SSEL = 1; // select the spi module

      curr_bit = 8; // msb first
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

    spi*    s;
    uint8_t value;
    size_t  curr_bit;

    // state machine junk
    auto currentState() const { return state; }
    States<SendingClockUp, SendingClockDown, Done> state;
  };

  struct Slave {
    MAKE_STATE(WaitingForData);  // told module to post a result somewhere
    MAKE_STATE(WaitingForClock); // waiting for fpga posedge
    MAKE_STATE(Done);            // got a result

    Events transition(Uninitialized, InitEvent) {
      static_assert(sizeof(s->out) == sizeof(uint8_t), "!");

      CHECK(s->out_avail == 0);
      state = WaitingForData{};
      return OneOf{
        RisingEdge{&(s->out_avail)}, // it worked
        Timeout{1000}                // it failed
      };
    }

    Events transition(WaitingForData, RisingEdge) {
      state = WaitingForClock{};
      return Only{
        Timeout{1}  // FIXME hack
      };
    }

    Events transition(WaitingForClock, Timeout) {
      REQUIRE(s->out == value);
      state = Done{};
      done = true;
      return None{};
    }

    Events transition(WaitingForData, Timeout) {
      REQUIRE(!(bool)("timeout")); // test failed, we timedout
      return None{};
    }

    spi*    s;
    uint8_t value;
    bool&   done;

    Slave(spi* s, uint32_t value, bool& done)
      : s(s)
      , value(value)
      , done(done)
    { }

    // state machine junk
    auto currentState() const { return state; }
    States<WaitingForData, WaitingForClock, Done> state;
  };
} // namespace t1

TEST_CASE("master -> slave, single", "[spi]")
{
  std::unique_ptr<spi> s(new spi);
  bool                 done(false);
  VMachine<spi>        m(s.get(), done, 2);
  t1::Master           master(s.get(), 111);
  t1::Slave            slave(s.get(), 111, done);

  auto sim = SimBuilder<>().add(m).add(master).add(slave).get_sim();
  while (!done) sim.poll();
}

namespace t2 {
  struct Master {
    MAKE_STATE(Waiting); // Waiting for the magic sequence to show up
    MAKE_STATE(Done);    // success

    Events transition(Uninitialized, InitEvent) {
      s->SSEL = 1; // select the spi module
      value   = 0; // reset our local cached value

      // slave will start sending "eventually", but we need to keep toggling the
      // clock and consuming bytes
      state = Waiting{};
      return AllOf{
        Timeout{10,   0}, // retrigger the clock
        Timeout{1000, 1}, // fail the test
      };
    }

    Events transition(Waiting, Timeout t) {
      if (t.user_id == 1) {
        REQUIRE(!(bool)"timeout");
        return None{}; // we are all done, this is unreachable
      }

      s->SCK = ~s->SCK;
      if (s->SCK) { // only on rising edge
        value = (value << 1) | (uint8_t)s->MISO;
      }

      // uint8_t n = value;
      // while (n) {
      //   if (n & 1)
      //     printf("1");
      //   else
      //     printf("0");

      //   n >>= 1;
      // }
      // printf("\n");

      if (value == magic) {
        state = Done{};
        done  = true;
        return None{};
      }
      else {
        state = Waiting{};
        return Only{Timeout{10}};
      }
    }

    Events transition(Done, Timeout) {
      return None{}; // already done, ignore the timeout
    }

    Master(spi* s, uint8_t magic, bool& done)
      : s(s)
      , magic(magic)
      , done(done)
    { }

    spi*    s;
    uint8_t value;
    uint8_t magic;
    bool&   done;

    // state machine junk
    auto currentState() const { return state; }
    States<Waiting, Done> state;
  };

  struct Slave {
    MAKE_STATE(WaitingForSend);
    MAKE_STATE(Sent);
    MAKE_STATE(Done);

    Events transition(Uninitialized, InitEvent) {
      static_assert(sizeof(s->out) == sizeof(uint8_t), "!");

      state = WaitingForSend{};
      return OneOf{
        RisingEdge{&(s->send_avail)}, // it worked
        Timeout{1000}                 // it failed
      };
    }

    Events transition(WaitingForSend, RisingEdge) {
      // we send in a single tick of the FPGA clock (then let SPI do its thing)
      s->in      = value;
      s->send_in = 1;

      state = Sent{};
      return Only{Timeout{4}}; // need to wait for clock to go up and down again
    }

    Events transition(Sent, Timeout) {
      s->send_in = 0; // all done

      state = Done{};
      return None{};
    }

    spi*    s;
    uint8_t value;

    Slave(spi* s, uint32_t value)
      : s(s)
      , value(value)
    { }

    // state machine junk
    auto currentState() const { return state; }
    States<WaitingForSend, Sent, Done> state;
  };
} // namespace t1

TEST_CASE("slave -> master, single", "[spi]")
{
  std::unique_ptr<spi> s(new spi);
  bool                 done(false);
  VMachine<spi>        m(s.get(), done, 2);
  t2::Master           master(s.get(), 111, done);
  t2::Slave            slave(s.get(), 111);

  auto sim = SimBuilder<>().add(m).add(master).add(slave).get_sim();
  while (!done) sim.poll();
}
