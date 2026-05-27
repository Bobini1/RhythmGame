#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Lr2SelectUpdateController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(QObject* selectContext READ selectContext WRITE setSelectContext NOTIFY selectContextChanged)
    Q_PROPERTY(QObject* runtimeOptions READ runtimeOptions WRITE setRuntimeOptions NOTIFY runtimeOptionsChanged)
    Q_PROPERTY(QObject* skinRuntime READ skinRuntime WRITE setSkinRuntime NOTIFY skinRuntimeChanged)
    Q_PROPERTY(int selectRevision READ selectRevision NOTIFY selectRevisionChanged)
    Q_PROPERTY(int selectDetailRevision READ selectDetailRevision NOTIFY selectDetailRevisionChanged)

public:
    explicit Lr2SelectUpdateController(QObject* parent = nullptr);

    QObject* host() const;
    void setHost(QObject* host);

    QObject* selectContext() const;
    void setSelectContext(QObject* context);

    QObject* runtimeOptions() const;
    void setRuntimeOptions(QObject* options);

    QObject* skinRuntime() const;
    void setSkinRuntime(QObject* runtime);

    int selectRevision() const;
    int selectDetailRevision() const;

    Q_INVOKABLE bool refreshBaseActiveOptions();
    Q_INVOKABLE bool refreshSelectRuntimeActiveOptions();
    Q_INVOKABLE bool refreshGameplayRuntimeActiveOptions();

signals:
    void hostChanged();
    void selectContextChanged();
    void runtimeOptionsChanged();
    void skinRuntimeChanged();
    void selectRevisionChanged();
    void selectDetailRevisionChanged();

private slots:
    void selectRevisionDependencyChanged();

private:
    QVariant invokeRuntimeOptions(const char* method) const;
    QVariant invokeRuntimeOptions(const char* method, const QVariant& arg) const;
    void invokeHostVoid(const char* method) const;
    int contextInt(const char* name, int fallback = 0) const;
    bool hostBool(const char* name, bool fallback = false) const;
    QString hostString(const char* name) const;
    QVariant hostVariant(const char* name) const;
    void setHostPropertyIfChanged(const char* name, const QVariant& value) const;
    void applyRuntimeActiveOptions(const QVariant& value) const;
    bool sameNumberArray(const QVariant& lhs, const QVariant& rhs) const;
    QString numberArrayKey(const QVariant& values) const;
    QVariant normalizedNumberArray(const QVariant& values) const;
    QVariantList mergedNumberArray(const QVariant& first, const QVariant& second) const;
    QVariantList selectStaticOptions(bool includeRankingOption, bool includePanelOption) const;
    QObject* skinModelObject() const;
    bool runtimeOwnsOptionPair(int option) const;
    bool skinUsesOption(int option) const;
    void appendUniqueOption(QVariantList& options, int option) const;
    void appendUniqueSkinOption(QVariantList& options, int option) const;
    void appendDefaultOptionIfMissing(QVariantList& options, const QList<int>& choices, int fallback) const;
    QList<int> numberList(const QVariant& values) const;
    void handleCommittedSelectState();
    void handleSelectRevisionChanged();
    bool updateSelectRevision(bool runSideEffects);
    void setSelectDetailRevision(int revision);
    void reconnectSelectContext();

    QPointer<QObject> m_host;
    QPointer<QObject> m_selectContext;
    QPointer<QObject> m_runtimeOptions;
    QPointer<QObject> m_skinRuntime;
    QMetaObject::Connection m_focusRevisionConnection;
    QMetaObject::Connection m_scoreRevisionConnection;
    int m_selectRevision = 0;
    int m_selectDetailRevision = 0;
};



