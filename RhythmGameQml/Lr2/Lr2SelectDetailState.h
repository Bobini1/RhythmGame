#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QJSValue>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

struct Lr2SelectScoreCountsData {
	int play = 0;
	int clear = 0;
	int fail = 0;
	int noplay = 0;
	int assist = 0;
	int lightAssist = 0;
	int easy = 0;
	int normal = 0;
	int hard = 0;
	int exhard = 0;
	int fc = 0;
	int perfect = 0;
	int max = 0;
	int minBadPoor = 0;
};

class Lr2SelectScoreCounts : public QObject {
	Q_OBJECT
	Q_PROPERTY(int play READ play NOTIFY changed)
	Q_PROPERTY(int clear READ clear NOTIFY changed)
	Q_PROPERTY(int fail READ fail NOTIFY changed)
	Q_PROPERTY(int noplay READ noplay NOTIFY changed)
	Q_PROPERTY(int assist READ assist NOTIFY changed)
	Q_PROPERTY(int lightAssist READ lightAssist NOTIFY changed)
	Q_PROPERTY(int easy READ easy NOTIFY changed)
	Q_PROPERTY(int normal READ normal NOTIFY changed)
	Q_PROPERTY(int hard READ hard NOTIFY changed)
	Q_PROPERTY(int exhard READ exhard NOTIFY changed)
	Q_PROPERTY(int fc READ fc NOTIFY changed)
	Q_PROPERTY(int perfect READ perfect NOTIFY changed)
	Q_PROPERTY(int max READ max NOTIFY changed)
	Q_PROPERTY(int minBadPoor READ minBadPoor NOTIFY changed)

public:
	explicit Lr2SelectScoreCounts(QObject* parent = nullptr);

	int play() const;
	int clear() const;
	int fail() const;
	int noplay() const;
	int assist() const;
	int lightAssist() const;
	int easy() const;
	int normal() const;
	int hard() const;
	int exhard() const;
	int fc() const;
	int perfect() const;
	int max() const;
	int minBadPoor() const;

	bool hasValues() const;
	bool setValues(const Lr2SelectScoreCountsData& values);
	void clearValues();

signals:
	void changed();

private:
	int m_play = 0;
	int m_clear = 0;
	int m_fail = 0;
	int m_noplay = 0;
	int m_assist = 0;
	int m_lightAssist = 0;
	int m_easy = 0;
	int m_normal = 0;
	int m_hard = 0;
	int m_exhard = 0;
	int m_fc = 0;
	int m_perfect = 0;
	int m_max = 0;
	int m_minBadPoor = 0;
};

Q_DECLARE_METATYPE(Lr2SelectScoreCounts*)

struct Lr2SelectScoreStatsData {
	bool valid = false;
	int pg = 0;
	int gr = 0;
	int gd = 0;
	int bd = 0;
	int poor = 0;
	int miss = 0;
	int pr = 0;
	int totalJudgements = 1;
	int comboBreak = 0;
	int badPoor = 0;
	int maxCombo = 0;
	double score = 0.0;
	double exscore = 0.0;
	double maxPoints = 0.0;
	QVariant early;
	QVariant late;
	int totalEarly = 0;
	int totalLate = 0;

	bool operator==(const Lr2SelectScoreStatsData& other) const;
};

class Lr2SelectScoreStats : public QObject {
	Q_OBJECT
	Q_PROPERTY(int pg READ pg NOTIFY changed)
	Q_PROPERTY(int gr READ gr NOTIFY changed)
	Q_PROPERTY(int gd READ gd NOTIFY changed)
	Q_PROPERTY(int bd READ bd NOTIFY changed)
	Q_PROPERTY(int poor READ poor NOTIFY changed)
	Q_PROPERTY(int miss READ miss NOTIFY changed)
	Q_PROPERTY(int pr READ pr NOTIFY changed)
	Q_PROPERTY(int totalJudgements READ totalJudgements NOTIFY changed)
	Q_PROPERTY(int comboBreak READ comboBreak NOTIFY changed)
	Q_PROPERTY(int badPoor READ badPoor NOTIFY changed)
	Q_PROPERTY(int maxCombo READ maxCombo NOTIFY changed)
	Q_PROPERTY(double score READ score NOTIFY changed)
	Q_PROPERTY(double exscore READ exscore NOTIFY changed)
	Q_PROPERTY(double maxPoints READ maxPoints NOTIFY changed)
	Q_PROPERTY(QVariant early READ early NOTIFY changed)
	Q_PROPERTY(QVariant late READ late NOTIFY changed)
	Q_PROPERTY(int totalEarly READ totalEarly NOTIFY changed)
	Q_PROPERTY(int totalLate READ totalLate NOTIFY changed)

public:
	explicit Lr2SelectScoreStats(QObject* parent = nullptr);

