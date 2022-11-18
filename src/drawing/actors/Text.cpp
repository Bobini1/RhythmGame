//
// Created by bobini on 11.11.22.
//

#include "Text.h"
#include "SFML/Graphics/RenderTarget.hpp"

void
drawing::actors::Text::update(std::chrono::nanoseconds delta)
{
}
void
drawing::actors::Text::setTransform(sf::Transform newTransform)
{
    transform = newTransform;
}
auto
drawing::actors::Text::getTransform() const -> sf::Transform
{
    return transform;
}
auto
drawing::actors::Text::getWidth() const -> float
{

    return text.getGlobalBounds().width + text.getGlobalBounds().left;
}
auto
drawing::actors::Text::getHeight() const -> float
{
    return text.getGlobalBounds().height + text.getGlobalBounds().top;
}
void
drawing::actors::Text::draw(sf::RenderTarget& target,
                            sf::RenderStates states) const
{
    states.transform *= transform;
    target.draw(text, states);
}
void
drawing::actors::Text::setWidthImpl(float width)
{
    text.setScale(width / text.getLocalBounds().width, 1);
}
void
drawing::actors::Text::setHeightImpl(float height)
{
    text.setScale(1, height / text.getLocalBounds().height);
}
drawing::actors::Text::Text(const std::string& text,
                            const sf::Font& font,
                            unsigned int size)
{
    this->text.setFont(font);
    this->text.setString(sf::String(text));
    this->text.setCharacterSize(size);
}

void
drawing::actors::Text::setFillColor(const sf::Color& color)
{
    text.setFillColor(color);
}
void
drawing::actors::Text::setOutlineColor(const sf::Color& color)
{
    text.setOutlineColor(color);
}
void
drawing::actors::Text::setOutlineThickness(float thickness)
{
    text.setOutlineThickness(thickness);
}
auto
drawing::actors::Text::getFillColor() const -> const sf::Color&
{
    return text.getFillColor();
}
auto
drawing::actors::Text::getOutlineColor() const -> const sf::Color&
{
    return text.getOutlineColor();
}
auto
drawing::actors::Text::getOutlineThickness() const -> float
{
    return text.getOutlineThickness();
}
void
drawing::actors::Text::setFont(const sf::Font& font)
{
    text.setFont(font);
}
void
drawing::actors::Text::setString(const std::string& string)
{
    text.setString(sf::String(string));
}
void
drawing::actors::Text::setCharacterSize(unsigned int size)
{
    text.setCharacterSize(size);
}
void
drawing::actors::Text::setStyle(sf::Uint32 style)
{
    text.setStyle(style);
}
void
drawing::actors::Text::setLineSpacing(float spacingFactor)
{
    text.setLineSpacing(spacingFactor);
}
void
drawing::actors::Text::setLetterSpacing(float spacingFactor)
{
    text.setLetterSpacing(spacingFactor);
}
auto
drawing::actors::Text::getString() const -> const std::string
{
    return text.getString().toAnsiString();
}
auto
drawing::actors::Text::getFont() const -> const sf::Font&
{
    return *text.getFont();
}
auto
drawing::actors::Text::getCharacterSize() const -> unsigned int
{
    return text.getCharacterSize();
}
auto
drawing::actors::Text::getStyle() const -> sf::Uint32
{
    return text.getStyle();
}
auto
drawing::actors::Text::getLineSpacing() const -> float
{
    return text.getLineSpacing();
}
auto
drawing::actors::Text::getLetterSpacing() const -> float
{
    return text.getLetterSpacing();
}
auto
drawing::actors::Text::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<Text>>(),
             sharedFromBase<Text>() };
}
