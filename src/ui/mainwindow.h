#pragma once

#include <QMainWindow>
#include <QTableView>
namespace powercalc::ui {
class DocumentEditor;
class DocumentView;
}

class QTabWidget;
class QMenu;
class QToolBar;
class PowerSystem;
class NodeTableModel;
class LineTableModel;
class QSplitter;
class QAction;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(PowerSystem &system, QWidget *parent = nullptr);

private:
	void createTabs();
	void createMenusAndToolbars();
	void applyTabContext(int index);
	void applyParsedSystem(class CsvParser &parser);

	void ensureDocView();
	void refreshPreview();

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

	QTableView *m_lineTable = nullptr;
	LineTableModel *m_lineModel = nullptr;

	QTableView *currentTable() const;

	PowerSystem &m_system;

	QSplitter *m_docSplitter = nullptr;
	powercalc::ui::DocumentView *m_docView = nullptr;
	QAction *m_previewAct = nullptr;

	powercalc::ui::DocumentEditor* m_docEditor = nullptr;
	QString m_documentLineEndings = "\n"; 

private slots:
	void onImportRastrWin();
	void onExportRastrWin();
	void onCalculate();
	void onClearResults();
	void onNewProject();
	void onSaveProject();
	void onOpenProject();
	void onCopySelection();
	void onPasteSelection();
	void onAddLine();
	void onAddNode();
	void onDeleteLine();
	void onDeleteNode();
	void onExportHtml();
	void onExportPdf();
};