	bool hasValues() const;
	int pg() const;
	int gr() const;
	int gd() const;
	int bd() const;
	int poor() const;
	int miss() const;
	int pr() const;
	int totalJudgements() const;
	int comboBreak() const;
	int badPoor() const;
	int maxCombo() const;
	double score() const;
	double exscore() const;
	double maxPoints() const;
	QVariant early() const;
	QVariant late() const;
	int totalEarly() const;
	int totalLate() const;

	const Lr2SelectScoreStatsData& values() const;
	bool setValues(const Lr2SelectScoreStatsData& value);
	void clearValues();

signals:
	void changed();

private:
	Lr2SelectScoreStatsData m_values;
};

Q_DECLARE_METATYPE(Lr2SelectScoreStats*)

struct Lr2SelectScoreSummaryData {
	QVariant bestScore;
	Lr2SelectScoreStatsData bestStats;
	Lr2SelectScoreCountsData counts;
	QString clearType = QStringLiteral("NOPLAY");
	int lamp = 0;
	int rank = 0;
	double scoreRate = 0.0;
	QVariantList optionIds;
};

struct Lr2SelectScoreSummaryCacheKey {
	QString identifier;
	int scoreGeneration = 0;

	bool operator==(const Lr2SelectScoreSummaryCacheKey& other) const {
		return identifier == other.identifier
			&& scoreGeneration == other.scoreGeneration;
	}
};

inline size_t qHash(const Lr2SelectScoreSummaryCacheKey& key, size_t seed = 0) {
	seed = qHash(key.identifier, seed);
	return qHash(key.scoreGeneration, seed);
}

class Lr2SelectScoreSummary : public QObject {
	Q_OBJECT
	Q_PROPERTY(QVariant bestScore READ bestScore NOTIFY changed)
	Q_PROPERTY(QObject* bestStats READ bestStatsObject NOTIFY changed)
	Q_PROPERTY(QObject* scoreCounts READ scoreCountsObject NOTIFY changed)
	Q_PROPERTY(QString clearType READ clearType NOTIFY changed)
	Q_PROPERTY(int lamp READ lamp NOTIFY changed)
	Q_PROPERTY(int rank READ rank NOTIFY changed)
	Q_PROPERTY(double scoreRate READ scoreRate NOTIFY changed)
	Q_PROPERTY(QVariant optionIds READ optionIds NOTIFY changed)

public:
	explicit Lr2SelectScoreSummary(QObject* parent = nullptr);

	QVariant bestScore() const;
	QObject* bestStatsObject() const;
	Lr2SelectScoreStats* bestStatsStats() const;
	QObject* scoreCountsObject() const;
	Lr2SelectScoreCounts* scoreCounts() const;
	QString clearType() const;
	int lamp() const;
	int rank() const;
	double scoreRate() const;
	QVariant optionIds() const;

	bool setValues(const Lr2SelectScoreSummaryData& values);
	void clearValues();

signals:
	void changed();

private:
	QVariant m_bestScore;
	Lr2SelectScoreStats m_bestStats;
	Lr2SelectScoreCounts m_scoreCounts;
	QString m_clearType = QStringLiteral("NOPLAY");
	int m_lamp = 0;
	int m_rank = 0;
	double m_scoreRate = 0.0;
	QVariantList m_optionIds;
};

