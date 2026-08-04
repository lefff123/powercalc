#include "powersystem.h"


PowerSystem::PowerSystem(double S_base, double V_base)
	: S_base_(S_base), V_base_(V_base), Z_base_((V_base * V_base / S_base)), Y_base_(1 / Z_base_)
{
	if (S_base_ <= 0) {
		throw std::invalid_argument("Base power should be not below 0!");
	}
	if (V_base_ < 0) {
		throw std::invalid_argument("Base voltage should be not below 0!");
	}
}

// Добавление элементов
void PowerSystem::addNode(const Node &node)
{
	if (id_to_index_.count(node.id())) {
		throw std::invalid_argument("Node with id " + std::to_string(node.id()) + " already exists");
	}
	id_to_index_[node.id()] = nodes_.size();
	nodes_.push_back(node);
	base_voltages_valid_ = false;
}

void PowerSystem::addLine(const Line &line)
{
	if (findLineIndex(line.id()).has_value()) {
		throw std::invalid_argument("Line with id " + std::to_string(line.id()) + " already exists");
	}
	if (!hasNode(line.from())) {
		throw std::invalid_argument("Line references non-existent node: " + std::to_string(line.from()));
	}
	if (!hasNode(line.to())) {
		throw std::invalid_argument("Line references non-existent node: " + std::to_string(line.to()));
	}
	lines_.push_back(line);
	base_voltages_valid_ = false;
}

// Доступ к элементам
const Node& PowerSystem::getNode(NodeId id) const
{
	auto idx = getNodeIndex(id);
	return nodes_[idx];
}
Node& PowerSystem::getNode(NodeId id)
{
	auto idx = getNodeIndex(id);
	return nodes_[idx];
}

const Line& PowerSystem::getLine(LineId id) const
{
	auto idx = findLineIndex(id);
	if (!idx.has_value()) {
		throw std::out_of_range("Node not found: " + std::to_string(id));
	}
	return lines_[*idx]; // *idx — разыменовываем optional
}

Line& PowerSystem::getLine(LineId id) 
{
	auto idx = findLineIndex(id);
	if (!idx.has_value()) {
		throw std::out_of_range("Node not found: " + std::to_string(id));
	}
	return lines_[*idx]; // *idx — разыменовываем optional
}
std::vector<Node>& PowerSystem::getNodes()
{
	return nodes_;
}
const std::vector<Node>& PowerSystem::getNodes() const
{
	return nodes_;
}
const std::vector<Line>& PowerSystem::getLines() const
{
	return lines_;
}

size_t PowerSystem::nodesCount() const
{
	return nodes_.size();
}
size_t PowerSystem::linesCount() const
{
	return lines_.size();
}

// Базисные величины
double PowerSystem::S_base() const
{
	return S_base_;
}
double PowerSystem::V_base() const
{
	return V_base_;
}
//Возвращаем базу для каждого узла отдельно
double PowerSystem::V_base(NodeId id) const { //NodeId == size_t
	recalculateBaseVoltages();
	return V_base_per_node_[getNodeIndex(id)]; 
}
double PowerSystem::Z_base() const
{
	return Z_base_;
}
double PowerSystem::Y_base() const
{
	return Y_base_;
}

// Конвертация в o.e.
double PowerSystem::PowerSystem::R_oe(const Line &line) const
{
	return line.R() / Z_base_;
}
double PowerSystem::X_oe(const Line &line) const
{
	return line.X() / Z_base_;
}
std::complex<double> PowerSystem::Z_oe(const Line &line) const
{
	return std::complex<double>(R_oe(line), X_oe(line));
}
std::complex<double> PowerSystem::Y_oe(const Line &line) const
{
	return std::complex<double>(1) / Z_oe(line);
}

double PowerSystem::P_oe(const Node &node) const
{
	return node.P_spec() / S_base_;
}
double PowerSystem::Q_oe(const Node &node) const
{
	return node.Q_spec() / S_base_;
}
double PowerSystem::V_oe(const Node &node) const
{
	recalculateBaseVoltages();
	return node.V_mag() /  V_base_per_node_[getNodeIndex(node.id())];
}
std::complex<double> PowerSystem::Y_shunt_from_oe(const Line& line) const {
	size_t i = getNodeIndex(line.from());
	double Y_base = S_base_ / (V_base_per_node_[i] * V_base_per_node_[i]);
		if (line.istransformer()) {
			// Г-образная: весь шунт со стороны from
			return line.Y() / Y_base;
		} else {
			// Π-образная: шунт делится пополам
			return (line.Y() / 2.0) / Y_base;
		}
}
std::complex<double> PowerSystem::Y_shunt_to_oe(const Line& line) const {
	size_t j = getNodeIndex(line.to());
	double Y_base = S_base_ / (V_base_per_node_[j] * V_base_per_node_[j]);
	
	if (line.istransformer()) {
		// Г-образная: шунта со стороны Т нет
		return std::complex<double>(0.0, 0.0);
	} else {
		// Π-образная: шунт делится пополам
		return (line.Y() / 2.0) / Y_base;
	}
}


