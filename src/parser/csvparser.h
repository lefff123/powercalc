#pragma once
#include "iparser.h"
#include <QFile>
#include <QTextStream>
#include <memory>

class CsvParser : public IParser {
    Q_OBJECT
public:
    explicit CsvParser(double S_base, QObject* parent = nullptr);
    
    bool parseFiles(const QString& nodes_filepath, const QString& branches_filepath) override;
    PowerSystem& getSystem() override;
    void clear() override;

private:
    std::unique_ptr<PowerSystem> system_;
    double S_base_;
    
    // Парсинг узлов
    bool parseNodes(const QString& filepath);
    // Парсинг ветвей
    bool parseBranches(const QString& filepath);
    
    // Вспомогательные методы
    NodeType determineNodeType(int tip, double pg);
    bool parseDouble(const QString& str, double& value);
    bool parseInt(const QString& str, int& value);
};