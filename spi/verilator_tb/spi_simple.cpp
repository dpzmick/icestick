#include "../catch/catch.hpp"

#include "verilog/spi.hvv"

TEST_CASE("spi-slave", "[spi-simple]")
{
  SECTION("slave does nothing with only clock")
  {
    // Simulator           sim;
    // IceStick<spi_slave> device;

    // // sets up the clock and stuff
    // sim.add_device(device);

    // // one shot tasks
    // auto oneshot = OneShot([&]() {
    //   device.doSomething();
    // });

    // // each function is called at the top of the event loop
    // // then, the model is evaled
    // auto chain = Chain([&]() {
    //   device.doSomethingElse()
    // })
    // .then([]() {
    //   REQUIRE(something); // should be checked after the next evaluation
    // })
    // .wait(1_second) // if the wait takes a lambda, wait for that amount of time
    // .then([]() {
    // });

    // // if anything fails, throw and explode, this is a testing framework


    // // empty return means next cycle?

    // // even loop order:
    // // 1) run any outstanding tasks (including checks)
    // // 2) evaluate the model

    // auto task = Task([](Simulator& s) {
    //   device.top()->sck = 1;
    //   s.wait(1_second);
    // }).then([]() {
    //   device.top()->sck = 0;
    // }).then([]() {
    //   device.top()->sck = 1;
    // });

    // test.complete();
  }
}
