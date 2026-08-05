#pragma once

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QApplication>
#include <QStyle>
#include <QMouseEvent>

class TableDelegate : public QStyledItemDelegate
{
public:
	TableDelegate(int typeCol, int checkCol, QStringList typeItems, QObject *parent = nullptr)
		: QStyledItemDelegate(parent)
		, m_typeCol(typeCol)
		, m_checkCol(checkCol)
		, m_items(std::move(typeItems)) {}

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
	{
		if (index.column() == m_typeCol) {
			auto *cb = new QComboBox(parent);
			cb->addItems(m_items);
			return cb;
		}
		return QStyledItemDelegate::createEditor(parent, option, index);
	}

	void setEditorData(QWidget *editor, const QModelIndex &index) const override
	{
		if (auto *cb = qobject_cast<QComboBox*>(editor)) {
			cb->setCurrentText(index.data().toString());
			return;
		}
		QStyledItemDelegate::setEditorData(editor, index);
	}

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
	{
		if (auto *cb = qobject_cast<QComboBox*>(editor)) {
			model->setData(index, cb->currentText(), Qt::EditRole);
			return;
		}
		QStyledItemDelegate::setModelData(editor, model, index);
	}

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
	{
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);
		opt.displayAlignment = Qt::AlignCenter;

		if (index.column() == m_checkCol && index.data(Qt::CheckStateRole).isValid()) {
			QStyleOptionButton buttonOpt;
			buttonOpt.state = opt.state;
			buttonOpt.state |= (index.data(Qt::CheckStateRole).toInt() == Qt::Checked) ? QStyle::State_On : QStyle::State_Off;
			buttonOpt.rect = checkBoxRect(option);
			QApplication::style()->drawControl(QStyle::CE_CheckBox, &buttonOpt, painter);
			return;
		}

		QStyledItemDelegate::paint(painter, opt, index);
	}

	bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override
	{
		if (index.column() == m_checkCol && event->type() == QEvent::MouseButtonRelease) {
			auto *mouseEvent = static_cast<QMouseEvent*>(event);
			QRect checkBoxRect = this->checkBoxRect(option);
			if (checkBoxRect.contains(mouseEvent->pos())) {
				Qt::CheckState state = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
				state = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
				model->setData(index, state, Qt::CheckStateRole);
				return true;
			}
		}
		return QStyledItemDelegate::editorEvent(event, model, option, index);
	}

private:
	QRect checkBoxRect(const QStyleOptionViewItem &opt) const
	{
		QStyleOptionButton buttonOpt;
		buttonOpt.state = opt.state;
		const QSize cbSize = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &buttonOpt).size();
		return QRect(opt.rect.center().x() - cbSize.width() / 2,
					 opt.rect.center().y() - cbSize.height() / 2,
					 cbSize.width(), cbSize.height());
	}

	int m_typeCol;
	int m_checkCol;
	QStringList m_items;
};