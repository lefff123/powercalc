#include "mainwindow.h"
#include "nodetablemodel.h"
#include "linetablemodel.h"
#include "tabledelegate.h"
#include "api.h"
#include "document_editor.h"
#include "document_view.h"
#include "html_generator.h"
#include "image_scheme_handler.h"
#include "pdf_pagenum.h"
#include "aboutdialog.h"

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
#include <QSplitter>
#include <QPageLayout>
#include <QPageSize>
#include <QDir>
#include <QFileInfo>
#include <QWebEngineProfile>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>


using powercalc::ui::DocumentEditor;

MainWindow::MainWindow(PowerSystem &system, QWidget *parent) : QMainWindow(parent), m_system(system)
{
	resize(1200, 800);
	createTabs();
	createMenusAndToolbars();
	m_imgHandler = new powercalc::ui::ImageSchemeHandler(this);
	QWebEngineProfile::defaultProfile()->installUrlSchemeHandler("pcimg", m_imgHandler);
	m_watcher = new QFileSystemWatcher(this);
	connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& dir) {
		if (!m_watcher->directories().contains(dir)) m_watcher->addPath(dir);
		refreshPreview();
	});
	setupProjectDir();
	statusBar()->showMessage("Готов");
	statusBar()->setStyleSheet("font-size: 11px;");
	statusBar()->setContentsMargins(6, 0, 6, 0);
	applyTabContext(m_mainTabs->currentIndex());
}

