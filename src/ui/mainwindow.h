#pragma once

#include <QMainWindow>
#include <QTableView>

class QTabWidget;
class QMenu;
class QToolBar;
class PowerSystem;
class NodeTableModel;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(PowerSystem &system, QWidget *parent = nullptr);

private:
	void createTabs();
	void createMenusAndToolbars();
	void applyTabContext(int index);

	QTabWidget *m_mainTabs = nullptr;   // Документ | Таблицы | Графика
	QTabWidget *m_tableTabs = nullptr;  // Узлы | Ветви

	QMenu *m_fileMenu = nullptr;
	QMenu *m_helpMenu = nullptr;
	QMenu *m_docEditMenu = nullptr;
	QMenu *m_tablesEditMenu = nullptr;
	QMenu *m_calcMenu = nullptr;
	QMenu *m_graphViewMenu = nullptr;

	QToolBar *m_docToolbar = nullptr;

	QTableView *m_nodeTable = nullptr;
	NodeTableModel *m_nodeModel = nullptr;

	PowerSystem &m_system;
};