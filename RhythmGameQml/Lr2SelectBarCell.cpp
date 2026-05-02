#include "Lr2SelectBarCell.h"

Lr2SelectBarCell::Lr2SelectBarCell(QObject* parent) : QObject(parent) {}

int Lr2SelectBarCell::row() const { return m_row; }
void Lr2SelectBarCell::setRow(int value) {
    if (m_row == value) return;
    m_row = value;
    emit rowChanged();
}

QVariant Lr2SelectBarCell::entry() const { return m_entry; }
void Lr2SelectBarCell::setEntry(const QVariant& value) {
    if (m_entry == value) return;
    m_entry = value;
    emit entryChanged();
}

bool Lr2SelectBarCell::isValid() const { return m_valid; }
void Lr2SelectBarCell::setValid(bool value) {
    if (m_valid == value) return;
    m_valid = value;
    emit validChanged();
}

QString Lr2SelectBarCell::text() const { return m_text; }
void Lr2SelectBarCell::setText(const QString& value) {
    if (m_text == value) return;
    m_text = value;
    emit textChanged();
}

int Lr2SelectBarCell::titleType() const { return m_titleType; }
void Lr2SelectBarCell::setTitleType(int value) {
    if (m_titleType == value) return;
    m_titleType = value;
    emit titleTypeChanged();
}

int Lr2SelectBarCell::bodyType() const { return m_bodyType; }
void Lr2SelectBarCell::setBodyType(int value) {
    if (m_bodyType == value) return;
    m_bodyType = value;
    emit bodyTypeChanged();
}

int Lr2SelectBarCell::playLevel() const { return m_playLevel; }
void Lr2SelectBarCell::setPlayLevel(int value) {
    if (m_playLevel == value) return;
    m_playLevel = value;
    emit playLevelChanged();
}

int Lr2SelectBarCell::difficulty() const { return m_difficulty; }
void Lr2SelectBarCell::setDifficulty(int value) {
    if (m_difficulty == value) return;
    m_difficulty = value;
    emit difficultyChanged();
}

int Lr2SelectBarCell::keymode() const { return m_keymode; }
void Lr2SelectBarCell::setKeymode(int value) {
    if (m_keymode == value) return;
    m_keymode = value;
    emit keymodeChanged();
}

bool Lr2SelectBarCell::isRanking() const { return m_ranking; }
void Lr2SelectBarCell::setRanking(bool value) {
    if (m_ranking == value) return;
    m_ranking = value;
    emit rankingChanged();
}

bool Lr2SelectBarCell::isChartLike() const { return m_chartLike; }
void Lr2SelectBarCell::setChartLike(bool value) {
    if (m_chartLike == value) return;
    m_chartLike = value;
    emit chartLikeChanged();
}

bool Lr2SelectBarCell::isEntryLike() const { return m_entryLike; }
void Lr2SelectBarCell::setEntryLike(bool value) {
    if (m_entryLike == value) return;
    m_entryLike = value;
    emit entryLikeChanged();
}

bool Lr2SelectBarCell::isFolderLike() const { return m_folderLike; }
void Lr2SelectBarCell::setFolderLike(bool value) {
    if (m_folderLike == value) return;
    m_folderLike = value;
    emit folderLikeChanged();
}

int Lr2SelectBarCell::lamp() const { return m_lamp; }
void Lr2SelectBarCell::setLamp(int value) {
    if (m_lamp == value) return;
    m_lamp = value;
    emit lampChanged();
}

int Lr2SelectBarCell::rank() const { return m_rank; }
void Lr2SelectBarCell::setRank(int value) {
    if (m_rank == value) return;
    m_rank = value;
    emit rankChanged();
}

void Lr2SelectBarCell::setCore(int row,
                               bool valid,
                               const QString& text,
                               int titleType,
                               int bodyType,
                               int playLevel,
                               int difficulty,
                               int keymode,
                               bool ranking,
                               bool chartLike,
                               bool entryLike,
                               bool folderLike,
                               int lamp,
                               int rank) {
    const bool rowChanged = m_row != row;
    const bool validChanged = m_valid != valid;
    const bool textChanged = m_text != text;
    const bool titleTypeChanged = m_titleType != titleType;
    const bool bodyTypeChanged = m_bodyType != bodyType;
    const bool playLevelChanged = m_playLevel != playLevel;
    const bool difficultyChanged = m_difficulty != difficulty;
    const bool keymodeChanged = m_keymode != keymode;
    const bool rankingChanged = m_ranking != ranking;
    const bool chartLikeChanged = m_chartLike != chartLike;
    const bool entryLikeChanged = m_entryLike != entryLike;
    const bool folderLikeChanged = m_folderLike != folderLike;
    const bool lampChanged = m_lamp != lamp;
    const bool rankChanged = m_rank != rank;

    if (!rowChanged
            && !validChanged
            && !textChanged
            && !titleTypeChanged
            && !bodyTypeChanged
            && !playLevelChanged
            && !difficultyChanged
            && !keymodeChanged
            && !rankingChanged
            && !chartLikeChanged
            && !entryLikeChanged
            && !folderLikeChanged
            && !lampChanged
            && !rankChanged) {
        return;
    }

    m_row = row;
    m_valid = valid;
    m_text = text;
    m_titleType = titleType;
    m_bodyType = bodyType;
    m_playLevel = playLevel;
    m_difficulty = difficulty;
    m_keymode = keymode;
    m_ranking = ranking;
    m_chartLike = chartLike;
    m_entryLike = entryLike;
    m_folderLike = folderLike;
    m_lamp = lamp;
    m_rank = rank;

    if (rowChanged) emit this->rowChanged();
    if (validChanged) emit this->validChanged();
    if (textChanged) emit this->textChanged();
    if (titleTypeChanged) emit this->titleTypeChanged();
    if (bodyTypeChanged) emit this->bodyTypeChanged();
    if (playLevelChanged) emit this->playLevelChanged();
    if (difficultyChanged) emit this->difficultyChanged();
    if (keymodeChanged) emit this->keymodeChanged();
    if (rankingChanged) emit this->rankingChanged();
    if (chartLikeChanged) emit this->chartLikeChanged();
    if (entryLikeChanged) emit this->entryLikeChanged();
    if (folderLikeChanged) emit this->folderLikeChanged();
    if (lampChanged) emit this->lampChanged();
    if (rankChanged) emit this->rankChanged();
}
