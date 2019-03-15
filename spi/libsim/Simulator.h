#pragma once

#include <variant>
#include <sstream>
#include <type_traits>
#include <iostream>

// This is not the way I usually write code, I promise!

#define MAKE_STATE(name)                                                        \
  struct name : public libsim::StateBase {                                      \
    using is_state = std::true_type;                                            \
    name() : StateBase(#name) { }                                               \
    bool operator==(name const&) const { return true; }                         \
  };                                                                            \

namespace libsim {

struct StateBase {
  StateBase(char const* h)
    : human_readable(h)
  { }

  char const* human_readable;
};

std::ostream& operator<<(std::ostream& os, StateBase& s)
{
  return (os << s.human_readable);
}

// all state machines must have this state and the InitEvent event handler
MAKE_STATE(Uninitialized);

template <typename... StateList>
using States = std::variant<Uninitialized, StateList...>;

// All events have an ID.
// IDs must be globally unique for the machine (across all types of events)
// If no ID is assigned by the user, the simulator will assign one
// A single event might cancel other events for the same machine (by id) if it
// is completed.
struct EventBase {
  EventBase(char const* h, uint64_t user_id)
    : human_readable(h)
    , event_id(0)
    , canceled(false)
    , user_id(user_id) // optional
  { }

  bool has_id() const      { return (bool)event_id; }
  void set_id(uint64_t id) { event_id = id; }

  char const* human_readable;
  uint64_t    event_id;

  bool                  canceled;
  std::vector<uint64_t> cancel_on_complete;

  uint64_t user_id;
};

std::ostream& operator<<(std::ostream& os, EventBase& e)
{
  return (os << e.human_readable);
}

// Event types cannot be defined by users, only by the library

struct InitEvent : public EventBase {
  using is_event = std::true_type;
  InitEvent(uint64_t id=0) : EventBase("InitEvent", id) {}
  bool satisfied(uint64_t) const { return true; }
};

struct Timeout : public EventBase {
  using is_event = std::true_type;

  Timeout(uint64_t duration, uint64_t event_id=0)
    : EventBase("Timeout", event_id)
    , duration(duration)
  {}

  void start(uint64_t now) { start_time = now; }
  bool satisfied(uint64_t now) const { return now >= start_time + duration; }

  uint64_t duration;
  uint64_t start_time = 0;
};

enum class HiLow {
  Hi,
  Low,
};

template <typename T>
struct DefaultEval;

template<>
struct DefaultEval<bool>
{
  HiLow operator()(void const* ptr) const {
    bool b = *static_cast<bool const*>(ptr);
    if (b) return HiLow::Hi;
    else   return HiLow::Low;
  }
};

template<>
struct DefaultEval<uint8_t>
{
  HiLow operator()(void const* ptr) const {
    uint8_t b = *static_cast<uint8_t const*>(ptr);
    if (b) return HiLow::Hi;
    else   return HiLow::Low;
  }
};

struct RisingEdge : public EventBase {
  using is_event = std::true_type;

  template <typename T>
  RisingEdge(T const* valuePtr,
             uint64_t event_id=0,
             std::function<HiLow(void const*)> eval = DefaultEval<T>{})
    : EventBase("RisingEdge", event_id)
    , ptr((void const*)valuePtr)
    , last(eval(ptr))
    , eval(eval)
  { }

  bool satisfied(uint64_t) {
    HiLow now = eval(ptr);
    if (last == HiLow::Low && now == HiLow::Hi) {
      return true;
    }
    else {
      last = now;
      return false;
    }
  }

  void const*                       ptr;
  HiLow                             last;
  std::function<HiLow(void const*)> eval;
};

struct FallingEdge : public EventBase {
  using is_event = std::true_type;

  template <typename T>
  FallingEdge(T const* valuePtr,
              uint64_t event_id=0,
              std::function<HiLow(void const*)> eval = DefaultEval<T>{})
    : EventBase("FallingEdge", event_id)
    , ptr((void const*)valuePtr)
    , last(eval(ptr))
    , eval(eval)
  { }

  bool satisfied(uint64_t) {
    HiLow now = eval(ptr);
    if (last == HiLow::Hi && now == HiLow::Low) {
      return true;
    }
    else {
      last = now;
      return false;
    }
  }

  void const*                       ptr;
  HiLow                             last;
  std::function<HiLow(void const*)> eval;
};

using SimpleEvent = std::variant<InitEvent,
                                 Timeout,
                                 RisingEdge,
                                 FallingEdge>;

// Does nothing
struct None { };

// Only one event allowed
struct Only { SimpleEvent ev; };

// Fires all of the events in the list
struct AllOf {
  template <typename... Args>
  AllOf(Args&&... args)
   : events({std::forward<Args>(args)...})
  { }

  std::vector<SimpleEvent> events;
};

// Exactly one event can fire, others are canceled if any fires
struct OneOf {
  template <typename... Args>
  OneOf(Args&&... args)
   : events({std::forward<Args>(args)...})
  { }

  std::vector<SimpleEvent> events;
};

using Events = std::variant<None, Only, AllOf, OneOf>;

// hm
template <typename M, typename S, typename E, typename = std::void_t<>>
struct has_transition : std::false_type { };

template <typename M, typename S, typename E>
struct has_transition<M, S, E,
          std::void_t< decltype( std::declval<M>().transition(std::declval<S>(), std::declval<E>()) ) >
  > : std::true_type { };

namespace test {
struct SimpleMachine {
  MAKE_STATE(Bogus);
  Events transition(Uninitialized, InitEvent) {
    return None{};
  }
};

static_assert(
    has_transition<SimpleMachine, Uninitialized, InitEvent>{},
    "!");

static_assert(
    !has_transition<SimpleMachine, SimpleMachine::Bogus, InitEvent>{},
    "!");
} // namespace test

template <typename M, typename S, typename E>
typename std::enable_if<!has_transition<M, S, E>::value, Events>::type
_invoke(M&, S s, E e) {
  static_assert(S::is_state::value);
  static_assert(E::is_event::value);
  std::stringstream ss;
  ss << "Invalid transition! Taking " << e << " from " << s;
  throw std::runtime_error(ss.str());

  // This cannot be checked at compile time as each transition function can
  // return any of the variant types
  // The picked interface can't ever check that no invalid paths through the
  // state machine exist, at compile time, because we can't overload the return
  // types. Even if the interface changed, I'm still not sure there's a way
}

template <typename M, typename S, typename E>
typename std::enable_if<has_transition<M, S, E>::value, Events>::type
_invoke(M& m, S s, E e) {
  static_assert(S::is_state::value);
  static_assert(E::is_event::value);
  return m.transition(s, e);
}

template <typename... MachineList>
class SimBuilder;

template <typename... MachineList>
class Simulator {
public:
  // Returns true if there is more work to do
  // ............ I think
  bool poll()
  {
    if (outstanding_events.empty()) return false;

    std::vector<E> next;
    for (auto e : outstanding_events) { // copy everything
      std::visit([&](auto m) {          // copy reference wrapper
          std::visit([&](auto s) {      // copy state
            bool last_completed = std::visit([this](auto&& ee) -> bool {
                if (ee.canceled) return false;
                return ee.satisfied(this->now_);
            }, e.event);

            if (last_completed) {
              Events agg = std::visit([&](auto ee) -> Events {
                return _invoke(m.get(), s, ee);
              }, e.event); // queue only ever contains Simple Events

              enqueue_many(next, m, agg);

              // do ?something? to cancel whatever needs to be canceled
              // don't ask me how this works
              std::visit([&, this](auto&& ee) {
                for (uint64_t cancel : ee.cancel_on_complete) {
                auto body = [&](auto&& candidate) {
                    auto mptr = [](auto&& m) { return (void*)&(m.get()); };
                    void* cm = std::visit(mptr, candidate.machine);
                    void* em = std::visit(mptr, e.machine);
                    if (cm != em) return;

                    uint64_t cid = std::visit([](auto&& eee) {
                        return eee.event_id;
                    }, candidate.event);

                    if (cid != cancel) return;
                    std::visit([](auto& eee) { eee.canceled = true; }, candidate.event);
                  };

                  for (auto& candidate : outstanding_events) body(candidate);
                  for (auto& candidate : next) body(candidate);
                }
              }, e.event);
            }
            else {
              bool canceled = std::visit([](auto&& ee) -> bool {
                return ee.canceled;
              }, e.event);
              if (!canceled) next.push_back(e);
            }
          }, m.get().currentState());
      }, e.machine);
    }

    now_ += 1; // perhaos the only sensible line of code in this file
    outstanding_events = std::move(next); // why did I do this?
    return true;
  }

private:
  struct E {
    std::variant<MachineList...> machine; // good luck
    SimpleEvent                  event;
  };

  uint64_t                            now_ = 0;
  std::vector<E>                      outstanding_events; // should be priority queue
  std::unordered_map<void*, uint64_t> ids_;               // literally insane

  template <typename Machine>
  int add_init(std::reference_wrapper<Machine> m) {
    std::visit([m, this](auto s){
      Events agg = _invoke(m.get(), s, InitEvent{});
      enqueue_many(this->outstanding_events, m, agg);
    }, m.get().currentState());
    return 0; // hack lol wtf
  }

  template <typename Machine>
  void enqueue_many(std::vector<E>& next, Machine m, Events e) {
    // FIXME rewrite as callable struct w/ overloads to get rid of runtime
    // exception
    std::visit([&](auto&& ee) {
      using T = std::decay_t<decltype(ee)>;
      if constexpr (std::is_same_v<T, None>) {
        // nothing to do!
      }
      else if constexpr (std::is_same_v<T, Only>) {
        enqueue_new(next, m, ee.ev);
      }
      else if constexpr (std::is_same_v<T, AllOf>) {
        for (auto&& eee : ee.events) enqueue_new(next, m, eee);
      }
      else if constexpr (std::is_same_v<T, OneOf>) {
        std::vector<uint64_t> cancel_ids;
        for (auto&& eee : ee.events) {
          cancel_ids.push_back(assign_id(m, eee));
        }

        for (auto&& eee : ee.events) {
          set_cancel_ids(eee, cancel_ids);
          enqueue_new(next, m, eee);
        }
      }
      else {
        // failing at runtime is *exactly* what we want here
        throw std::logic_error("Missing case in enqueue_many");
      }
    }, e);
  }

  template <typename Machine>
  void enqueue_new(std::vector<E>& next, Machine m, SimpleEvent e) {
    assign_id(m, e); // too many code paths

    return std::visit([&, this](auto&& ee) {
      using T = std::decay_t<decltype(ee)>;
      if constexpr (std::is_same_v<T, Timeout>) {
        ee.start(this->now_);
      }

      next.push_back( E{m, ee} );
    }, e);
  }

  // modifies the event because I haven't used this feature of c++ yet
  template <typename Machine>
  uint64_t assign_id(Machine m, SimpleEvent& e) {
    return std::visit([&, this](auto&& ee) {
      if (!ee.has_id()) ee.set_id(next_id_for(m));
      return ee.event_id;
    }, e);
  }

  void set_cancel_ids(SimpleEvent& e, std::vector<uint64_t> const& cancel_ids) {
    // exclude yourself because the user of this function probably couldn't be
    // bothered to? Rewrite the user? No!
    std::visit([&](auto&& ee) {
      for (uint64_t c : cancel_ids) {
        if (c != ee.event_id) ee.cancel_on_complete.push_back(c);
      }
    }, e);
  }

  // We are storing references to machines, so we know they can't be moving
  // around. Use pointers. It's safe I promise
  template <typename Machine>
  uint64_t next_id_for(std::reference_wrapper<Machine> m) {
    void* ptr = &(m.get());
    ids_[ptr] += 1; // zero isn't a valid id, always increment
    return ids_[ptr];
  }

  friend class SimBuilder<MachineList...>;
};

// big hack to hide the types from the user
template <typename... MachineList>
class SimBuilder {
public:
  SimBuilder() {}

  SimBuilder(std::tuple<MachineList...> machines) : machines_(machines) {}

  template <typename Machine>
  SimBuilder<std::reference_wrapper<Machine>, MachineList...> add(Machine& m) &&
  {
    return {std::tuple_cat(std::make_tuple(std::reference_wrapper(m)),
                           std::move(machines_))};
  }

  Simulator<MachineList...> get_sim() const {
    Simulator<MachineList...> ret;

    // sort of a hack for iterator
    std::apply([&ret](auto... ms) {
      auto blah = {ret.add_init(ms)...};
      (void)blah;
    }, machines_);

    return ret;
  }

private:
  std::tuple<MachineList...> machines_;
};

}; // namespace libsim. thank god its over
