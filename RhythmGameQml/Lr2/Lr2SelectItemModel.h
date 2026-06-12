#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell;

class Lr2SelectItemModel : public QAbstractListModel {
	Q_OBJECT
	QML_ELEMENT
	Q_PROPERTY(QVariantList items READ items WRITE setItems NOTIFY itemsChanged)
	Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
	Q_PROPERTY(QVariant currentItem READ currentItem NOTIFY currentItemChanged)
	Q_PROPERTY(QString currentKey READ currentKey NOTIFY currentItemChanged)

public:
	enum ItemKind {
		EmptyKind = 0,
		ChartKind,
		EntryKind,
		CourseKind,
		TableKind,
		LevelKind,
		FolderKind,
		RankingKind,
		UnknownKind
	};
	Q_ENUM(ItemKind)

	enum Roles {
		KeyRole = Qt::UserRole + 1,
		KindRole,
		DisplayTextRole,
		TitleTypeRole,
		BodyTypeRole,
		TitleRole,
		SubtitleRole,
		ArtistRole,
		SubartistRole,
		GenreRole,
		TagRole,
		PathRole,
		Md5Role,
		Sha256Role,
		DirectoryRole,
		PlayLevelRole,
		DifficultyRole,
		KeymodeRole,
		RankRole,
		DurationRole,
		MinBpmRole,
		MaxBpmRole,
		MainBpmRole,
		LampRole,
		ScoreRankRole,
		ScoreRateRole,
		LabelMaskRole,
		FolderLampRole,
		FolderScoreCountsRole,
		FolderDistributionRole,
		FolderGraphLampsRole,
		FolderGraphRanksRole,
		IsChartRole,
		IsEntryRole,
		IsCourseRole,
		IsFolderLikeRole,
		IsRankingEntryRole,
		RawItemRole,
		ChartDataRole,
		ActivationObjectRole
	};
	Q_ENUM(Roles)

	explicit Lr2SelectItemModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	QVariantList items() const;
	void setItems(const QVariantList& items);

	int currentIndex() const;
	void setCurrentIndex(int index);
	QVariant currentItem() const;
	QString currentKey() const;

	Q_INVOKABLE void moveRowTo(int from, int to);
	Q_INVOKABLE void clearFolderSummaries();
	Q_INVOKABLE void setFolderSummary(const QString& key,
									  int lamp,
									  const QVariant& scoreCounts,
									  const QVariant& distribution);
	Q_INVOKABLE void setFolderSummaries(const QVariantMap& lamps,
										const QVariantMap& scoreCounts,
										const QVariantMap& distributions);
	Q_INVOKABLE void clearScoreSummaries();
	Q_INVOKABLE void setScoreSummary(const QString& identifier,
									 int lamp,
									 int scoreRank,
									 double scoreRate);
	Q_INVOKABLE void setScoreSummaries(const QVariantMap& lamps,
									   const QVariantMap& scoreRanks,
									   const QVariantMap& scoreRates);
	bool populateBarCell(int sourceRow, int visualRow, Lr2SelectBarCell* cell) const;

signals:
	void itemsChanged();
	void currentIndexChanged();
	void currentItemChanged();

private:
	struct Item {
		QVariant raw;
		QVariantMap map;
		QString key;
		QString folderKey;
		QString scoreIdentifier;
		ItemKind kind = EmptyKind;
		QString displayText;
		int titleType = 0;
		int bodyType = 0;
		QString title;
		QString subtitle;
		QString artist;
		QString subartist;
		QString genre;
		QString tag;
		QString path;
		QString md5;
		QString sha256;
		QString directory;
		int playLevel = 0;
		int difficulty = 0;
		int keymode = 0;
		int rank = 0;
		int duration = 0;
		double minBpm = 0.0;
		double maxBpm = 0.0;
		double mainBpm = 0.0;
		int lamp = 0;
		int scoreRank = 0;
		double scoreRate = 0.0;
		int labelMask = 0;
		int folderLamp = 0;
		QVariant folderScoreCounts;
		QVariantList folderGraphLamps;
		QVariantList folderGraphRanks;
	};

	struct FolderSummary {
		int lamp = 0;
		QVariant scoreCounts;
		QVariantList graphLamps;
		QVariantList graphRanks;
	};

	Item itemFromVariant(const QVariant& value, int fallbackIndex) const;
	QVariant rawItemAt(int row) const;
	QString keyAt(int row) const;
	void applyFolderSummary(Item& item) const;
	static QString scoreIdentifierFor(const QVariant& value, const QVariantMap& map, ItemKind kind);
	static FolderSummary folderSummaryFromValues(int lamp,
												 const QVariant& scoreCounts,
												 const QVariant& distribution);
	static QVariant fieldValue(const QVariant& value, const QVariantMap& map, const char* name);
	static QVariantList listField(const QVariant& value, const QVariantMap& map, const char* name);
	static QString stringField(const QVariant& value, const QVariantMap& map, const char* name);
	static int intField(const QVariant& value, const QVariantMap& map, const char* name, int fallback = 0);
	static double doubleField(const QVariant& value, const QVariantMap& map, const char* name, double fallback = 0.0);
	static bool boolField(const QVariant& value, const QVariantMap& map, const char* name);
	static ItemKind kindFor(const QVariant& value, const QVariantMap& map);
	static QString displayTextFor(const QVariant& value, const QVariantMap& map, ItemKind kind);
	static QString keyFor(const QVariant& value, const QVariantMap& map, ItemKind kind, int fallbackIndex);
	QVariant roleData(const Item& item, int role) const;
	void replaceItems(const QVariantList& items);
	void updateExistingItems(const QVariantList& items);

	QList<Item> m_items;
	QHash<QString, FolderSummary> m_folderSummaries;
	int m_currentIndex = 0;
};