Q_DECLARE_METATYPE(Lr2SelectScoreSummary*)

class Lr2SelectDifficultyModel : public QAbstractListModel {
	Q_OBJECT
	QML_ELEMENT

public:
	enum Role {
		DifficultyRole = Qt::UserRole + 1,
		CountRole,
		PlayLevelRole,
		LampRole,
	};
	Q_ENUM(Role)

	explicit Lr2SelectDifficultyModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

	Q_INVOKABLE int countForDifficulty(int difficulty) const;
	Q_INVOKABLE int playLevelForDifficulty(int difficulty) const;
	Q_INVOKABLE int lampForDifficulty(int difficulty) const;
	Q_INVOKABLE QVariantList optionIdsForKeymode(int keymode, bool includeLamps) const;

	bool setRows(const QVariantList& counts,
				 const QVariantList& levels,
				 const QVariantList& lamps,
				 int selectedDifficulty = 0,
				 int selectedLamp = 0);
	bool setRowsFromJsValues(const QJSValue& counts,
							 const QJSValue& levels,
							 const QJSValue& lamps,
							 int selectedDifficulty = 0,
							 int selectedLamp = 0);

private:
	struct Row {
		int difficulty = 0;
		int count = 0;
		int playLevel = 0;
		int lamp = 0;

		bool operator==(const Row& other) const;
	};

	bool setRows(QList<Row> nextRows);
	const Row* rowForDifficulty(int difficulty) const;
	static QList<Row> rowsFromValues(const QVariantList& counts,
									 const QVariantList& levels,
									 const QVariantList& lamps,
									 int selectedDifficulty,
									 int selectedLamp);
	static QList<Row> rowsFromJsValues(const QJSValue& counts,
									   const QJSValue& levels,
									   const QJSValue& lamps,
									   int selectedDifficulty,
									   int selectedLamp);
	QList<Row> m_rows;
};

class Lr2SelectDetailState : public QObject {
	Q_OBJECT
	QML_ELEMENT
	Q_PROPERTY(int scoreGeneration READ scoreGeneration WRITE setScoreGeneration NOTIFY scoreGenerationChanged)
	Q_PROPERTY(int listGeneration READ listGeneration WRITE setListGeneration NOTIFY listGenerationChanged)
	Q_PROPERTY(QVariant item READ item WRITE setItem NOTIFY itemChanged)
	Q_PROPERTY(QVariant chartData READ chartData WRITE setChartData NOTIFY chartDataChanged)
	Q_PROPERTY(QObject* summary READ summaryObject CONSTANT)
	Q_PROPERTY(QObject* bestStats READ bestStatsObject NOTIFY bestStatsChanged)
	Q_PROPERTY(QObject* scoreCounts READ scoreCountsObject CONSTANT)
	Q_PROPERTY(QVariant scoreOptionIds READ scoreOptionIds WRITE setScoreOptionIds NOTIFY scoreOptionIdsChanged)
	Q_PROPERTY(Lr2SelectDifficultyModel* difficultyModel READ difficultyModel CONSTANT)

public:
	explicit Lr2SelectDetailState(QObject* parent = nullptr);

	int scoreGeneration() const;
	void setScoreGeneration(int value);

	int listGeneration() const;
	void setListGeneration(int value);

	QVariant item() const;
	void setItem(const QVariant& value);

	QVariant chartData() const;
	void setChartData(const QVariant& value);

	QObject* summaryObject() const;
	Lr2SelectScoreSummary* summary() const;

	QObject* bestStatsObject() const;

	QObject* scoreCountsObject() const;
	Lr2SelectScoreCounts* scoreCounts() const;

	QVariant scoreOptionIds() const;
	void setScoreOptionIds(const QVariant& value);

	Lr2SelectDifficultyModel* difficultyModel();

