#pragma once

#include "Lr2SelectStateCache.h"
#include "Lr2SelectVisualState.h"

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <limits>

class Lr2SelectNavigationController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(Lr2SelectVisualState* visualState READ visualState WRITE setVisualState NOTIFY visualStateChanged)
    Q_PROPERTY(Lr2SelectStateCache* stateCache READ stateCache WRITE setStateCache NOTIFY stateCacheChanged)

public:
    explicit Lr2SelectNavigationController(QObject* parent = nullptr);

    QObject* context() const;
    void setContext(QObject* context);

    Lr2SelectVisualState* visualState() const;
    void setVisualState(Lr2SelectVisualState* state);

    Lr2SelectStateCache* stateCache() const;
    void setStateCache(Lr2SelectStateCache* cache);

    Q_INVOKABLE bool refreshFocusedState();
    Q_INVOKABLE bool touchSelection();
    Q_INVOKABLE bool commitLogicalSelection(int index);
    Q_INVOKABLE bool syncCurrentToVisual(int cursorBaseIndex = -1);
    Q_INVOKABLE void applyLr2ScrollDelta(qreal entries, int durationMs, qreal nowMs = -1.0, int currentFixed = std::numeric_limits<int>::min());
    Q_INVOKABLE void scrollBy(qreal entries, int durationMs = -1);
    Q_INVOKABLE void scrollByKey(qreal entries, bool repeated);
    Q_INVOKABLE void decrementViewIndex(bool repeated);
    Q_INVOKABLE void incrementViewIndex(bool repeated);

signals:
    void contextChanged();
    void visualStateChanged();
    void stateCacheChanged();

private:
    int intProperty(const char* name, int fallback = 0) const;
    qreal realProperty(const char* name, qreal fallback = 0.0) const;
    bool boolProperty(const char* name, bool fallback = false) const;
    QVariant variantProperty(const char* name) const;
    void setPropertyIfChanged(const char* name, const QVariant& value) const;
    void incrementProperty(const char* name) const;

    int logicalCount() const;
    int normalizeIndex(int index) const;
    int animatedTopbarFixed() const;
    qreal nearestVisualIndex(int index, qreal anchor) const;
    bool visualMoveActive() const;
    void emitEntryChangeSoundsRequested(int count) const;
    bool beginVisualMove(int durationMs, qreal nowMs);
    bool publishCursorBaseIndex(bool force);
    bool refreshSelectedScoreState();
    void applySelectedScoreState(const QVariantMap& state) const;
    QString chartAssetUrl(const QVariant& chartData, const QVariant& fileName) const;

    QPointer<QObject> m_context;
    QPointer<Lr2SelectVisualState> m_visualState;
    QPointer<Lr2SelectStateCache> m_stateCache;
};


