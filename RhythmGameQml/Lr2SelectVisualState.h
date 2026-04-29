#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class Lr2SelectVisualState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal visualIndex READ visualIndex WRITE setVisualIndex NOTIFY visualIndexChanged)
    Q_PROPERTY(int logicalCount READ logicalCount WRITE setLogicalCount NOTIFY logicalCountChanged)
    Q_PROPERTY(int scrollDirection READ scrollDirection WRITE setScrollDirection NOTIFY scrollDirectionChanged)
    Q_PROPERTY(int scrollDownDirection READ scrollDownDirection WRITE setScrollDownDirection NOTIFY scrollDownDirectionChanged)
    Q_PROPERTY(int fixed READ fixed NOTIFY fixedChanged)
    Q_PROPERTY(int rawFixed READ rawFixed NOTIFY rawFixedChanged)
    Q_PROPERTY(int baseIndex READ baseIndex NOTIFY baseIndexChanged)
    Q_PROPERTY(int cursorBaseIndex READ cursorBaseIndex NOTIFY cursorBaseIndexChanged)
    Q_PROPERTY(qreal offset READ offset NOTIFY offsetChanged)

public:
    explicit Lr2SelectVisualState(QObject* parent = nullptr);

    qreal visualIndex() const;
    void setVisualIndex(qreal value);

    int logicalCount() const;
    void setLogicalCount(int value);

    int scrollDirection() const;
    void setScrollDirection(int value);

    int scrollDownDirection() const;
    void setScrollDownDirection(int value);

    int fixed() const;
    int rawFixed() const;
    int baseIndex() const;
    int cursorBaseIndex() const;
    qreal offset() const;

signals:
    void visualIndexChanged();
    void logicalCountChanged();
    void scrollDirectionChanged();
    void scrollDownDirectionChanged();
    void fixedChanged();
    void rawFixedChanged();
    void baseIndexChanged();
    void cursorBaseIndexChanged();
    void offsetChanged();
    void logicalPositionChanged();

private:
    void recompute();

    qreal m_visualIndex = 0.0;
    int m_logicalCount = 0;
    int m_scrollDirection = 0;
    int m_scrollDownDirection = 2;
    int m_fixed = 0;
    int m_rawFixed = 0;
    int m_baseIndex = 0;
    int m_cursorBaseIndex = 0;
    qreal m_offset = 0.0;
};
