#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SelectDifficultyModel : public QAbstractListModel {
	Q_OBJECT
	QML_ELEMENT

public:
	enum Role {
		DifficultyRole = Qt::UserRole + 1,
		ChartRole,
		CountRole,
		PlayLevelRole,
		LampRole,
	};
	Q_ENUM(Role)

	explicit Lr2SelectDifficultyModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

	Q_INVOKABLE QVariant chartForDifficulty(int difficulty) const;
	Q_INVOKABLE int countForDifficulty(int difficulty) const;
	Q_INVOKABLE int playLevelForDifficulty(int difficulty) const;
	Q_INVOKABLE int lampForDifficulty(int difficulty) const;

	void setRows(const QVariantList& charts,
				 const QVariantList& counts,
				 const QVariantList& levels,
				 const QVariantList& lamps);

private:
	struct Row {
		int difficulty = 0;
		QVariant chart;
		int count = 0;
		int playLevel = 0;
		int lamp = 0;
	};

	const Row* rowForDifficulty(int difficulty) const;
	QList<Row> m_rows;
};

class Lr2SelectDetailState : public QObject {
	Q_OBJECT
	QML_ELEMENT
	Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)
	Q_PROPERTY(int scoreRevision READ scoreRevision WRITE setScoreRevision NOTIFY scoreRevisionChanged)
	Q_PROPERTY(int listRevision READ listRevision WRITE setListRevision NOTIFY listRevisionChanged)
	Q_PROPERTY(QVariant item READ item WRITE setItem NOTIFY itemChanged)
	Q_PROPERTY(QVariant chartData READ chartData WRITE setChartData NOTIFY chartDataChanged)
	Q_PROPERTY(QVariant chartWrapper READ chartWrapper WRITE setChartWrapper NOTIFY chartWrapperChanged)
	Q_PROPERTY(QVariantList scoreList READ scoreList WRITE setScoreList NOTIFY scoreListChanged)
	Q_PROPERTY(QVariant summary READ summary WRITE setSummary NOTIFY summaryChanged)
	Q_PROPERTY(QVariant bestStats READ bestStats WRITE setBestStats NOTIFY bestStatsChanged)
	Q_PROPERTY(QVariant scoreCounts READ scoreCounts WRITE setScoreCounts NOTIFY scoreCountsChanged)
	Q_PROPERTY(QVariant scoreOptionIds READ scoreOptionIds WRITE setScoreOptionIds NOTIFY scoreOptionIdsChanged)
	Q_PROPERTY(QVariant difficultyState READ difficultyState WRITE setDifficultyState NOTIFY difficultyStateChanged)
	Q_PROPERTY(Lr2SelectDifficultyModel* difficultyModel READ difficultyModel CONSTANT)
	Q_PROPERTY(QVariant asObject READ asObject NOTIFY asObjectChanged)

public:
	explicit Lr2SelectDetailState(QObject* parent = nullptr);

	QString key() const;
	void setKey(const QString& value);

	int scoreRevision() const;
	void setScoreRevision(int value);

	int listRevision() const;
	void setListRevision(int value);

	QVariant item() const;
	void setItem(const QVariant& value);

	QVariant chartData() const;
	void setChartData(const QVariant& value);

	QVariant chartWrapper() const;
	void setChartWrapper(const QVariant& value);

	QVariantList scoreList() const;
	void setScoreList(const QVariantList& value);

	QVariant summary() const;
	void setSummary(const QVariant& value);

	QVariant bestStats() const;
	void setBestStats(const QVariant& value);

	QVariant scoreCounts() const;
	void setScoreCounts(const QVariant& value);

	QVariant scoreOptionIds() const;
	void setScoreOptionIds(const QVariant& value);

	QVariant difficultyState() const;
	void setDifficultyState(const QVariant& value);

	Lr2SelectDifficultyModel* difficultyModel();

	QVariant asObject() const;

	Q_INVOKABLE QVariant snapshot() const;
	Q_INVOKABLE void apply(const QVariantMap& values);
	Q_INVOKABLE bool refresh(const QString& key,
							 int scoreRevision,
							 int listRevision,
							 const QVariant& item,
							 const QVariant& chartData,
							 const QVariantList& scoreList,
							 const QVariantList& difficultyCharts,
							 const QVariantList& difficultyCounts,
							 const QVariantList& difficultyLevels,
							 const QVariantList& difficultyLamps);
	Q_INVOKABLE void clear();

signals:
	void keyChanged();
	void scoreRevisionChanged();
	void listRevisionChanged();
	void itemChanged();
	void chartDataChanged();
	void chartWrapperChanged();
	void scoreListChanged();
	void summaryChanged();
	void bestStatsChanged();
	void scoreCountsChanged();
	void scoreOptionIdsChanged();
	void difficultyStateChanged();
	void asObjectChanged();

private:
	QVariantMap buildChartWrapper(const QVariant& chartData) const;
	void emitObjectChangedIf(bool changed);

	QString m_key;
	int m_scoreRevision = -1;
	int m_listRevision = -1;
	QVariant m_item;
	QVariant m_chartData;
	QVariant m_chartWrapper;
	QVariantList m_scoreList;
	QVariant m_summary;
	QVariant m_bestStats;
	QVariant m_scoreCounts;
	QVariant m_scoreOptionIds;
	QVariant m_difficultyState;
	Lr2SelectDifficultyModel m_difficultyModel;
};
