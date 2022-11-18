//
// Created by bobini on 13.10.22.
//

#ifndef RHYTHMGAME_ABSTRACTVECTORCOLLECTION_H
#define RHYTHMGAME_ABSTRACTVECTORCOLLECTION_H
#include <memory>
#include <vector>
#include "Parent.h"

namespace drawing::actors {
class Actor;
/**
 * An AbstractVectorCollection is an abstract base class that manages its
 * children in an ordered vector. It supports insertion of any number of
 * children.
 */
class AbstractVectorCollection : public Parent
{
  public:
    using iterator = std::vector<std::shared_ptr<Actor>>::iterator;
    using const_iterator = std::vector<std::shared_ptr<Actor>>::const_iterator;
    using reverse_iterator =
      std::vector<std::shared_ptr<Actor>>::reverse_iterator;
    using const_reverse_iterator =
      std::vector<std::shared_ptr<Actor>>::const_reverse_iterator;
    using size_type = std::vector<std::shared_ptr<Actor>>::size_type;

    void addChild(std::shared_ptr<Actor> actor);
    void removeChild(std::shared_ptr<Actor> actor) override;
    [[nodiscard]] auto operator[](size_type index) const
      -> std::shared_ptr<Actor>;
    [[nodiscard]] auto getSize() const -> size_type;

    [[nodiscard]] auto begin() noexcept -> iterator;
    [[nodiscard]] auto end() noexcept -> iterator;
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    [[nodiscard]] auto cbegin() const noexcept -> const_iterator;
    [[nodiscard]] auto cend() const noexcept -> const_iterator;
    [[nodiscard]] auto rbegin() noexcept -> reverse_iterator;
    [[nodiscard]] auto rend() noexcept -> reverse_iterator;
    [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator;
    [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator;
    [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator;
    [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator;

  private:
    std::vector<std::shared_ptr<Actor>> children;
    /**
     * @brief This function is called whenever a child is added or removed.
     */
};
} // namespace drawing::actors
#endif // RHYTHMGAME_ABSTRACTVECTORCOLLECTION_H
