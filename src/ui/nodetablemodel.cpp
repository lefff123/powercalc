#include "nodetablemodel.h"

NodeTableModel::NodeTableModel(PowerSystem &system, QObject *parent) 
	: QAbstractTableModel(parent), m_system(system) {}

int NodeTableModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) return 0;
	return m_system.nodesCount() + 1;
}

QVariant NodeTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() > m_system.nodesCount())
		return {};

	if (index.row() == m_system.nodesCount()) {
		if (role == Qt::CheckStateRole && index.column() == ColEnabled)
			return static_cast<int>(Qt::Checked);
		if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == ColType)
			return QStringLiteral("PQ");
		return {};
	}

	const Node &n = m_system.getNodes()[index.row()];
	const int col = index.column();

	if (role == Qt::CheckStateRole && col == ColEnabled)
		return n.isEnabled() ? Qt::Checked : Qt::Unchecked;

	if (qOutOfLimits(n) && col == ColQ) {
		if (role == Qt::BackgroundRole)
			return QColor(255, 192, 203);
		if (role == Qt::ForegroundRole)
			return QColor(Qt::black);
	}

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		const QVariant raw = rawData(n, col);
		if (role == Qt::DisplayRole && raw.metaType().id() == QMetaType::Double)
			return formatDouble(raw.toDouble());
		return raw;
	}
	return {};
}

QVariant NodeTableModel::rawData(const Node &n, int col) const
{
	switch (col) {
	case ColId: return static_cast<qlonglong>(n.id());
	case ColName: return m_names.value(n.id(), QString("Узел %1").arg(n.id()));
	case ColType:
		switch (n.type()) {
		case NodeType::PQ: return "PQ";
		case NodeType::PV: return "PV";
		case NodeType::SLACK: return "SLACK";
		}
		break;
	case ColP: return n.P_spec() / 1e6;
	case ColQ: return n.Q_spec() / 1e6;
	case ColVset: return n.V_set() / 1e3;
	case ColVmag: {
		const double v = n.V_mag();
		return (v > 0) ? v / 1e3 : QVariant();
	}
	case ColDelta: return n.delta() * 180.0 / M_PI;
	case ColQmin: return n.Q_min() / 1e6;
	case ColQmax: return n.Q_max() / 1e6;
	case ColEnabled: return {};
	}
	return {};
}

bool NodeTableModel::qOutOfLimits(const Node &n) const
{
	const double q = m_calcQ.contains(n.id()) ? m_calcQ[n.id()] : n.Q_spec();
	return q > n.Q_max() || q < n.Q_min();
}

bool NodeTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() > m_system.nodesCount())
   		return false;

	if (index.row() == m_system.nodesCount()) {
		const int last = m_system.nodesCount();
		beginInsertRows({}, last + 1, last + 1);
		m_system.addNode(Node::makePQ(nextFreeId(), 0.0, 0.0, 110e3));
		endInsertRows();
	}
	const auto &nodes = m_system.getNodes();
	Node &n = m_system.getNode(nodes[index.row()].id());
	const int col = index.column();

	// Checkbox
	if (role == Qt::CheckStateRole && col == ColEnabled) {
		if (value.toInt() == Qt::Checked)
			n.connect();
		else
			n.disconnect();
		emit dataChanged(index, index, {role});
		return true;
	}

	if (role != Qt::EditRole)
		return false;

	bool ok = false;
	switch (col) {
	case ColName:
		m_names[n.id()] = value.toString();
		break;
	case ColType: {
		const QString s = value.toString().toUpper();
		if (s == "PQ") n.setType(NodeType::PQ);
		else if (s == "PV") n.setType(NodeType::PV);
		else if (s == "SLACK") n.setType(NodeType::SLACK);
		else return false;
		break;
	}
	case ColP: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		n.setP_spec(val * 1e6);
		break;
	}
	case ColQ: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		n.setQ_spec(val * 1e6);
		break;
	}
	case ColVset: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		n.setV_set(val * 1e3);
		break;
	}
	case ColQmin: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		n.setQ_min(val * 1e6);
		break;
	}
	case ColQmax: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		n.setQ_max(val * 1e6);
		break;
	}
	case ColVmag: {
		const double val = value.toDouble(&ok);
		if (!ok || val <= 0) return false;
		n.setV(val * 1e3);  // кВ → В
		break;
	}
	case ColDelta: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		n.setDelta(val * M_PI / 180.0);  // град → рад
		break;
	}
	default:
		return false;  // V_mag, delta — readonly
	}

	emit dataChanged(index, index, {role});
	return true;
}

void NodeTableModel::addNode(const Node &node)
{
	m_system.addNode(node);
	refresh();
}

void NodeTableModel::removeNode(int row)
{
	if (row < 0 || row >= m_system.nodesCount()) return;
	// PowerSystem не имеет removeNode — пока не реализуем
	// Просто обновим представление
	beginResetModel();
	endResetModel();
}

void NodeTableModel::refresh()
{
	beginResetModel();
	endResetModel();
}

int NodeTableModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid()) return 0;
	return ColCount;
}

Qt::ItemFlags NodeTableModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags f = QAbstractTableModel::flags(index);
	const int col = index.column();

	if (col == ColId)
		return f;  // ID не редактируется

	if (col == ColEnabled)
		return f | Qt::ItemIsUserCheckable;  // checkbox

	return f | Qt::ItemIsEditable;
}

QVariant NodeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return {};
	if (orientation == Qt::Horizontal) {
		switch (section) {
		case ColId: return "ID";
		case ColName: return "Имя";
		case ColType: return "Тип";
		case ColP: return "P (МВт)";
		case ColQ: return "Q (Мвар)";
		case ColVset: return "V_set (кВ)";
		case ColVmag: return "V (кВ)";
		case ColDelta: return "δ (град)";
		case ColQmin: return "Q_min (Мвар)";
		case ColQmax: return "Q_max (Мвар)";
		case ColEnabled: return "Вкл";
		}
	}
	return {};
}

NodeId NodeTableModel::nextFreeId() const
{
	NodeId maxId = 0;
	for (const Node &n : m_system.getNodes())
		maxId = std::max(maxId, n.id());
	return m_system.nodesCount() ? maxId + 1 : 1;
}

void NodeTableModel::setCalcQ(const QMap<NodeId, double> &calcQ)
{
	m_calcQ = calcQ;
	refresh();
}