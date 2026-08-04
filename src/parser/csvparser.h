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
	explicit CsvParser(double S_base, QObject* parent = nullptr);
	
	bool parseFiles(const QString& nodes_filepath, const QString& branches_filepath) override;
	PowerSystem& getSystem() override;
	void clear() override;

private:
	std::unique_ptr<PowerSystem> system_;
	std::unordered_map<QString, size_t> headers_;
	QStringList names_nodes_;
	QStringList names_lines_;
	double S_base_;
	
	// Парсинг заголовка
	std::unordered_map<QString, size_t> parseHeaders(const QString &line);
	// Парсинг узлов
	bool parseNodes(const QString& filepath);
	// Парсинг ветвей
	bool parseBranches(const QString& filepath);
	
	// Вспомогательные методы
	bool parseDouble(const QString& str, double& value);
	bool parseInt(const QString& str, int& value);
};