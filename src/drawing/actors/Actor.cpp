//
// Created by bobini on 30.07.2022.
//

#include "Parent.h"
auto
drawing::actors::Actor::getParent() const -> std::shared_ptr<Parent>
{
    return parent.lock();
}
auto
drawing::actors::Actor::setParent(const std::shared_ptr<Parent>& newParent)
  -> void
{
    // defensive programming
    if (auto parentPtr = parent.lock()) {
        this->parent = newParent;
        parentPtr->removeChild(shared_from_this());
    } else {
        this->parent = newParent;
    }
}
