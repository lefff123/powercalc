#include "linetablemodel.h"
#include "nodetablemodel.h"

LineTableModel::LineTableModel(PowerSystem &system, QObject *parent)
	: QAbstractTableModel(parent), m_system(system) {}

int LineTableModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) return 0;
	return m_system.linesCount() + 1;
}

int LineTableModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid()) return 0;
	return ColCount;
}

QVariant LineTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() > m_system.linesCount())
		return {};

	// пустая строка добавления
	if (index.row() == m_system.linesCount()) {
		if (role == Qt::CheckStateRole && index.column() == ColEnabled)
			return static_cast<int>(Qt::Checked);
		if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.column() == ColType)
			return QStringLiteral("ЛЭП");
		return {};
	}

	const Line &l = m_system.getLines()[index.row()];
	const int col = index.column();

	if (role == Qt::CheckStateRole && col == ColEnabled)
		return l.isEnabled() ? Qt::Checked : Qt::Unchecked;

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (col) {
		case ColId: return static_cast<qlonglong>(l.id());
		case ColName: return m_names.value(l.id(), QString("Линия %1").arg(l.id()));
		case ColFrom: {
		if (role == Qt::EditRole)
			return static_cast<qlonglong>(l.from());  // ID для редактирования
		// DisplayRole — имя
		if (m_nodeModel && m_nodeModel->hasNode(l.from()))
			return m_nodeModel->nodeName(l.from());
		return static_cast<qlonglong>(l.from());
		}
		case ColTo: {
			if (role == Qt::EditRole)
				return static_cast<qlonglong>(l.to());
			if (m_nodeModel && m_nodeModel->hasNode(l.to()))
				return m_nodeModel->nodeName(l.to());
			return static_cast<qlonglong>(l.to());
		}
		case ColType: return l.istransformer() ? "Трансформатор" : "ЛЭП";
		case ColR: return l.R();
		case ColX: return l.X();
		case ColG: return l.Y().real() * 1e6;
		case ColB: return -l.Y().imag() * 1e6;
		case ColKt: return l.istransformer() ? 1.0 / l.k_t().real() : QVariant();
		case ColPfrom: {
			if (!m_flows.contains(l.id())) return QVariant();
			return m_flows[l.id()].S_from.real() / 1e6;
		}
		case ColQfrom: {
			if (!m_flows.contains(l.id())) return QVariant();
			return m_flows[l.id()].S_from.imag() / 1e6;
		}
		case ColPto: {
			if (!m_flows.contains(l.id())) return QVariant();
			return m_flows[l.id()].S_to.real() / 1e6;
		}
		case ColQto: {
			if (!m_flows.contains(l.id())) return QVariant();
			return m_flows[l.id()].S_to.imag() / 1e6;
		}
		case ColPloss: {
			if (!m_flows.contains(l.id())) return QVariant();
			return m_flows[l.id()].S_loss.real() / 1e6;
		}
		case ColQloss: {
			if (!m_flows.contains(l.id())) return QVariant();
			return m_flows[l.id()].S_loss.imag() / 1e6;
		}
		}
	}
	return {};
}

