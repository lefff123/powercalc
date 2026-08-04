#pragma once

#include <QAbstractTableModel>
#include <QMap>
#include "powersystem.h"

class NodeTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum Column {
		ColId = 0,
		ColName,
		ColType,
		ColP,
		ColQ,
		ColVset,
		ColVmag,
		ColDelta,
		ColQmin,
		ColQmax,
		ColEnabled,
		ColCount
	};

	explicit NodeTableModel(PowerSystem &system, QObject *parent = nullptr);

	// QAbstractTableModel contract
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	// API
	void addNode(const Node &node);
	void removeNode(int row);
	void refresh();  // Перечитать из PowerSystem

private:
	PowerSystem &m_system;
	QMap<NodeId, QString> m_names;
};