void MainWindow::createTabs()
{
	m_mainTabs = new QTabWidget(this);

	m_docEditor = new DocumentEditor(this);
	m_docSplitter = new QSplitter(Qt::Horizontal, this);
	m_docSplitter->addWidget(m_docEditor);
	m_mainTabs->addTab(m_docSplitter, "Документ");

	connect(m_docEditor, &DocumentEditor::diagnosticCountChanged, this, [this](int err, int warn) {
		if (err == 0 && warn == 0) {
			statusBar()->showMessage("Документ OK");
		} else {
			statusBar()->showMessage(QString("Документ: %1 ошибок, %2 предупреждений").arg(err).arg(warn));
		}
	});

	connect(m_docEditor, &DocumentEditor::imageDropRequested, this, [this](const QStringList& paths) {
	for (const QString& src : paths) {
		const QString name = QFileInfo(src).fileName();
		const QString dst = m_projectDir.path() + "/images/" + name;
		QFile::remove(dst);
		if (!QFile::copy(src, dst)) {
			QMessageBox::warning(this, "Картинка", "Не удалось скопировать: " + name);
			continue;
		}
		m_docEditor->insertAtCursor(QString("![%1](%1)").arg(name));
	}
	});

	connect(m_docEditor, &DocumentEditor::imageDataDropped, this, [this](const QByteArray& png) {
		const QString name = "drop_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png";
		QFile f(m_projectDir.path() + "/images/" + name);
		if (!f.open(QIODevice::WriteOnly)) return;
		f.write(png);
		f.close();
		m_docEditor->insertAtCursor(QString("![%1](%1)").arg(name));
	});

	connect(m_docEditor, &DocumentEditor::urlDropRequested, this, [this](const QStringList& urls) {
	for (const QString& us : urls) {
		const QUrl u(us);
		auto* nam = new QNetworkAccessManager(this);
		QNetworkReply* reply = nam->get(QNetworkRequest(u));
		connect(reply, &QNetworkReply::finished, this, [this, reply, nam, u]() {
			reply->deleteLater();
			nam->deleteLater();
			if (reply->error() != QNetworkReply::NoError) {
				QMessageBox::warning(this, "Картинка", "Не удалось скачать: " + u.toString());
				return;
			}
			const QByteArray data = reply->readAll();
			const QString mime = reply->header(QNetworkRequest::ContentTypeHeader).toString();
			QString ext = "png";
			if (mime.contains("jpeg")) ext = "jpg";
			else if (mime.contains("svg")) ext = "svg";
			else if (mime.contains("webp")) ext = "webp";
			const QString name = "web_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + "." + ext;
			QFile f(m_projectDir.path() + "/images/" + name);
			if (!f.open(QIODevice::WriteOnly)) return;
			f.write(data);
			f.close();
			m_docEditor->insertAtCursor(QString("![%1](%1)").arg(name));
		});
	}
	});

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

	m_fileMenu->addAction("Экспорт HTML…", this, &MainWindow::onExportHtml);
	m_fileMenu->addAction("Экспорт PDF…", this, &MainWindow::onExportPdf);

	m_fileMenu->addSeparator();
	m_fileMenu->addAction("Выход", this, &QWidget::close);

	// Документ
	m_docEditMenu = menuBar()->addMenu("Правка");
	QAction *undoAction = m_docEditMenu->addAction("Отменить");
	undoAction->setShortcut(QKeySequence::Undo);
	connect(undoAction, &QAction::triggered, this, [this]() {
		if (m_docEditor && m_mainTabs->currentIndex() == 0) {
			m_docEditor->findChild<QTextEdit*>()->undo();
		}
	});
	QAction *redoAction = m_docEditMenu->addAction("Повторить");
	redoAction->setShortcut(QKeySequence::Redo);
	connect(redoAction, &QAction::triggered, this, [this]() {
		if (m_docEditor && m_mainTabs->currentIndex() == 0) {
			m_docEditor->findChild<QTextEdit*>()->redo();
		}
	});

	m_insertMenu = menuBar()->addMenu("Вставка");
	m_insertMenu->addAction("Картинка…", this, &MainWindow::onInsertImage);

	m_docEditMenu->addSeparator();
	m_previewAct = m_docEditMenu->addAction("Превью");
	m_previewAct->setCheckable(true);
	m_previewAct->setChecked(true);
	ensureDocView();
	m_docView->setVisible(true);
	connect(m_previewAct, &QAction::triggered, this, [this](bool on) {
		if (on) ensureDocView();
		if (m_docView) {
			m_docView->setVisible(on);
			if (on) {
				const int half = m_docSplitter->width() / 2;
				m_docSplitter->setSizes({half, half});
				refreshPreview();
			} else {
				m_docSplitter->setSizes({m_docSplitter->width(), 0});
			}
		}
	});

	// Таблицы
	m_tablesEditMenu = menuBar()->addMenu("Правка");
	QAction *addNode = m_tablesEditMenu->addAction("Добавить узел");
	connect(addNode, &QAction::triggered, this, &MainWindow::onAddNode);
	QAction *delNode = m_tablesEditMenu->addAction("Удалить узел");
	connect(delNode, &QAction::triggered, this, &MainWindow::onDeleteNode);
	m_tablesEditMenu->addSeparator();
	QAction *copyAction = m_tablesEditMenu->addAction("Копировать");
	copyAction->setShortcut(QKeySequence::Copy);
	connect(copyAction, &QAction::triggered, this, &MainWindow::onCopySelection);
	QAction *pasteAction = m_tablesEditMenu->addAction("Вставить");
	pasteAction->setShortcut(QKeySequence::Paste);
	connect(pasteAction, &QAction::triggered, this, &MainWindow::onPasteSelection);
	m_tablesEditMenu->addSeparator();
	QAction *addLine = m_tablesEditMenu->addAction("Добавить ветвь");
	connect(addLine, &QAction::triggered, this, &MainWindow::onAddLine);
	QAction *delLine = m_tablesEditMenu->addAction("Удалить ветвь");
	connect(delLine, &QAction::triggered, this, &MainWindow::onDeleteLine);

	m_calcMenu = menuBar()->addMenu("Расчёт");
	QAction *calcAction = m_calcMenu->addAction("Рассчитать");
	connect(calcAction, &QAction::triggered, this, &MainWindow::onCalculate);
	QAction *clearAction = m_calcMenu->addAction("Очистить результаты");
	connect(clearAction, &QAction::triggered, this, &MainWindow::onClearResults);

	// Графика
	m_graphViewMenu = menuBar()->addMenu("Вид");
	m_graphViewMenu->addAction("Вписать по размеру");

	m_helpMenu = menuBar()->addMenu("Помощь");
	m_helpMenu->addAction("О программе", this, &MainWindow::onAbout);
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
	m_insertMenu->menuAction()->setVisible(isDoc);
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
	setupProjectDir();
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
	const QString dp = tmp.path() + "/document.txt";
	
	if (!RastrWriter::write(m_system, m_nodeModel->names(), m_lineModel->names(), np, bp)) {
		QMessageBox::warning(this, "Проект", "Не удалось подготовить данные");
		return;
	}

	// Сохраняем document.txt с оригинальными line endings
	QString docText = m_docEditor->text();
	docText.replace("\n", m_documentLineEndings);
	QFile docFile(dp);
	docFile.open(QIODevice::WriteOnly);
	docFile.write(docText.toUtf8());
	docFile.close();

	QuaZip zip(path);
	if (!zip.open(QuaZip::mdCreate)) { QMessageBox::warning(this, "Проект", "Не удалось создать архив"); return; }
	auto addFile = [&](const QString& file, const QString& zipName) {
		QuaZipFile out(&zip);
		if (!out.open(QIODevice::WriteOnly, QuaZipNewInfo(zipName, file))) {
			zip.close(); QMessageBox::warning(this, "Проект", "Ошибка записи в архив"); return;
		}
		QFile in(file); in.open(QIODevice::ReadOnly); out.write(in.readAll()); out.close();
	};
	addFile(np, "nodes.csv");
	addFile(bp, "branches.csv");
	addFile(dp, "document.txt");
	for (const QFileInfo& fi : QDir(m_projectDir.path() + "/images").entryInfoList(QDir::Files))
		addFile(fi.absoluteFilePath(), "images/" + fi.fileName());
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
	setupProjectDir();
	QString np, bp, dp;
	for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
		const QString name = zip.getCurrentFileName();
		QuaZipFile in(&zip);
		if (!in.open(QIODevice::ReadOnly)) continue;
		const QByteArray data = in.readAll();
		in.close();

		QString target;
		if (name.endsWith("nodes.csv")) target = np = m_projectDir.path() + "/nodes.csv";
		else if (name.endsWith("branches.csv")) target = bp = m_projectDir.path() + "/branches.csv";
		else if (name.endsWith("document.txt")) target = dp = m_projectDir.path() + "/document.txt";
		else if (name.startsWith("images/")) {
			target = m_projectDir.path() + "/" + name;
			QDir().mkpath(QFileInfo(target).absolutePath());
		}
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

	// Загружаем document.txt
	if (!dp.isEmpty()) {
		QFile docFile(dp);
		docFile.open(QIODevice::ReadOnly);
		QByteArray docData = docFile.readAll();
		docFile.close();

		// Определяем оригинальные line endings
		if (docData.contains("\r\n")) {
			m_documentLineEndings = "\r\n";
		} else {
			m_documentLineEndings = "\n";
		}

		QString docText = QString::fromUtf8(docData);
		m_docEditor->setText(docText);
	} else {
		// document.txt нет — создаём шаблон
		m_documentLineEndings = "\n";
		QString templateText = "---\ntitle: Новый документ\nauthor: \ndate: \n---\n\n# Заголовок\n\n$$\nx = 1\n$$\n";
		m_docEditor->setText(templateText);
	}
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

void MainWindow::onDeleteNode()
{
	const int row = m_nodeTable->selectionModel()->currentIndex().row();
	if (row < 0) return;
	try {
		m_nodeModel->removeNode(row);
		statusBar()->showMessage("Узел удалён");
	} catch (const std::exception &e) {
		QMessageBox::warning(this, "Удаление", e.what());
	}
}

void MainWindow::onDeleteLine()
{
	const int row = m_lineTable->selectionModel()->currentIndex().row();
	if (row < 0) return;
	try {
		m_lineModel->removeLine(row);
		statusBar()->showMessage("Ветвь удалена");
	} catch (const std::exception &e) {
		QMessageBox::warning(this, "Удаление", e.what());
	}
}

void MainWindow::onAddNode()
{
	NodeId maxId = 0;
	for (const Node &n : m_system.getNodes())
		maxId = std::max(maxId, n.id());
	NodeId newId = m_system.nodesCount() ? maxId + 1 : 1;
	m_nodeModel->addNode(Node::makePQ(newId, 0.0, 0.0, 110e3));
	
	// Прокрутить к новой строке
	const int lastRow = m_nodeModel->rowCount() - 2;  // -2 потому что +1 пустая строка
	if (lastRow >= 0) {
		m_nodeTable->selectRow(lastRow);
		m_nodeTable->scrollTo(m_nodeModel->index(lastRow, 0));
	}
	statusBar()->showMessage("Узел добавлен");
}

void MainWindow::onAddLine()
{
	if (m_system.nodesCount() < 2) {
		QMessageBox::information(this, "Добавление", "Создайте минимум 2 узла");
		return;
	}
	LineId maxId = 0;
	for (const Line &l : m_system.getLines())
		maxId = std::max(maxId, l.id());
	LineId newId = m_system.linesCount() ? maxId + 1 : 1;
	
	const NodeId from = m_system.getNodes()[0].id();
	const NodeId to = m_system.getNodes()[1].id();
	m_lineModel->addLine(Line(newId, from, to, 0.0, 0.0));
	
	// Прокрутить к новой строке
	const int lastRow = m_lineModel->rowCount() - 2;
	if (lastRow >= 0) {
		m_lineTable->selectRow(lastRow);
		m_lineTable->scrollTo(m_lineModel->index(lastRow, 0));
	}
	statusBar()->showMessage("Ветвь добавлена");
}

static double toMm(const std::string& s) {
	double v = std::atof(s.c_str());
	return s.size() > 2 && s.compare(s.size() - 2, 2, "cm") == 0 ? v * 10.0 : v;
}

void MainWindow::ensureDocView()
{
	if (m_docView) return;
	m_docView = new powercalc::ui::DocumentView(m_docSplitter);
	m_docSplitter->addWidget(m_docView);
	m_docSplitter->setStretchFactor(0, 1);
	m_docSplitter->setStretchFactor(1, 1);
	// стартовый размер 50/50
	const int half = m_docSplitter->width() / 2;
	m_docSplitter->setSizes({half, half});
	connect(m_docEditor, &DocumentEditor::documentChanged, this, &MainWindow::refreshPreview);
	refreshPreview();
}

void MainWindow::refreshPreview()
{
	if (!m_docView) return;
	powercalc::document::HtmlOptions o;
	o.imageResolver = [this](const std::string& n) -> std::string {
		QFileInfo fi(m_projectDir.path() + "/images/" + QString::fromStdString(n));
		if (!fi.exists()) return "";
		return ("pcimg://" + fi.fileName() + "?v=" + QString::number(fi.lastModified().toMSecsSinceEpoch())).toStdString();
	};
	m_docView->showHtml(QString::fromStdString(
		powercalc::document::generateHtml(m_docEditor->ast(), m_docEditor->evalResult(), o)));
}

void MainWindow::onExportHtml()
{
	const QString path = QFileDialog::getSaveFileName(this, "Экспорт HTML", "report.html", "HTML (*.html)");
	if (path.isEmpty()) return;
	powercalc::document::HtmlOptions o;
	o.assetPrefix = "katex/";
	o.exportMode = true;

	o.imageResolver = [this](const std::string& n) -> std::string {
	const QString name = QString::fromStdString(n);
	QFile f(m_projectDir.path() + "/images/" + name);
	if (!f.open(QIODevice::ReadOnly)) return "";
	QString mime = "image/png";
	if (name.endsWith(".svg", Qt::CaseInsensitive)) mime = "image/svg+xml";
	else if (name.endsWith(".jpg", Qt::CaseInsensitive) || name.endsWith(".jpeg", Qt::CaseInsensitive)) mime = "image/jpeg";
	return ("data:" + mime + ";base64," + QString::fromLatin1(f.readAll().toBase64())).toStdString();
	};

	const std::string html = powercalc::document::generateHtml(m_docEditor->ast(), m_docEditor->evalResult(), o);
	QFile f(path);
	f.open(QIODevice::WriteOnly);
	f.write(QByteArray::fromStdString(html));
	f.close();

	const QString base = QFileInfo(path).absolutePath() + "/katex";
	QDir().mkpath(base + "/fonts");
	QDir().mkpath(base + "/contrib");
	auto copy = [](const QString& q, const QString& d) { QFile::remove(d); QFile::copy(q, d); };
	copy(":/katex/katex.min.css", base + "/katex.min.css");
	copy(":/katex/katex.min.js", base + "/katex.min.js");
	copy(":/katex/contrib/auto-render.min.js", base + "/contrib/auto-render.min.js");
	for (const QString& fn : QDir(":/katex/fonts").entryList({"*.woff2"}, QDir::Files))
		copy(":/katex/fonts/" + fn, base + "/fonts/" + fn);
	statusBar()->showMessage("Экспортировано: " + path);
}

void MainWindow::onExportPdf()
{
	const QString path = QFileDialog::getSaveFileName(this, "Экспорт PDF", "report.pdf", "PDF (*.pdf)");
	if (path.isEmpty()) return;

	ensureDocView();

	const auto& m = m_docEditor->ast().meta;
	QPageSize ps(QPageSize::A4);
	if (m.pageSize == "A3") ps = QPageSize(QPageSize::A3);
	else if (m.pageSize == "A5") ps = QPageSize(QPageSize::A5);
	else if (m.pageSize == "Letter") ps = QPageSize(QPageSize::Letter);
	const QPageLayout layout(ps, QPageLayout::Portrait,
		QMarginsF(toMm(m.marginLeft), toMm(m.marginTop), toMm(m.marginRight), toMm(m.marginBottom)),
		QPageLayout::Millimeter);

	const bool needNumbers = m.showPageNumbers;
	const int start = m.pageStart;
	const bool first = m.numberFirstPage;
	const double bottomMm = toMm(m.marginBottom);
	const QString pageSizeS = QString::fromStdString(m.pageSize);
	const QString tmp = path + ".tmp.pdf";

	connect(m_docView, &QWebEngineView::loadFinished, this, [this, tmp, layout]() {
		m_docView->page()->printToPdf(tmp, layout);
	}, Qt::SingleShotConnection);

	connect(m_docView->page(), &QWebEnginePage::pdfPrintingFinished, this,
		[this, path, tmp, needNumbers, start, first, bottomMm, pageSizeS](const QString&, bool ok) {
			if (!ok) {
				QFile::remove(tmp);
				QMessageBox::warning(this, "PDF", "Не удалось сформировать PDF");
				return;
			}
			if (!needNumbers) {
				QFile::remove(path);
				QFile::rename(tmp, path);
				statusBar()->showMessage("PDF сохранён: " + path);
			} else {
				QString err;
				int pages = 0;
				if (!powercalc::ui::addPageNumbers(tmp, path, start, first, bottomMm, pageSizeS, &err, &pages)) {
					QFile::remove(tmp);
					QMessageBox::warning(this, "PDF", "Ошибка нумерации страниц: " + err);
					return;
				}
				QFile::remove(tmp);
				statusBar()->showMessage(QString("PDF сохранён: %1 (страниц: %2, нумерация с %3)")
					.arg(path).arg(pages).arg(first ? start : start + 1));
			}
		}, Qt::SingleShotConnection);

	refreshPreview();
}
void MainWindow::setupProjectDir()
{
	m_projectDir = QTemporaryDir();
	const QString img = m_projectDir.path() + "/images";
	QDir().mkpath(img);
	m_imgHandler->setImagesDir(img);
	for (const QString& d : m_watcher->directories()) m_watcher->removePath(d);
	m_watcher->addPath(img);
}

void MainWindow::onInsertImage()
{
	const QString src = QFileDialog::getOpenFileName(this, "Выберите картинку", "", "Images (*.png *.jpg *.jpeg *.svg)");
	if (src.isEmpty()) return;
	const QString name = QFileInfo(src).fileName();
	const QString dst = m_projectDir.path() + "/images/" + name;
	QFile::remove(dst);
	if (!QFile::copy(src, dst)) { QMessageBox::warning(this, "Картинка", "Не удалось скопировать файл"); return; }
	m_docEditor->insertAtCursor(QString("![%1](%1)").arg(name));
}

void MainWindow::onAbout() {
	AboutDialog dlg(this);
	dlg.exec();
}