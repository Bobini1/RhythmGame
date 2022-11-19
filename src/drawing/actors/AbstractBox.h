//
// Created by bobini on 18.11.22.
//

#ifndef RHYTHMGAME_ABSTRACTBOX_H
#define RHYTHMGAME_ABSTRACTBOX_H

#include "AbstractVectorCollection.h"
namespace drawing::actors {
/**
 * @brief AbstractBox is an abstract base class for VBox and HBox.
 * It provides the concept of a size mode, which determines how the size of the
 * box is calculated.
 */
class AbstractBox : public AbstractVectorCollection
{
  public:
    /**
     * @brief The size mode determines how the size of the box is calculated.
     */
    enum class SizeMode
    {
        /**
         * The size of the box does not change.
         */
        Fixed,
        /**
         * The size of the box is the sum of the sizes of its children.
         */
        WrapChildren,
        /**
         * The size of the box is managed by its parent.
         */
        Managed
    };
    auto setHorizontalSizeMode(SizeMode mode) -> void;
    auto getHorizontalSizeMode() const -> SizeMode;
    auto setVerticalSizeMode(SizeMode mode) -> void;
    auto getVerticalSizeMode() const -> SizeMode;
    auto setTopPadding(float padding) -> void;
    auto getTopPadding() const -> float;
    auto setBottomPadding(float padding) -> void;
    auto getBottomPadding() const -> float;
    auto setLeftPadding(float padding) -> void;
    auto getLeftPadding() const -> float;
    auto setRightPadding(float padding) -> void;
    auto getRightPadding() const -> float;
    auto setSpacing(float spacing) -> void;
    auto getSpacing() const -> float;

  private:
    SizeMode verticalSizeMode{};
    SizeMode horizontalSizeMode{};
    float topPadding{};
    float bottomPadding{};
    float leftPadding{};
    float rightPadding{};
    float spacing{};
};
} // namespace drawing::actors
#endif // RHYTHMGAME_ABSTRACTBOX_H
