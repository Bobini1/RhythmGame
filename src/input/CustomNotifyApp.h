//
// Created by PC on 16/06/2025.
//

#ifndef CUSTOMNOTIFYAPP_H
#define CUSTOMNOTIFYAPP_H
#include "InputTranslator.h"

#include <QGuiApplication>

namespace input {

class CustomNotifyApp final : public QGuiApplication
{
    Q_OBJECT
    using QGuiApplication::QGuiApplication;
    InputTranslator* inputTranslator = nullptr;

  public:
    void setInputTranslator(InputTranslator* translator);
    auto notify(QObject* receiver, QEvent* event) -> bool override;
};

} // input

#endif // CUSTOMNOTIFYAPP_H
