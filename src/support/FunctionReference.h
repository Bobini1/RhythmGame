//
// Created by PC on 21/12/2022.
//

#ifndef RHYTHMGAME_FUNCTIONREFERENCE_H
#define RHYTHMGAME_FUNCTIONREFERENCE_H

#include <memory>
#include <algorithm>
#include <any>
#include <functional>
namespace support {
class FunctionReference
{
    std::any function;

  public:
    template<typename Return, typename... Args>
    explicit FunctionReference(std::function<Return(Args...)>& function)
      : function(&function){};

    template<typename Return, typename... Args>
    auto operator()(Args&&... args) const -> Return
    {
        return std::invoke(
          *std::any_cast<std::function<void(Args...)>*>(function),
          std::forward<Args>(args)...);
    }

    template<typename Return, typename... Args>
    auto getFunction() const -> std::function<Return(Args...)>
    {
        return std::any_cast<std::function<Return(Args...)>*>(function);
    }

    template<typename Return, typename... Args>
    auto assignFunction(std::function<Return(Args...)>& newFunction) -> void
    {
        *std::any_cast<std::function<Return(Args...)>*>(function) = newFunction;
    }
};
} // namespace support

#endif // RHYTHMGAME_FUNCTIONREFERENCE_H
