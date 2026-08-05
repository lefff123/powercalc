#include "mainwindow.h"
#include "powersystem.h"
#include "nodetablemodel.h"
#include "linetablemodel.h"
#include "tabledelegate.h"
#include "csvparser.h"
#include "solver.h"
#include "csvwriter.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QTemporaryDir>
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include <quazip/quazipnewinfo.h>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>

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
	connect(m_fileMenu->addAction("Новый проект"), &QAction::triggered, this, &MainWindow::onNewProject);
	QAction *openProj = m_fileMenu->addAction("Открыть проект...");
	connect(openProj, &QAction::triggered, this, &MainWindow::onOpenProject);
	QAction *saveProj = m_fileMenu->addAction("Сохранить проект");
	connect(saveProj, &QAction::triggered, this, &MainWindow::onSaveProject);
	m_fileMenu->addSeparator();

	QAction *importAction = m_fileMenu->addAction("Импорт из RastrWin...");
	connect(importAction, &QAction::triggered, this, &MainWindow::onImportRastrWin);

	QAction *exportAction = m_fileMenu->addAction("Экспорт в RastrWin...");
	connect(exportAction, &QAction::triggered, this, &MainWindow::onExportRastrWin);
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
	QAction *copyAction = m_tablesEditMenu->addAction("Копировать");
	copyAction->setShortcut(QKeySequence::Copy);
	connect(copyAction, &QAction::triggered, this, &MainWindow::onCopySelection);
	QAction *pasteAction = m_tablesEditMenu->addAction("Вставить");
	pasteAction->setShortcut(QKeySequence::Paste);
	connect(pasteAction, &QAction::triggered, this, &MainWindow::onPasteSelection);
	m_tablesEditMenu->addAction("Добавить ветвь");
	m_tablesEditMenu->addAction("Удалить ветвь");

	m_calcMenu = menuBar()->addMenu("Расчёт");
	QAction *calcAction = m_calcMenu->addAction("Рассчитать");
	connect(calcAction, &QAction::triggered, this, &MainWindow::onCalculate);
	QAction *clearAction = m_calcMenu->addAction("Очистить результаты");
	connect(clearAction, &QAction::triggered, this, &MainWindow::onClearResults);

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

	applyParsedSystem(parser);
}

void MainWindow::applyParsedSystem(CsvParser &parser)
{
	const QStringList &nodeNames = parser.nodeNames();
	QMap<NodeId, QString> nodeNameMap;
	for (int i = 0; i < nodeNames.size() && i < m_system.nodesCount(); ++i)
		nodeNameMap[m_system.getNodes()[i].id()] = nodeNames[i];
	m_nodeModel->setNames(nodeNameMap);

	const QStringList &lineNames = parser.lineNames();
	QMap<LineId, QString> lineNameMap;
	for (int i = 0; i < lineNames.size() && i < m_system.linesCount(); ++i)
		lineNameMap[m_system.getLines()[i].id()] = lineNames[i];
	m_lineModel->setNames(lineNameMap);

	m_nodeModel->setCalcQ({});
	m_lineModel->clearFlows();
	statusBar()->showMessage(
		QString("Узлов: %1, линий: %2").arg(m_system.nodesCount()).arg(m_system.linesCount()));
}

