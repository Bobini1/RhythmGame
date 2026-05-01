#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged)
    Q_PROPERTY(QVariant entry READ entry WRITE setEntry NOTIFY entryChanged)
    Q_PROPERTY(bool valid READ isValid WRITE setValid NOTIFY validChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int titleType READ titleType WRITE setTitleType NOTIFY titleTypeChanged)
    Q_PROPERTY(int bodyType READ bodyType WRITE setBodyType NOTIFY bodyTypeChanged)
    Q_PROPERTY(int playLevel READ playLevel WRITE setPlayLevel NOTIFY playLevelChanged)
    Q_PROPERTY(int difficulty READ difficulty WRITE setDifficulty NOTIFY difficultyChanged)
    Q_PROPERTY(int keymode READ keymode WRITE setKeymode NOTIFY keymodeChanged)
    Q_PROPERTY(bool ranking READ isRanking WRITE setRanking NOTIFY rankingChanged)
    Q_PROPERTY(bool chartLike READ isChartLike WRITE setChartLike NOTIFY chartLikeChanged)
    Q_PROPERTY(bool entryLike READ isEntryLike WRITE setEntryLike NOTIFY entryLikeChanged)
    Q_PROPERTY(bool folderLike READ isFolderLike WRITE setFolderLike NOTIFY folderLikeChanged)
    Q_PROPERTY(int lamp READ lamp WRITE setLamp NOTIFY lampChanged)
    Q_PROPERTY(int rank READ rank WRITE setRank NOTIFY rankChanged)

public:
    explicit Lr2SelectBarCell(QObject* parent = nullptr);

    int row() const;
    void setRow(int value);

    QVariant entry() const;
    void setEntry(const QVariant& value);

    bool isValid() const;
    void setValid(bool value);

    QString text() const;
    void setText(const QString& value);

    int titleType() const;
    void setTitleType(int value);

    int bodyType() const;
    void setBodyType(int value);

    int playLevel() const;
    void setPlayLevel(int value);

    int difficulty() const;
    void setDifficulty(int value);

    int keymode() const;
    void setKeymode(int value);

    bool isRanking() const;
    void setRanking(bool value);

    bool isChartLike() const;
    void setChartLike(bool value);

    bool isEntryLike() const;
    void setEntryLike(bool value);

    bool isFolderLike() const;
    void setFolderLike(bool value);

    int lamp() const;
    void setLamp(int value);

    int rank() const;
    void setRank(int value);

signals:
    void rowChanged();
    void entryChanged();
    void validChanged();
    void textChanged();
    void titleTypeChanged();
    void bodyTypeChanged();
    void playLevelChanged();
    void difficultyChanged();
    void keymodeChanged();
    void rankingChanged();
    void chartLikeChanged();
    void entryLikeChanged();
    void folderLikeChanged();
    void lampChanged();
    void rankChanged();

private:
    int m_row = -1;
    QVariant m_entry;
    bool m_valid = false;
    QString m_text;
    int m_titleType = -1;
    int m_bodyType = 0;
    int m_playLevel = 0;
    int m_difficulty = 0;
    int m_keymode = 0;
    bool m_ranking = false;
    bool m_chartLike = false;
    bool m_entryLike = false;
    bool m_folderLike = false;
    int m_lamp = 0;
    int m_rank = 0;
};