// Валидация сети
void PowerSystem::validate() const
{
	if (nodes_.empty()) {
		throw std::invalid_argument("Network has no nodes");
	}
	for (const auto &node : nodes_) {
		if (node.type() == NodeType::SLACK)
			return; // Нашли Slack — всё ок
	}
	throw std::invalid_argument("Network must have at least one Slack node");
}

// Создание матрицы проводимостей
Matrix<std::complex<double>> PowerSystem::buildYBus() const
{
	Matrix<std::complex<double>> Y_bus(nodes_.size(), nodes_.size());
	recalculateBaseVoltages();
	// заполняем проводимостями
	for (const auto &line : lines_) {
		if (!line.isEnabled())
			continue;
		auto idx_from = getNodeIndex(line.from());
		auto idx_to = getNodeIndex(line.to());

		// Эффективный коэффициент трансформации в о.е.
		std::complex<double> k_pu = line.k_t() * V_base_per_node_[idx_to] /
									V_base_per_node_[idx_from];
		std::complex<double> k_pu_conj = std::conj(k_pu);
		double k_pu_abs_sq = std::norm(k_pu); // |k_pu|²
		std::complex<double> y = Y_oe(line);

		// диагональные элементы матрицы
		Y_bus(idx_from, idx_from) += y / k_pu_abs_sq; // Y_ii = y / |k_pu|²
		Y_bus(idx_to, idx_to) += y;                   // Y_jj = y

		// недиагональные элементы
		Y_bus(idx_from, idx_to) -= y / k_pu_conj; // Y_ij = -y / k_pu*
		Y_bus(idx_to, idx_from) -= y / k_pu;      // Y_ji = -y / k_pu

		//шунтовые проводимости
		Y_bus(idx_from, idx_from) += Y_shunt_from_oe(line); // Y_ii = y / |k_pu|²
		Y_bus(idx_to, idx_to) += Y_shunt_to_oe(line);     // Y_jj = y
	}
	return Y_bus;
}

size_t PowerSystem::getNodeIndex(NodeId id) const
{
	auto it = id_to_index_.find(id);
	if (it == id_to_index_.end()) {
		throw std::out_of_range("Node not found: " + std::to_string(id));
	}
	return it->second;
}

bool PowerSystem::hasNode(NodeId id) const
{
	return id_to_index_.count(id) > 0;
}

std::vector<LineFlows> PowerSystem::calculateLineFlows() const{
	recalculateBaseVoltages();

	std::vector<LineFlows> flows;
	flows.reserve(lines_.size());
	for (auto line : lines_){
		if (!line.isEnabled()) continue;
		//находим индексы точек по id
		size_t i = getNodeIndex(line.from());
		size_t j = getNodeIndex(line.to());
		
		//вычисляем комплексные напряжения в точках
		std::complex<double> V_i = (nodes_[i].V_mag() / V_base_per_node_[i]) * std::complex<double>(std::cos(nodes_[i].delta()), std::sin(nodes_[i].delta()));
		std::complex<double> V_j = (nodes_[j].V_mag() / V_base_per_node_[j]) * std::complex<double>(std::cos(nodes_[j].delta()), std::sin(nodes_[j].delta()));
				
		// Эффективный коэффициент трансформации в о.е.
		std::complex<double> k_pu = line.k_t() * V_base_per_node_[j] / V_base_per_node_[i];
		std::complex<double> k_pu_conj = std::conj(k_pu);
		double k_pu_abs_sq = std::norm(k_pu);

		//Подготовим данные к расчету
		std::complex<double> y = Y_oe(line);

		// Токи 
		auto I_from = (y / k_pu_abs_sq) * V_i - (y / k_pu_conj) * V_j + Y_shunt_from_oe(line)*V_i;
		auto I_to = -(y / k_pu) * V_i + y * V_j + Y_shunt_to_oe(line) * V_j;
		auto S_from_pu = V_i * std::conj(I_from);
		auto S_to_pu = V_j * std::conj(I_to);
		flows.push_back(LineFlows(line.id(), line.from(), line.to(), S_from_pu * S_base_, S_to_pu * S_base_, (S_from_pu + S_to_pu) * S_base_));
	}
	return flows;
}

