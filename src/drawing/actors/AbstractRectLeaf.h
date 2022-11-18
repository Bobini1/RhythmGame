//
// Created by bobini on 18.11.22.
//

#ifndef RHYTHMGAME_ABSTRACTRECTLEAF_H
#define RHYTHMGAME_ABSTRACTRECTLEAF_H

#include "Actor.h"
namespace drawing::actors {
class AbstractRectLeaf : public Actor
{
  public:
    auto setIsWidthManaged(bool isWidthManaged) -> void;
    auto setIsHeightManaged(bool isHeightManaged) -> void;
    auto setMinWidth(float minWidth) -> void;
    auto setMinHeight(float minHeight) -> void;
    [[nodiscard]] auto getIsWidthManaged() const -> bool override;
    [[nodiscard]] auto getIsHeightManaged() const -> bool override;
    [[nodiscard]] auto getMinWidth() const -> float override;
    [[nodiscard]] auto getMinHeight() const -> float override;

  private:
    float minWidth = 0;
    float minHeight = 0;
    bool isWidthManaged = false;
    bool isHeightManaged = false;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_ABSTRACTRECTLEAF_H