void MainWindow::onCalculate()
{
	try {
		m_system.validate();

		Solver solver(m_system);
		const Result res = solver.solve();

		m_nodeModel->refresh();

		if (res.converged) {
			// Потоки по линиям
			std::vector<LineFlows> flows = m_system.calculateLineFlows();
			QVector<LineFlows> qflows;
			qflows.reserve(static_cast<int>(flows.size()));
			for (auto &fl : flows) {
				qflows.append(fl);
			}
			m_lineModel->setFlows(qflows);

			// Рассчитанный Q для PV-узлов (для проверки лимитов)
			auto Y_bus = m_system.buildYBus();
			const auto &nodes = m_system.getNodes();
			QMap<NodeId, double> calcQ;
			for (size_t i = 0; i < nodes.size(); ++i) {
				if (!nodes[i].isEnabled()) continue;
				if (nodes[i].type() != NodeType::PV) continue;
				
				double q_pu = 0.0;
				for (size_t j = 0; j < nodes.size(); ++j) {
					if (!nodes[j].isEnabled()) continue;
					const double v_i = nodes[i].V_mag() / m_system.V_base(nodes[i].id());
					const double v_j = nodes[j].V_mag() / m_system.V_base(nodes[j].id());
					const double braces = Y_bus(i, j).real() * std::sin(nodes[i].delta() - nodes[j].delta())
										- Y_bus(i, j).imag() * std::cos(nodes[i].delta() - nodes[j].delta());
					q_pu += v_i * v_j * braces;
				}
				calcQ[nodes[i].id()] = q_pu * m_system.S_base();
			}
			m_nodeModel->setCalcQ(calcQ);

			statusBar()->showMessage(
				QString("Расчёт сошёлся за %1 итераций, невязка: %2")
					.arg(res.iterations)
					.arg(res.max_mismatch, 0, 'e', 2));
		} else {
			QMessageBox::warning(this, "Расчёт",
				QString("Не сошёлся за %1 итераций. Макс. невязка: %2")
					.arg(res.iterations)
					.arg(res.max_mismatch, 0, 'e', 2));
			statusBar()->showMessage("Расчёт не сошёлся");
		}
	} catch (const std::exception &e) {
		QMessageBox::warning(this, "Ошибка расчёта", e.what());
		statusBar()->showMessage("Ошибка расчёта");
	}
}

void MainWindow::onClearResults()
{
	m_lineModel->clearFlows();
	m_nodeModel->setCalcQ({});
	statusBar()->showMessage("Результаты очищены");
}

void MainWindow::onExportRastrWin()
{
	const QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку экспорта");
	if (dir.isEmpty()) return;

	if (!RastrWriter::write(m_system, m_nodeModel->names(), m_lineModel->names(),
							dir + "/nodes.csv", dir + "/branches.csv")) {
		QMessageBox::warning(this, "Экспорт", "Не удалось записать файлы");
		return;
	}
	statusBar()->showMessage("Экспортировано в " + dir);
}

void MainWindow::onNewProject()
{
	m_system.clear();
	m_nodeModel->setNames({});
	m_lineModel->setNames({});
	m_nodeModel->setCalcQ({});
	m_lineModel->clearFlows();
	statusBar()->showMessage("Новый проект");
}

void MainWindow::onSaveProject()
{
	const QString path = QFileDialog::getSaveFileName(this, "Сохранить проект", "project.zip", "Проекты (*.zip)");
	if (path.isEmpty()) return;

	QTemporaryDir tmp;
	const QString np = tmp.path() + "/nodes.csv";
	const QString bp = tmp.path() + "/branches.csv";
	if (!RastrWriter::write(m_system, m_nodeModel->names(), m_lineModel->names(), np, bp)) {
		QMessageBox::warning(this, "Проект", "Не удалось подготовить данные");
		return;
	}

	QuaZip zip(path);
	if (!zip.open(QuaZip::mdCreate)) {
		QMessageBox::warning(this, "Проект", "Не удалось создать архив");
		return;
	}
	for (const QString &file : {np, bp}) {
		QuaZipFile out(&zip);
		if (!out.open(QIODevice::WriteOnly, QuaZipNewInfo(QFileInfo(file).fileName(), file))) {
			zip.close();
			QMessageBox::warning(this, "Проект", "Ошибка записи в архив");
			return;
		}
		QFile in(file);
		in.open(QIODevice::ReadOnly);
		out.write(in.readAll());
		out.close();
	}
	zip.close();
	statusBar()->showMessage("Проект сохранён: " + path);
}

