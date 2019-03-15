#include "../../catch/catch.hpp"

#include "../Simulator.h" // don't look in here

#include <unordered_set>

using namespace libsim;

TEST_CASE("timer", "[libsim]")
{
  struct MyMachine {
    MAKE_STATE(Waiting);
    MAKE_STATE(Triggered);
    States<Waiting, Triggered> state = Uninitialized();

    Events transition(Uninitialized, InitEvent) {
      state = Waiting{};
      return Only{Timeout{100}};
    }

    Events transition(Waiting, Timeout) {
      state = Triggered{};
      return None{};
    }

    bool wasTriggered() const { return std::holds_alternative<Triggered>(state); }
    auto currentState() const { return state; }
  };

  MyMachine m;
  auto sim = SimBuilder<>().add(m).get_sim();
  while (sim.poll()) {};
  CHECK(m.wasTriggered());
}

TEST_CASE("rising edge", "[libsim]")
{
  // a rising edge requires that a value can go from "low" to "hi"
  // we need some way to tell the event what is "low" and what is "hi"

  struct MyMachine {
    MAKE_STATE(Waiting);
    MAKE_STATE(Triggered);

    MyMachine(bool const* that)
      : state(Uninitialized{})
      , that(that)
    { }

    Events transition(Uninitialized, InitEvent) {
      state = Waiting{};
      return Only{RisingEdge{that}};
    }

    Events transition(Waiting, RisingEdge) {
      state = Triggered{};
      return None{};
    }

    bool wasTriggered() const { return std::holds_alternative<Triggered>(state); }
    auto currentState() const { return state; }

    States<Waiting, Triggered> state;
    bool const*                that;
  };

  bool value = false; // lifetimes are fun!

  MyMachine m(&value);
  auto sim = SimBuilder<>().add(m).get_sim();

  for (size_t i = 0; i < 100; ++i) REQUIRE(sim.poll());

  value = true;

  REQUIRE(sim.poll());
  REQUIRE(!sim.poll()); // nothing left to do
  REQUIRE(m.wasTriggered());
}

TEST_CASE("falling edge", "[libsim]")
{
  struct MyMachine {
    MAKE_STATE(Waiting);
    MAKE_STATE(Triggered);

    MyMachine(bool const* that)
      : state(Uninitialized{})
      , that(that)
    { }

    Events transition(Uninitialized, InitEvent) {
      state = Waiting{};
      return Only{FallingEdge{that}};
    }

    Events transition(Waiting, FallingEdge) {
      state = Triggered{};
      return None{};
    }

    bool wasTriggered() const { return std::holds_alternative<Triggered>(state); }
    auto currentState() const { return state; }

    States<Waiting, Triggered> state;
    bool const*                that;
  };

  bool value = true;

  MyMachine m(&value);
  auto sim = SimBuilder<>().add(m).get_sim();

  for (size_t i = 0; i < 100; ++i) REQUIRE(sim.poll());

  value = false;

  REQUIRE(sim.poll());
  REQUIRE(!sim.poll()); // should be no work left
  REQUIRE(m.wasTriggered());
}

TEST_CASE("bad transition", "[libsim]")
{
  struct MyMachine {
    MAKE_STATE(BadState);
    Events transition(Uninitialized, InitEvent) {
      state = BadState{};
      return Only{Timeout{0}};
    }

    auto currentState() const { return state; }

    // no handler for Timeout when in BadState
    States<BadState> state;
  };

  MyMachine m;
  auto sim = SimBuilder<>().add(m).get_sim();

  auto poll_for_a_while = [&] {
    for (size_t i = 0; i < 100; ++i) sim.poll();
  };

  REQUIRE_THROWS(poll_for_a_while());
}

TEST_CASE("AllOf", "[libsim]")
{
  struct MyMachine {
    MAKE_STATE(Waiting);
    MAKE_STATE(Triggered);

    Events transition(Uninitialized, InitEvent) {
      state           = Waiting{};
      timeout_counter = 0;
      return AllOf{Timeout{100}, Timeout{200}};
    }

    Events transition(Waiting, Timeout) {
      timeout_counter += 1;
      if (timeout_counter == 2) {
        state = Triggered{};
      }
      return None{};
    }

    bool wasTriggered() const { return std::holds_alternative<Triggered>(state); }
    auto currentState() const { return state; }

    size_t                     timeout_counter;
    States<Waiting, Triggered> state = Uninitialized{};

  };

  MyMachine m;
  auto sim = SimBuilder<>().add(m).get_sim();
  while (sim.poll()) { } // must eventually stop
  REQUIRE(m.wasTriggered());
}

TEST_CASE("OneOf", "[libsim]")
{
  struct MyMachine {
    MAKE_STATE(Waiting);
    MAKE_STATE(Triggered);

    Events transition(Uninitialized, InitEvent) {
      state = Waiting{};
      return OneOf{Timeout{100}, Timeout{200}};
    }

    Events transition(Waiting, Timeout) {
      state = Triggered{};
      return None{};
    }

    bool wasTriggered() const { return std::holds_alternative<Triggered>(state); }
    auto currentState() const { return state; }

    States<Waiting, Triggered> state = Uninitialized{};
  };

  MyMachine m;
  auto sim = SimBuilder<>().add(m).get_sim();
  while (sim.poll()) { } // must eventually stop, will throw if we trigger twice
  REQUIRE(m.wasTriggered());
}


TEST_CASE("user id", "[libsim]")
{
  struct MyMachine {
    MAKE_STATE(Waiting);
    MAKE_STATE(Triggered);

    Events transition(Uninitialized, InitEvent) {
      state = Waiting{};
      return AllOf{ Timeout{100, 0xdeadbeef}, Timeout{200, 0xcafebabe} };
    }

    Events transition(Waiting, Timeout t) {
      seen_ids.insert(t.user_id);
      if (seen_ids.count(0xdeadbeef) && seen_ids.count(0xcafebabe)) {
        state = Triggered{};
      }
      return None{};
    }

    bool wasTriggered() const { return std::holds_alternative<Triggered>(state); }
    auto currentState() const { return state; }

    States<Waiting, Triggered> state = Uninitialized{};
    std::unordered_set<uint64_t> seen_ids;
  };

  MyMachine m;
  auto sim = SimBuilder<>().add(m).get_sim();
  while (sim.poll()) { } // must eventually stop, will throw if we trigger twice
  REQUIRE(m.wasTriggered());
}