bool LineTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() > m_system.linesCount())
		return false;

	// коммит в пустую строку — создаём линию между первыми двумя узлами
	if (index.row() == m_system.linesCount()) {
		if (m_system.nodesCount() < 2)
			return false;
		const NodeId from = m_system.getNodes()[0].id();
		const NodeId to = m_system.getNodes()[1].id();
		const int last = m_system.linesCount();
		beginInsertRows({}, last + 1, last + 1);
		m_system.addLine(Line(nextFreeId(), from, to, 0.0, 0.0));
		endInsertRows();
	}

	Line &l = m_system.getLine(m_system.getLines()[index.row()].id());
	const int col = index.column();

	if (role == Qt::CheckStateRole && col == ColEnabled) {
		if (value.toInt() == Qt::Checked)
			m_system.connectLine(l.id());
		else
			m_system.disconnectLine(l.id());
		emit dataChanged(index, index, {role});
		return true;
	}

	if (role != Qt::EditRole)
		return false;

	bool ok = false;
	switch (col) {
	case ColName:
		m_names[l.id()] = value.toString();
		break;
	case ColFrom: {
		// Пытаемся как ID
		NodeId v = value.toULongLong(&ok);
		if (!ok) {
			// Ищем по имени
			v = 0;
			for (const Node &n : m_system.getNodes()) {
				if (m_nodeModel->nodeName(n.id()) == value.toString()) {
					v = n.id();
					ok = true;
					break;
				}
			}
		}
		if (!ok || !m_system.hasNode(v) || v == l.to())
			return false;
		l.setFrom(v);
		break;
	}
	case ColTo: {
		// Пытаемся как ID
		NodeId v = value.toULongLong(&ok);
		if (!ok) {
			// Ищем по имени
			v = 0;
			for (const Node &n : m_system.getNodes()) {
				if (m_nodeModel->nodeName(n.id()) == value.toString()) {
					v = n.id();
					ok = true;
					break;
				}
			}
		}
		if (!ok || !m_system.hasNode(v) || v == l.from())
			return false;
		l.setTo(v);
		break;
	}
	case ColType:
		l.setTransformer(value.toString() == "Трансформатор");
		break;
	case ColR: {
		const double val = value.toDouble(&ok);
		if (!ok || val < 0) return false;
		l.setR(val);
		break;
	}
	case ColX: {
		const double val = value.toDouble(&ok);
		if (!ok || val < 0) return false;
		l.setX(val);
		break;
	}
	case ColG: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		l.setY({val * 1e-6, l.Y().imag()});
		break;
	}
	case ColB: {
		const double val = value.toDouble(&ok);
		if (!ok) return false;
		l.setY({l.Y().real(), -val * 1e-6});
		break;
	}
	case ColKt: {
		const double val = value.toDouble(&ok);
		if (!ok || val <= 0) return false;
		l.setKt({1.0 / val, l.k_t().imag()});
		l.setTransformer(true);
		break;
	}
	default:
		return false;
	}

	emit dataChanged(index, index, {role});
	return true;
}

Qt::ItemFlags LineTableModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags f = QAbstractTableModel::flags(index);
	const int col = index.column();

	if (col == ColId)
		return f;

	if (col == ColEnabled)
		return f | Qt::ItemIsUserCheckable;

	if (col == ColPfrom || col == ColQfrom || col == ColPto || col == ColQto || 
		col == ColPloss || col == ColQloss)
		return f;
	return f | Qt::ItemIsEditable;
}

QVariant LineTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return {};
	if (orientation == Qt::Horizontal) {
		switch (section) {
		case ColId: return "ID";
		case ColName: return "Имя";
		case ColEnabled: return "Вкл";
		case ColFrom: return "Откуда";
		case ColTo: return "Куда";
		case ColType: return "Тип";
		case ColR: return "R (Ом)";
		case ColX: return "X (Ом)";
		case ColG: return "G (мкСм)";
		case ColB: return "B (мкСм)";
		case ColKt: return "Kt";
		case ColPfrom: return "P_from (МВт)";
		case ColQfrom: return "Q_from (Мвар)";
		case ColPto: return "P_to (МВт)";
		case ColQto: return "Q_to (Мвар)";
		case ColPloss: return "ΔP (МВт)";
		case ColQloss: return "ΔQ (Мвар)";
		}
	}
	return {};
}

void LineTableModel::addLine(const Line &line)
{
	m_system.addLine(line);
	refresh();
}

void LineTableModel::refresh()
{
	beginResetModel();
	endResetModel();
}

LineId LineTableModel::nextFreeId() const
{
	LineId maxId = 0;
	for (const Line &l : m_system.getLines())
		maxId = std::max(maxId, l.id());
	return m_system.linesCount() ? maxId + 1 : 1;
}

void LineTableModel::setFlows(const QVector<LineFlows> &flows)
{
	m_flows.clear();
	for (const LineFlows &fl : flows) {
		m_flows.insert(fl.line_id, fl);
	}
	refresh();
}

void LineTableModel::clearFlows()
{
	m_flows.clear();
	refresh();
}

void LineTableModel::removeLine(int row)
{
	if (row < 0 || row >= static_cast<int>(m_system.linesCount())) return;
	const LineId id = m_system.getLines()[row].id();
	m_system.removeLine(id);
	m_names.remove(id);
	refresh();
}