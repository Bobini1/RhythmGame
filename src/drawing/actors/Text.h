//
// Created by bobini on 11.11.22.
//

#ifndef RHYTHMGAME_TEXT_H
#define RHYTHMGAME_TEXT_H
#include "AbstractRectLeaf.h"
#include <SFML/Graphics/Text.hpp>

namespace drawing::actors {
/**
 * @brief Text is a leaf actor that displays a string of text.
 *
 * Text holds a non-owning pointer to its font.
 *
 * The best method to change the size of text is setCharacterSize().
 * setWidth() and setHeight() will stretch the text.
 */
class Text : public AbstractRectLeaf
{
    sf::Text text;
    sf::Transform transform;

  public:
    static auto make() -> std::shared_ptr<Text>;
    static auto make(const std::string& text,
                     const sf::Font& font,
                     unsigned int size = defaultFontSize)
      -> std::shared_ptr<Text>;
    static constexpr unsigned int defaultFontSize = 30;
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto setTransform(sf::Transform transform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    [[nodiscard]] auto getWidth() const -> float override;
    [[nodiscard]] auto getHeight() const -> float override;

    void setFillColor(const sf::Color& color);
    void setOutlineColor(const sf::Color& color);
    void setOutlineThickness(float thickness);
    [[nodiscard]] auto getFillColor() const -> const sf::Color&;
    [[nodiscard]] auto getOutlineColor() const -> const sf::Color&;
    [[nodiscard]] auto getOutlineThickness() const -> float;

    void setText(const std::string& string);
    void setFont(const sf::Font& font);
    void setCharacterSize(unsigned int size);
    void setStyle(sf::Uint32 style);
    void setLineSpacing(float spacingFactor);
    void setLetterSpacing(float spacingFactor);
    [[nodiscard]] auto getText() const -> const std::string;
    [[nodiscard]] auto getFont() const -> const sf::Font*;
    [[nodiscard]] auto getCharacterSize() const -> unsigned int;
    [[nodiscard]] auto getStyle() const -> sf::Uint32;
    [[nodiscard]] auto getLineSpacing() const -> float;
    [[nodiscard]] auto getLetterSpacing() const -> float;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    Text() = default;
    Text(const std::string& text,
         const sf::Font& font,
         unsigned int size = defaultFontSize);

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_TEXT_H
