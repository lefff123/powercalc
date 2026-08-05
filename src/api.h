#pragma once

// =====================================================================
// ПУБЛИЧНЫЙ API БЭКЕНДА ДЛЯ UI — КОНТРАКТ
// ---------------------------------------------------------------------
// UI (и в 2.2 — парсер документа) имеет право использовать ТОЛЬКО
// перечисленные ниже методы. Всё остальное — внутренности бэка.
// Любое изменение сигнатур/семантики ниже = breaking change для фронта,
// требует синхронного обновления UI и поднятия POWERCALC_BACKEND_API_VERSION.
//
// Расширение для 2.2 (extension point):
//   - SymbolTable (переменные формул документа) появится здесь же;
//     парсер документа читает значения через геттеры PowerSystem
//     (V_mag, delta, P_spec, Q_spec, потоки) + SymbolTable.
// =====================================================================

#define POWERCALC_BACKEND_API_VERSION 1

#include "powersystem.h"
#include "node.h"
#include "line.h"
#include "types.h"
#include "solver.h"
#include "csvparser.h"
#include "csvwriter.h"

// --------------------------- PowerSystem -----------------------------
// Конструктор: PowerSystem(double S_base, double V_base)  [ВА, В]
// Добавление:    addNode(const Node&), addLine(const Line&)   (throw на дубликат/битые ссылки)
// Удаление:      removeNode(NodeId), removeLine(LineId)       (throw: нет узла / у узла есть ветви)
// Доступ:        getNode(NodeId) [&/const], getLine(LineId) [&/const],
//                getNodes() [&/const], getLines() [const],
//                hasNode(NodeId), nodesCount(), linesCount()
// Включение:     connectLine(LineId), disconnectLine(LineId)
// Базисные:      S_base(), V_base(), V_base(NodeId), Z_base(), Y_base()
// о.е.:          R_oe/X_oe/Z_oe/Y_oe(const Line&), P_oe/Q_oe/V_oe(const Node&),
//                Y_shunt_from_oe/Y_shunt_to_oe(const Line&)
// Валидация:     validate()                     (throw: нет узлов / нет enabled Slack / нулевой импеданс)
// Расчёт:        buildYBus(), calculateLineFlows(), calculateSlackPower()
// Сброс:         clear()
//
// ------------------------------- Node --------------------------------
// Геттеры: id(), type(), P_spec(), Q_spec(), V_nom(), V_set(), V_mag(),
//          delta(), Q_min(), Q_max(), isEnabled()
// Сеттеры: setType(), setP_spec(), setQ_spec(), setV_set(), setV(),
//          setDelta(), setQ_min(), setQ_max(), connect(), disconnect()
// Фабрики: Node::makePQ / makePV / makeSlack
// Единицы: строго СИ (Вт, вар, В, рад)
//
// ------------------------------- Line --------------------------------
// Геттеры: id(), from(), to(), R(), X(), k_t(), Y(), istransformer(), isEnabled()
// Сеттеры: setR(), setX(), setY(), setKt(), setFrom(), setTo(), setTransformer()
// Вкл/выкл — ТОЛЬКО через PowerSystem::connectLine/disconnectLine
// Единицы: Ом, См; k_t безразмерный (конвенция: V_высш/V_низш)
//
// ---------------------------- Solver et al ---------------------------
// Solver(PowerSystem&), Solver(PowerSystem&, const Options&)
// Result solve();   Options{max_iterations, tolerance}; Result{converged, iterations, max_mismatch}
// Солвер НЕ мутирует типы/Q_spec узлов (PV→PQ конверсия — внутренняя).
//
// ------------------------- Парсер / райтер ---------------------------
// CsvParser::parseFiles(nodesCsv, branchesCsv, PowerSystem&) -> bool
// CsvParser::nodeNames()/lineNames()  (порядок = порядку добавления в PowerSystem)
// CsvWriter::write(system, nodeNames, lineNames, nodesPath, branchesPath) -> bool
// Конвенции границы: МВт/Мвар/кВ/град/мкСм; ktr = V_низш/V_высш; b со знаком RastrWin.
// =====================================================================

// ------------------------- Парсер / райтер ---------------------------
// CsvParser::parseFiles(nodesCsv, branchesCsv, PowerSystem&) -> bool
// CsvParser::nodeNames()/lineNames()  (порядок = порядку добавления в PowerSystem)
// CsvWriter::write(system, nodeNames, lineNames, nodesPath, branchesPath) -> bool
// Валидация: parseFiles проверяет заголовки (tip/ny/name для узлов, tip/ip/iq/name для ветвей)
// Конвенции границы: МВт/Мвар/кВ/град/мкСм; ktr = V_низш/V_высш; b со знаком RastrWin.