void MainWindow::onOpenProject()
{
	const QString path = QFileDialog::getOpenFileName(this, "Открыть проект", "", "Проекты (*.zip)");
	if (path.isEmpty()) return;

	QuaZip zip(path);
	if (!zip.open(QuaZip::mdUnzip)) {
		QMessageBox::warning(this, "Проект", "Не удалось открыть архив");
		return;
	}

	QTemporaryDir tmp;
	QString np, bp;
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
		const QString name = zip.getCurrentFileName();
		QuaZipFile in(&zip);
		if (!in.open(QIODevice::ReadOnly)) continue;
		const QByteArray data = in.readAll();
		in.close();

		QString target;
		if (name.endsWith("nodes.csv")) target = np = tmp.path() + "/nodes.csv";
		else if (name.endsWith("branches.csv")) target = bp = tmp.path() + "/branches.csv";
		else continue;

		QFile out(target);
		out.open(QIODevice::WriteOnly);
		out.write(data);
	}
	zip.close();

	if (np.isEmpty() || bp.isEmpty()) {
		QMessageBox::warning(this, "Проект", "В архиве нет nodes.csv / branches.csv");
		return;
	}

	m_system.clear();
	CsvParser parser;
	if (!parser.parseFiles(np, bp, m_system)) {
		QMessageBox::warning(this, "Проект", "Ошибка чтения данных проекта");
		return;
	}
	applyParsedSystem(parser);
}

void MainWindow::onCopySelection()
{
	QTableView *table = nullptr;
	table = currentTable();
	
	if (!table) return;
	
	const QItemSelectionModel *sel = table->selectionModel();
	if (!sel->hasSelection()) return;
	
	const QModelIndexList indexes = sel->selectedIndexes();
	if (indexes.isEmpty()) return;
	
	// Находим границы выделения
	int minRow = indexes.first().row(), maxRow = minRow;
	int minCol = indexes.first().column(), maxCol = minCol;
	for (const QModelIndex &idx : indexes) {
		if (idx.row() < minRow) minRow = idx.row();
		if (idx.row() > maxRow) maxRow = idx.row();
		if (idx.column() < minCol) minCol = idx.column();
		if (idx.column() > maxCol) maxCol = idx.column();
	}
	
	// Собираем TSV
	QStringList rows;
	for (int r = minRow; r <= maxRow; ++r) {
		QStringList cols;
		for (int c = minCol; c <= maxCol; ++c) {
			const QModelIndex idx = table->model()->index(r, c);
			QVariant data = table->model()->data(idx, Qt::DisplayRole);
			cols << data.toString();
		}
		rows << cols.join('\t');
	}
	
	QApplication::clipboard()->setText(rows.join('\n'));
	statusBar()->showMessage("Скопировано строк: " + QString::number(maxRow - minRow + 1));
}

QTableView *MainWindow::currentTable() const
{
	if (m_tableTabs->currentIndex() == 0) return m_nodeTable;
	if (m_tableTabs->currentIndex() == 1) return m_lineTable;
	return nullptr;
}

void MainWindow::onPasteSelection()
{
	QTableView *table = currentTable();
	if (!table) return;
	QItemSelectionModel *sel = table->selectionModel();
	if (!sel->hasSelection()) return;

	const QString text = QApplication::clipboard()->text();
	if (text.isEmpty()) return;

	// Парсим TSV из буфера
	const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
	QVector<QStringList> grid;
	for (QString line : lines) {
		if (line.endsWith('\r')) line.chop(1);  // Excel на Windows даёт CRLF
		grid.append(line.split('\t'));
	}
	if (grid.isEmpty()) return;

	// Границы выделения
	const QModelIndexList indexes = sel->selectedIndexes();
	int minRow = indexes.first().row(), maxRow = minRow;
	int minCol = indexes.first().column(), maxCol = minCol;
	for (const QModelIndex &idx : indexes) {
		minRow = qMin(minRow, idx.row());
		maxRow = qMax(maxRow, idx.row());
		minCol = qMin(minCol, idx.column());
		maxCol = qMax(maxCol, idx.column());
	}

	QAbstractItemModel *model = table->model();
	const int height = maxRow - minRow + 1;
	const int width = maxCol - minCol + 1;

	int pasted = 0;
	for (int r = 0; r < grid.size() && r < height; ++r) {
		const QStringList &cols = grid[r];
		for (int c = 0; c < cols.size() && c < width; ++c) {
			const QModelIndex idx = model->index(minRow + r, minCol + c);
			if (model->setData(idx, cols[c], Qt::EditRole))
				++pasted;
		}
	}
	statusBar()->showMessage(QString("Вставлено ячеек: %1").arg(pasted));
}