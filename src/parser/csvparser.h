#pragma once
#include "iparser.h"
#include <QFile>
#include <QTextStream>
#include <cstddef>
#include <memory>
#include <qglobal.h>
#include <qobject.h>
#include <unordered_map>


class CsvParser : public IParser {
	Q_OBJECT
public:
	explicit CsvParser(QObject* parent = nullptr);
	
	bool parseFiles(const QString& nodes_filepath, 
                            const QString& branches_filepath,
                            PowerSystem& system) override;
private:
	std::unordered_map<QString, size_t> headers_;
	QStringList names_nodes_;
	QStringList names_lines_;
	
	// Парсинг заголовка
	std::unordered_map<QString, size_t> parseHeaders(const QString &line);
	// Парсинг узлов
	bool parseNodes(const QString& filepath, PowerSystem& system);
	// Парсинг ветвей
	bool parseBranches(const QString& filepath, PowerSystem& system);

};