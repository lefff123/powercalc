#pragma once

#include <QAbstractTableModel>
#include <QMap>
#include "powersystem.h"

class NodeTableModel;

class LineTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum Column {
		ColType = 0,
		ColEnabled,
		ColId,
		ColName,
		ColFrom,
		ColTo,
		ColR,
		ColX,
		ColG,
		ColB,
		ColKt,
		ColCount
	};

	explicit LineTableModel(PowerSystem &system, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	void addLine(const Line &line);
	void setNames(const QMap<NodeId, QString> &names) { m_names = names; refresh(); }
	void setNodeModel(NodeTableModel *model) { m_nodeModel = model; }
	void refresh();

private:
	PowerSystem &m_system;
	QMap<LineId, QString> m_names;
	NodeTableModel *m_nodeModel = nullptr;

	LineId nextFreeId() const;
};