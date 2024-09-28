//
// Created by PC on 24/09/2024.
//

#ifndef KEYBOARDINPUTFORWARDER_H
#define KEYBOARDINPUTFORWARDER_H
#include <QObject>
namespace qml_components {
class ProfileList;
}
namespace resource_managers {

class KeyboardInputForwarder final : public QObject
{
    Q_OBJECT

    qml_components::ProfileList* profileList;

  public:
    explicit KeyboardInputForwarder(qml_components::ProfileList* profileList,
                                    QObject* parent = nullptr);
    auto eventFilter(QObject* watched, QEvent* event) -> bool override;
};

} // namespace resource_managers

#endif // KEYBOARDINPUTFORWARDER_H
