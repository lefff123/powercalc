#pragma once

#include "api.h"
#include "numberformat.h"

#include <cmath>
#include <QAbstractTableModel>
#include <QMap>
#include <QColor>
#include <QMetaType>

class NodeTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum Column {
		ColType = 0,
		ColEnabled,
		ColId,
		ColName,
		ColP,
		ColQ,
		ColVset,
		ColVmag,
		ColDelta,
		ColQmin,
		ColQmax,
		ColCount
	};

	QMap<NodeId, double> m_calcQ;  // рассчитанный Q (вар) после расчёта
	
	explicit NodeTableModel(PowerSystem &system, QObject *parent = nullptr);

	// QAbstractTableModel contract
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QVariant rawData(const Node &n, int col) const;
	bool qOutOfLimits(const Node &n) const;
	void setCalcQ(const QMap<NodeId, double> &calcQ);

	// API
	void addNode(const Node &node);
	void setNames(const QMap<NodeId, QString> &names) { m_names = names; refresh(); }
	void removeNode(int row);
	void refresh();  // Перечитать из PowerSystem
	bool hasNode(NodeId id) const {
		for (const Node &n : m_system.getNodes())
			if (n.id() == id) return true;
		return false;
	}
	QString nodeName(NodeId id) const {
		return m_names.value(id, QString::number(id));
	}
	QMap<NodeId, QString> names() const { return m_names; }

private:
	PowerSystem &m_system;
	QMap<NodeId, QString> m_names;

	NodeId nextFreeId() const;
};