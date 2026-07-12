#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "../core/powersystem.h"

class IParser : public QObject
{
    Q_OBJECT
public:
    explicit IParser(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IParser() = default;

    // Парсинг двух файлов (узлы и ветви)
    virtual bool parseFiles(const QString& nodes_filepath, const QString& branches_filepath) = 0;
    
    // Получить собранную систему
    virtual PowerSystem& getSystem() = 0;
    
    // Очистить текущую систему
    virtual void clear() = 0;

signals:
    void started();
    void nodeParsed(size_t id, const QString& name);
    void lineParsed(size_t id, size_t from, size_t to);
    void progressChanged(int percent); // 0..100
    void completed();
    void errorOccurred(const QString& message);
    void warningOccurred(const QString& message);
};