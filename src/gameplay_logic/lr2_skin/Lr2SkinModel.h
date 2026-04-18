#pragma once

#include <QAbstractListModel>
#include <QList>
#include "Lr2SkinParser.h"

namespace gameplay_logic::lr2_skin {

class Lr2SkinModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString csvPath READ csvPath WRITE setCsvPath NOTIFY csvPathChanged)
    Q_PROPERTY(QVariantMap settingValues READ settingValues WRITE setSettingValues NOTIFY settingValuesChanged)
    Q_PROPERTY(QVariantList activeOptions READ activeOptions WRITE setActiveOptions NOTIFY activeOptionsChanged)
    Q_PROPERTY(int startInput READ startInput NOTIFY skinMetadataChanged)
    Q_PROPERTY(int sceneTime READ sceneTime NOTIFY skinMetadataChanged)
    Q_PROPERTY(int fadeOut READ fadeOut NOTIFY skinMetadataChanged)
    Q_PROPERTY(int skip READ skip NOTIFY skinMetadataChanged)

public:
    enum Roles {
        TypeRole = Qt::UserRole + 1,
        SrcRole,
        DstsRole
    };

    explicit Lr2SkinModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString csvPath() const;
    void setCsvPath(const QString& path);
    QVariantMap settingValues() const;
    void setSettingValues(const QVariantMap& values);
    QVariantList activeOptions() const;
    void setActiveOptions(const QVariantList& options);
    int startInput() const;
    int sceneTime() const;
    int fadeOut() const;
    int skip() const;

signals:
    void csvPathChanged();
    void settingValuesChanged();
    void activeOptionsChanged();
    void skinMetadataChanged();
    void skinLoaded();

private:
    void loadSkin();
    QList<Lr2Element> m_elements;
    QString m_csvPath;
    QVariantMap m_settingValues;
    QVariantList m_activeOptions;
    int m_startInput = 0;
    int m_sceneTime = 0;
    int m_fadeOut = 0;
    int m_skip = 0;
};

} // namespace gameplay_logic::lr2_skin
