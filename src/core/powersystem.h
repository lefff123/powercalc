#pragma once

#include "line.h"
#include "node.h"
#include "types.h"
#include <complex>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>
#include <algorithm>


class PowerSystem {
public:
  PowerSystem(double S_base, double V_base) : S_base_(S_base), V_base_(V_base), Z_base_((V_base*V_base/S_base)), Y_base_(1/Z_base_) {
    if (S_base_ <= 0){
        throw std::invalid_argument("Base power should be not below 0!");
    }
    if (V_base_ < 0){
      throw std::invalid_argument("Base voltage should be not below 0!");
    }
  }

  // Добавление элементов
  void addNode(const Node &node) {
    if (findNodeIndex(node.id()).has_value()) {
      throw std::invalid_argument("Node with id " + std::to_string(node.id()) +
                                  " already exists");
    }
    nodes_.push_back(node);
  }

  void addLine(const Line &line) {
    if (findLineIndex(line.id()).has_value()) {
      throw std::invalid_argument("Line with id " + std::to_string(line.id()) +
                                  " already exists");
    }
    // Проверка существования узлов
    if (!findNodeIndex(line.from()).has_value()) {
      throw std::invalid_argument("Line references non-existent node: " +
                                  std::to_string(line.from()));
    }
    if (!findNodeIndex(line.to()).has_value()) {
      throw std::invalid_argument("Line references non-existent node: " +
                                  std::to_string(line.to()));
    }
    lines_.push_back(line);
  }

  // Доступ к элементам
  const Node &getNode(NodeId id) const {
    auto idx = findNodeIndex(id);
    if (!idx.has_value()) {
      throw std::out_of_range("Node not found: " + std::to_string(id));
    }
    return nodes_[*idx]; // *idx — разыменовываем optional
  }
   Node &getNode(NodeId id){
    auto idx = findNodeIndex(id);
    if (!idx.has_value()) {
      throw std::out_of_range("Node not found: " + std::to_string(id));
    }
    return nodes_[*idx]; // *idx — разыменовываем optional
  }

  const Line &getLine(LineId id) const {
    auto idx = findLineIndex(id);
    if (!idx.has_value()) {
      throw std::out_of_range("Node not found: " + std::to_string(id));
    }
    return lines_[*idx]; // *idx — разыменовываем optional
  }

  const std::vector<Node> &getNodes() const{
    return nodes_;
  }
  const std::vector<Line> &getLines() const{
    return lines_;
  }

  size_t nodesCount() const{
    return nodes_.size();
  }
  size_t linesCount() const{
    return lines_.size();
  }

  // Базисные величины
  double S_base() const{
    return  S_base_;
  }
  double V_base() const{
    return V_base_;
  }
  double Z_base() const{
    return Z_base_;
  }
  double Y_base() const{
    return Y_base_;
  }

  // Конвертация в o.e.
  double R_oe(const Line &line) const{
    return line.R()/Z_base_;
  }
  double X_oe(const Line &line) const{
    return line.X()/Z_base_;
  }
  std::complex<double> Z_oe(const Line &line) const{
    return  std::complex<double>(R_oe(line), X_oe(line));
  }
  std::complex<double> Y_oe(const Line &line) const{
    return std::complex<double>(1)/Z_oe(line);
  }

  double P_oe(const Node &node) const{
    return node.P_spec() / S_base_;
  }
  double Q_oe(const Node &node) const{
    return node.Q_spec()/S_base_;
  }
  double V_oe(const Node &node) const {
    return node.V_mag() / V_base_; 
  }

  // Валидация сети
  void validate() const{
    if (nodes_.size() > 0){
        bool is_slack =false;
        for (size_t i = 0; i< nodes_.size(); ++i){
            if (nodes_[i].type() == NodeType::SLACK){
                is_slack = true;
            }
        }
        if (is_slack){
            return;
        }
    }
    throw std::invalid_argument("В сети нет узлов либо нет базисного!");
  }

private:
  std::vector<Node> nodes_;
  std::vector<Line> lines_;

  double S_base_;
  double V_base_;
  double Z_base_;
  double Y_base_;

  std::optional<size_t> findNodeIndex(NodeId id) const{
    for (size_t i = 0; i < nodes_.size(); ++i){
        if (id == nodes_[i].id()){
            return i;
        }
    }
    return std::nullopt;
  }
  std::optional<size_t> findLineIndex(LineId id) const{
    for (size_t i = 0; i < lines_.size(); ++i) {
      if (id == lines_[i].id()) {
        return i;
      }
    }
    return std::nullopt;
  }
};