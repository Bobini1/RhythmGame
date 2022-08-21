#include <co/co.h>
#include <future>
#include <thread>
#include "state_transitions/WindowStateMachineImpl.h"
#include "wiring/Injector.h"

auto
mainCo() -> int
{
    auto injector = wiring::getInjector();

    auto windowManager =
      injector.create<std::shared_ptr<state_transitions::WindowStateMachine>>();

    std::atomic<bool> finished;
    auto eventManagement = std::jthread{ [&windowManager, &finished] {
        while (!finished.load(std::memory_order_acquire)) {
            windowManager->pollEvents();
        }
    } };
    auto start = std::chrono::high_resolution_clock::now();
    while (windowManager->isOpen()) {
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = now - start;
        start = now;
        windowManager->update(std::chrono::nanoseconds{ delta });
        windowManager->draw();
    }
    finished.store(true, std::memory_order_release);
}

auto
main() -> int
{
    co::WaitGroup waitGroup;
    waitGroup.add();
    int ret{};
    go([&]() { ret = mainCo(); });
    waitGroup.wait();

    return ret;
}