	Q_INVOKABLE bool selectedIdentityMatches(const QString& itemKey,
											 const QString& targetItemKey,
											 bool rankingMode,
											 int scoreGeneration,
											 int listGeneration,
											 bool useBeatorajaSemantics,
											 bool buildScoreOptionIds,
											 bool difficultyStateUsed,
											 bool difficultyLampStateUsed) const;
	Q_INVOKABLE bool refreshSelectedFromQmlIdentityForIdentifier(const QString& itemKey,
																 const QString& targetItemKey,
																 bool rankingMode,
																 int scoreGeneration,
																 int listGeneration,
																 QJSValue item,
																 QJSValue chartData,
																 QJSValue scoreList,
																 const QString& scoreIdentifier,
																 int selectedDifficulty,
																 QJSValue difficultyCounts,
																 QJSValue difficultyLevels,
																 QJSValue difficultyLamps,
																 bool useBeatorajaSemantics,
																 bool buildScoreOptionIds,
																 bool difficultyStateUsed,
																 bool difficultyLampStateUsed);
	Q_INVOKABLE QObject* cachedScoreSummaryForIdentifier(const QString& identifier,
														 int scoreGeneration,
														 QJSValue scoreList,
														 bool useBeatorajaSemantics,
														 bool buildScoreOptionIds);
	Q_INVOKABLE bool hasCachedScoreSummaryForIdentifier(const QString& identifier,
														int scoreGeneration,
														bool useBeatorajaSemantics,
														bool buildScoreOptionIds);
	Q_INVOKABLE void clearScoreSummaryCache();
	Q_INVOKABLE void clear();

signals:
	void scoreGenerationChanged();
	void listGenerationChanged();
	void itemChanged();
	void chartDataChanged();
	void summaryChanged();
	void bestStatsChanged();
	void scoreCountsChanged();
	void scoreOptionIdsChanged();

private:
	bool selectedRefreshMatches(const QString& itemKey,
								const QString& targetItemKey,
								bool rankingMode,
								int scoreGeneration,
								int listGeneration,
								bool useBeatorajaSemantics,
								bool buildScoreOptionIds,
								bool difficultyStateUsed,
								bool difficultyLampStateUsed) const;
	void rememberSelectedIdentity(const QString& itemKey,
								  const QString& targetItemKey,
								  bool rankingMode,
								  bool difficultyStateUsed,
								  bool difficultyLampStateUsed);
	void clearSelectedIdentity();
	bool applyRefreshData(int scoreGeneration,
						  int listGeneration,
						  const QVariant& item,
						  const QVariant& chartData,
						  bool useBeatorajaSemantics,
						  bool buildScoreOptionIds,
						  const Lr2SelectScoreSummaryData& scoreSummary);
	const Lr2SelectScoreSummaryData& cachedScoreSummaryData(const Lr2SelectScoreSummaryCacheKey& cacheKey,
															const QJSValue& scoreList,
															bool useBeatorajaSemantics,
															bool buildScoreOptionIds);
	void ensureScoreSummaryCacheSemantics(bool useBeatorajaSemantics,
										  bool buildScoreOptionIds);

	int m_scoreGeneration = -1;
	int m_listGeneration = -1;
	bool m_useBeatorajaSemantics = false;
	bool m_buildScoreOptionIds = true;
	bool m_selectedIdentityValid = false;
	QString m_selectedItemKey;
	QString m_selectedTargetItemKey;
	bool m_selectedRankingMode = false;
	bool m_selectedDifficultyStateUsed = true;
	bool m_selectedDifficultyLampStateUsed = true;
	QVariant m_item;
	QVariant m_chartData;
	Lr2SelectScoreSummary m_summary;
	QVariant m_scoreOptionIds;
	Lr2SelectDifficultyModel m_difficultyModel;
	struct CachedScoreSummary {
		Lr2SelectScoreSummaryData data;
		Lr2SelectScoreSummary* object = nullptr;
	};
	QHash<Lr2SelectScoreSummaryCacheKey, CachedScoreSummary> m_scoreSummaryCache;
	bool m_scoreSummaryCacheSemanticsInitialized = false;
	bool m_scoreSummaryCacheUseBeatorajaSemantics = false;
	bool m_scoreSummaryCacheBuildScoreOptionIds = true;
};
