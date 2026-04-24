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
    Q_PROPERTY(int loadStart READ loadStart NOTIFY skinMetadataChanged)
    Q_PROPERTY(int loadEnd READ loadEnd NOTIFY skinMetadataChanged)
    Q_PROPERTY(int playStart READ playStart NOTIFY skinMetadataChanged)
    Q_PROPERTY(int fadeOut READ fadeOut NOTIFY skinMetadataChanged)
    Q_PROPERTY(int skip READ skip NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList effectiveActiveOptions READ effectiveActiveOptions NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList barRows READ barRows NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList helpFiles READ helpFiles NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantMap mouseCursor READ mouseCursor NOTIFY skinMetadataChanged)
    Q_PROPERTY(QString transColor READ transColor NOTIFY skinMetadataChanged)
    Q_PROPERTY(bool reloadBanner READ reloadBanner NOTIFY skinMetadataChanged)
    Q_PROPERTY(int barCenter READ barCenter NOTIFY skinMetadataChanged)
    Q_PROPERTY(int barAvailableStart READ barAvailableStart NOTIFY skinMetadataChanged)
    Q_PROPERTY(int barAvailableEnd READ barAvailableEnd NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList noteSources READ noteSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList mineSources READ mineSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList lnStartSources READ lnStartSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList lnEndSources READ lnEndSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList lnBodySources READ lnBodySources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList autoNoteSources READ autoNoteSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList autoMineSources READ autoMineSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList autoLnStartSources READ autoLnStartSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList autoLnEndSources READ autoLnEndSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList autoLnBodySources READ autoLnBodySources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList noteDsts READ noteDsts NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList lineSources READ lineSources NOTIFY skinMetadataChanged)
    Q_PROPERTY(QVariantList lineDsts READ lineDsts NOTIFY skinMetadataChanged)

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
    int loadStart() const;
    int loadEnd() const;
    int playStart() const;
    int fadeOut() const;
    int skip() const;
    QVariantList effectiveActiveOptions() const;
    QVariantList barRows() const;
    QVariantList helpFiles() const;
    QVariantMap mouseCursor() const;
    QString transColor() const;
    bool reloadBanner() const;
    int barCenter() const;
    int barAvailableStart() const;
    int barAvailableEnd() const;
    QVariantList noteSources() const;
    QVariantList mineSources() const;
    QVariantList lnStartSources() const;
    QVariantList lnEndSources() const;
    QVariantList lnBodySources() const;
    QVariantList autoNoteSources() const;
    QVariantList autoMineSources() const;
    QVariantList autoLnStartSources() const;
    QVariantList autoLnEndSources() const;
    QVariantList autoLnBodySources() const;
    QVariantList noteDsts() const;
    QVariantList lineSources() const;
    QVariantList lineDsts() const;

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
    QVariantList m_effectiveActiveOptions;
    QVariantList m_barRows;
    QVariantList m_helpFiles;
    QVariantMap m_mouseCursor;
    QString m_transColor = "#000000";
    bool m_reloadBanner = false;
    int m_startInput = 0;
    int m_sceneTime = 0;
    int m_loadStart = 0;
    int m_loadEnd = 0;
    int m_playStart = 2000;
    int m_fadeOut = 0;
    int m_skip = 0;
    int m_barCenter = 0;
    int m_barAvailableStart = 0;
    int m_barAvailableEnd = -1;
    QVariantList m_noteSources;
    QVariantList m_mineSources;
    QVariantList m_lnStartSources;
    QVariantList m_lnEndSources;
    QVariantList m_lnBodySources;
    QVariantList m_autoNoteSources;
    QVariantList m_autoMineSources;
    QVariantList m_autoLnStartSources;
    QVariantList m_autoLnEndSources;
    QVariantList m_autoLnBodySources;
    QVariantList m_noteDsts;
    QVariantList m_lineSources;
    QVariantList m_lineDsts;
};

} // namespace gameplay_logic::lr2_skin
