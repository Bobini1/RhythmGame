//
// Created by PC on 24/09/2024.
//

#ifndef KEYBOARDINPUTFORWARDER_H
#define KEYBOARDINPUTFORWARDER_H
#include <QObject>
namespace resource_managers {
class InputTranslators;
}
namespace resource_managers {

class KeyboardInputForwarder final : public QObject
{
    Q_OBJECT

    InputTranslators* inputTranslators;

  public:
    explicit KeyboardInputForwarder(InputTranslators* inputTranslators,
                                    QObject* parent = nullptr);
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;
};

} // namespace resource_managers

#endif // KEYBOARDINPUTFORWARDER_H