// Расчёт комплексной мощности в Slack-узле
std::complex<double> PowerSystem::calculateSlackPower() const {
	auto Y_bus = buildYBus();
	size_t n = nodes_.size();
	
	// Находим Slack-узел
	size_t slack_idx = 0;
	for (size_t i = 0; i < n; ++i) {
		if (nodes_[i].type() == NodeType::SLACK) {
			slack_idx = i;
			break;
		}
	}
	
	// Собираем комплексные напряжения
	std::vector<std::complex<double>> V(n);
	for (size_t i = 0; i < n; ++i) {
		double v_pu = nodes_[i].V_mag() / V_base_per_node_[i];
		double delta = nodes_[i].delta();
		V[i] = v_pu * std::complex<double>(std::cos(delta), std::sin(delta));
	}
	
	// S_slack = V_slack * conj(sum(Y_slack,j * V_j))
	std::complex<double> I_slack(0.0, 0.0);
	for (size_t j = 0; j < n; ++j) {
		I_slack += Y_bus(slack_idx, j) * V[j];
	}
	
	std::complex<double> S_slack_pu = V[slack_idx] * std::conj(I_slack);
	
	// Переводим в ВА
	return S_slack_pu * S_base_;
}

// Отключение линии с инвалидацией кэша
void PowerSystem::disconnectLine(LineId id) {
	auto idx = findLineIndex(id);
	if (!idx.has_value()) {
		throw std::out_of_range("Line not found: " + std::to_string(id));
	}
	lines_[*idx].disconnect();
	base_voltages_valid_ = false;  //  Сбрасываем кэш
}

// Включение линии с инвалидацией кэша
void PowerSystem::connectLine(LineId id) {
	auto idx = findLineIndex(id);
	if (!idx.has_value()) {
		throw std::out_of_range("Line not found: " + std::to_string(id));
	}
	lines_[*idx].connect();
	base_voltages_valid_ = false;  // Сбрасываем кэш
}

std::optional<size_t> PowerSystem::findLineIndex(LineId id) const
{
	for (size_t i = 0; i < lines_.size(); ++i) {
		if (id == lines_[i].id()) {
			return i;
		}
	}
	return std::nullopt;
}
void PowerSystem::recalculateBaseVoltages() const{
	if (base_voltages_valid_) return;
	V_base_per_node_.resize(nodes_.size());
	
	// 1. Найти Slack-узел (индекс)
	std::optional<size_t> slack_idx;  // size_t — индекс
	for (size_t i = 0; i < nodes_.size(); ++i) {
		if (nodes_[i].type() == NodeType::SLACK && nodes_[i].isEnabled()) {
			slack_idx = i;
			break;
		}
	}
	
	if (!slack_idx.has_value()) {
		throw std::runtime_error("No Slack node found");
	}
	
	// 2. Инициализация
	V_base_per_node_[*slack_idx] = nodes_[*slack_idx].V_nom();
	std::vector<bool> visited(nodes_.size(), false);
	std::queue<size_t> q;  //  Храним индексы
	q.push(*slack_idx);
	visited[*slack_idx] = true;
	
	// 3. BFS обход
	while (!q.empty()) {
		size_t current = q.front();  // current — это индекс
		q.pop();
		
		for (const auto& line : lines_) {
			if (!line.isEnabled()) continue;
			
			size_t i = getNodeIndex(line.from());  //  Индекс узла from
			size_t j = getNodeIndex(line.to());     //  Индекс узла to
			
			// Переход от current к j (линия from → to)
			if (i == current && !visited[j]) {  //  Сравниваем индексы
				V_base_per_node_[j] = V_base_per_node_[current] / std::abs(line.k_t());
				visited[j] = true;  //  Помечаем соседа
				q.push(j);
			}
			// Переход от current к i (линия to → from, т.е. обратное направление)
			else if (j == current && !visited[i]) {  //  Сравниваем индексы
				V_base_per_node_[i] = V_base_per_node_[current] * std::abs(line.k_t());  
				visited[i] = true;  //  Помечаем соседа
				q.push(i);
			}
		}
		
	}
	
	// 4. Проверка связности
	for (size_t i = 0; i < nodes_.size(); ++i) {
		if (!visited[i]) {
			throw std::runtime_error("Network is disconnected: node " + 
									std::to_string(nodes_[i].id()) + " unreachable");
		}
	}
	base_voltages_valid_ = true;
}

void PowerSystem::clear() {
	nodes_.clear();
	lines_.clear();
	id_to_index_.clear();
	base_voltages_valid_ = false;
}