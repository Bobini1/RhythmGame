#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class Lr2SkinOptionRules : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Lr2SkinOptionRules(QObject* parent = nullptr);

    Q_INVOKABLE bool isRuntimeOwnedOption(int option) const;

    static bool isRuntimeOwnedOptionValue(int option);
};
