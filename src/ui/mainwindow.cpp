#include "mainwindow.h"
#include "powersystem.h"
#include "nodetablemodel.h"
#include "linetablemodel.h"
#include "tabledelegate.h"
#include "csvparser.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(PowerSystem &system, QWidget *parent) : QMainWindow(parent), m_system(system)
{
	resize(1200, 800);
	createTabs();
	createMenusAndToolbars();
	statusBar()->showMessage("Готов");
	statusBar()->setStyleSheet("font-size: 11px;");
	statusBar()->setContentsMargins(6, 0, 6, 0);
	applyTabContext(m_mainTabs->currentIndex());
}

void MainWindow::createTabs()
{
	m_mainTabs = new QTabWidget(this);

	auto *docPage = new QWidget(this);

	m_docToolbar = new QToolBar(docPage);
	m_docToolbar->setMovable(false);
	m_docToolbar->addAction("Отменить");
	m_docToolbar->addAction("Повторить");

	auto *docLayout = new QVBoxLayout(docPage);
	docLayout->setContentsMargins(0, 0, 0, 0);
	docLayout->setSpacing(0);
	docLayout->addWidget(m_docToolbar);
	docLayout->addWidget(new QLabel("Документ будет в Этапе 2.2"), 1);

	m_mainTabs->addTab(docPage, "Документ");

	m_tableTabs = new QTabWidget(this);

	m_nodeModel = new NodeTableModel(m_system, this);
	m_nodeTable = new QTableView(this);
	m_nodeTable->setModel(m_nodeModel);
	m_nodeTable->hideColumn(NodeTableModel::ColId);
	m_nodeTable->setItemDelegate(new TableDelegate(NodeTableModel::ColType, NodeTableModel::ColEnabled, {"PQ", "PV", "SLACK"}, this));
	m_tableTabs->addTab(m_nodeTable, "Узлы");

	m_lineModel = new LineTableModel(m_system, this);
	m_lineModel->setNodeModel(m_nodeModel); 
	m_lineTable = new QTableView(this);
	m_lineTable->setModel(m_lineModel);
	m_lineTable->hideColumn(LineTableModel::ColId);
	m_lineTable->setItemDelegate(new TableDelegate(LineTableModel::ColType, LineTableModel::ColEnabled, {"ЛЭП", "Трансформатор"}, this));
	m_tableTabs->addTab(m_lineTable, "Ветви");
	m_mainTabs->addTab(m_tableTabs, "Таблицы");

	m_mainTabs->addTab(new QLabel("Графика — позже на QML"), "Графика");
	m_mainTabs->setDocumentMode(true);

	setCentralWidget(m_mainTabs);
	connect(m_mainTabs, &QTabWidget::currentChanged,
			this, &MainWindow::applyTabContext);
}

void MainWindow::createMenusAndToolbars()
{
	// Общие
	m_fileMenu = menuBar()->addMenu("Файл");
	m_fileMenu->addAction("Новый проект");
	m_fileMenu->addAction("Открыть проект...");
	m_fileMenu->addAction("Сохранить проект");
	m_fileMenu->addAction("Сохранить проект как...");
	m_fileMenu->addSeparator();

	QAction *importAction = m_fileMenu->addAction("Импорт из RastrWin...");
	connect(importAction, &QAction::triggered, this, &MainWindow::onImportRastrWin);

	m_fileMenu->addAction("Экспорт в RastrWin...");
	m_fileMenu->addSeparator();
	m_fileMenu->addAction("Выход", this, &QWidget::close);

	// Документ
	m_docEditMenu = menuBar()->addMenu("Правка");
	m_docEditMenu->addAction("Отменить");
	m_docEditMenu->addAction("Повторить");

	// Таблицы
	m_tablesEditMenu = menuBar()->addMenu("Правка");
	m_tablesEditMenu->addAction("Добавить узел");
	m_tablesEditMenu->addAction("Удалить узел");
	m_tablesEditMenu->addSeparator();
	m_tablesEditMenu->addAction("Добавить ветвь");
	m_tablesEditMenu->addAction("Удалить ветвь");

	m_calcMenu = menuBar()->addMenu("Расчёт");
	m_calcMenu->addAction("Рассчитать");
	m_calcMenu->addAction("Очистить результаты");

	// Графика
	m_graphViewMenu = menuBar()->addMenu("Вид");
	m_graphViewMenu->addAction("Вписать по размеру");

	m_helpMenu = menuBar()->addMenu("Помощь");
	m_helpMenu->addAction("О программе");
}

void MainWindow::applyTabContext(int index)
{
	const bool isDoc    = (index == 0);
	const bool isTables = (index == 1);
	const bool isGraph  = (index == 2);

	m_docEditMenu->menuAction()->setVisible(isDoc);
	m_tablesEditMenu->menuAction()->setVisible(isTables);
	m_calcMenu->menuAction()->setVisible(isTables);
	m_graphViewMenu->menuAction()->setVisible(isGraph);
}

void MainWindow::onImportRastrWin()
{
	QString nodesFile = QFileDialog::getOpenFileName(this, "Выберите nodes.csv", "", "CSV Files (*.csv)");
	if (nodesFile.isEmpty()) return;
	
	QString branchesFile = QFileDialog::getOpenFileName(this, "Выберите branches.csv", "", "CSV Files (*.csv)");
	if (branchesFile.isEmpty()) return;
	
	m_system.clear();
	
	CsvParser parser;
	if (!parser.parseFiles(nodesFile, branchesFile, m_system)) {
		QMessageBox::warning(this, "Ошибка импорта", "Не удалось прочитать файлы");
		return;
	}
	
	// Заполняем имена узлов
	const QStringList &nodeNames = parser.nodeNames();
	QMap<NodeId, QString> nodeNameMap;
	for (int i = 0; i < nodeNames.size() && i <= m_system.nodesCount(); ++i) {
		nodeNameMap[m_system.getNodes()[i].id()] = nodeNames[i];
	}
	m_nodeModel->setNames(nodeNameMap);
	
	// Заполняем имена линий
	const QStringList &lineNames = parser.lineNames();
	QMap<LineId, QString> lineNameMap;
	for (int i = 0; i < lineNames.size() && i < m_system.linesCount(); ++i) {
		lineNameMap[m_system.getLines()[i].id()] = lineNames[i];
	}
	m_lineModel->setNames(lineNameMap);
	
	statusBar()->showMessage(
		QString("Импортировано: %1 узлов, %2 линий")
			.arg(m_system.nodesCount())
			.arg(m_system.linesCount()));
}