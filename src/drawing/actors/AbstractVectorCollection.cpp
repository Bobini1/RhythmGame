//
// Created by bobini on 13.10.22.
//

#include "AbstractVectorCollection.h"
#include <algorithm>

void
drawing::actors::AbstractVectorCollection::addChild(
  std::shared_ptr<Actor> actor)
{
    addChildImpl(actor);
    actor->setParent(sharedFromBase<Parent>());
    children.push_back(std::move(actor));
}
void
drawing::actors::AbstractVectorCollection::removeChild(
  std::shared_ptr<Actor> actor)
{
    auto position = std::find(children.begin(), children.end(), actor);
    if (position != children.end()) {
        removeChildImpl(actor);
        children.erase(position);
    }
    if (actor->getParent() == sharedFromBase<Parent>()) {
        actor->setParent(nullptr);
    }
}
auto
drawing::actors::AbstractVectorCollection::begin() noexcept -> iterator
{
    return children.begin();
}

auto
drawing::actors::AbstractVectorCollection::end() noexcept -> iterator
{
    return children.end();
}
auto
drawing::actors::AbstractVectorCollection::operator[](size_type index) const
  -> std::shared_ptr<Actor>
{
    return children[index];
}
auto
drawing::actors::AbstractVectorCollection::begin() const noexcept
  -> const_iterator
{
    return children.begin();
}
auto
drawing::actors::AbstractVectorCollection::end() const noexcept
  -> const_iterator
{
    return children.end();
}
auto
drawing::actors::AbstractVectorCollection::cbegin() const noexcept
  -> const_iterator
{
    return children.cbegin();
}
auto
drawing::actors::AbstractVectorCollection::cend() const noexcept
  -> const_iterator
{
    return children.cend();
}
auto
drawing::actors::AbstractVectorCollection::rbegin() noexcept -> reverse_iterator
{
    return children.rbegin();
}
auto
drawing::actors::AbstractVectorCollection::rend() noexcept -> reverse_iterator
{
    return children.rend();
}
auto
drawing::actors::AbstractVectorCollection::rbegin() const noexcept
  -> const_reverse_iterator
{
    return children.rbegin();
}
auto
drawing::actors::AbstractVectorCollection::rend() const noexcept
  -> const_reverse_iterator
{
    return children.rend();
}
auto
drawing::actors::AbstractVectorCollection::crbegin() const noexcept
  -> const_reverse_iterator
{
    return children.crbegin();
}
auto
drawing::actors::AbstractVectorCollection::crend() const noexcept
  -> const_reverse_iterator
{
    return children.crend();
}
void
drawing::actors::AbstractVectorCollection::addChildImpl(
  std::shared_ptr<Actor> actor)
{
}
void
drawing::actors::AbstractVectorCollection::removeChildImpl(
  const std::shared_ptr<Actor>& actor)
{
}
