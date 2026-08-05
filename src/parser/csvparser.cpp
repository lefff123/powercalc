#include "csvparser.h"
#include "node.h"
#include "powersystem.h"
#include "types.h"
#include <QDebug>
#include <QTextCodec>
#include <complex>
#include <cstddef>
#include <qglobal.h>
#include <qobject.h>
#include <qstringconverter_base.h>
#include <cmath>

// "+J200", "10+J220", "-J555", "100" → комплекс (мкСм → См)
static std::complex<double> parseComplexYsh(const QString &str)
{
	QString s = str.trimmed().toUpper();
	s.remove(' ');
	if (s.isEmpty()) return {0.0, 0.0};

	double real = 0.0, imag = 0.0;
	const int jPos = s.indexOf('J');
	if (jPos >= 0) {
		QString realPart = s.left(jPos);
		double sign = 1.0;
		if (realPart.endsWith('-')) { sign = -1.0; realPart.chop(1); }
		else if (realPart.endsWith('+')) { realPart.chop(1); }
		imag = sign * s.mid(jPos + 1).toDouble();
		real = realPart.isEmpty() ? 0.0 : realPart.toDouble();
	} else {
		real = s.toDouble();
	}
	std::complex<double> result {real * 1e-6, imag * 1e-6};
	return result;
}

// Хелпер для безопасного парсинга чисел из CSV
double parseDouble(const QString& str, double default_val = 0.0) {
	if (str.trimmed().isEmpty()) return default_val;
	bool ok;
	double val = str.toDouble(&ok);
	return ok ? val : default_val;
}

size_t parseSize(const QString& str, size_t default_val = 0) {
	if (str.trimmed().isEmpty()) return default_val;
	bool ok;
	size_t val = str.toULongLong(&ok);
	return ok ? val : default_val;
}

// Перевод градусов в радианы
double degToRad(double deg) {
	return deg * M_PI / 180.0;
}

CsvParser::CsvParser(QObject* parent){
}

bool CsvParser::parseFiles(const QString& nodes_filepath, 
							const QString& branches_filepath,
							PowerSystem& system){
	if (nodes_filepath.isEmpty() || branches_filepath.isEmpty()){
		return false;
	}
	if (!parseNodes(nodes_filepath, system)){
		return false;
	}
	if (!parseBranches(branches_filepath, system)){
		return false;
	}
	return true;
}

bool CsvParser::parseNodes(const QString& filepath, PowerSystem& system){
	QFile file(filepath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
		qWarning() << "Не удалось открыть файл " << file.errorString();
		return false;
	}
	QTextStream in(&file);
	auto encoding = QStringConverter::encodingForName("Windows-1251");
	if (encoding.has_value()) {
		in.setEncoding(*encoding);
	}

	headers_ = parseHeaders(in.readLine());
	if (headers_.empty())
		return false;
		// Валидация: это должен быть файл узлов
	const QStringList requiredNodeHeaders = {"tip", "ny", "name", "uhom", "vzd"};
	for (const QString &h : requiredNodeHeaders) {
		if (headers_.find(h) == headers_.end()) {
			qWarning() << "Файл" << filepath << "не содержит заголовок" << h 
					   << "- это не файл узлов (возможно, передан branches.csv)";
			return false;
		}
	}
	names_nodes_.clear();
	// Множители для перевода МВт/Мвар -> Вт/Вар
	const double P_MULT = 1e6;
	const double Q_MULT = 1e6;
	const double V_MULT = 1e3; // кВ -> В

	while(!in.atEnd()){
		QString line = in.readLine();
		if (line.trimmed().isEmpty()) continue;
		QStringList row = line.split(';');
		
		if (row.size() <= headers_["tip"]) continue;
		
		int tip = row[headers_["tip"]].toInt();
		NodeId ny = parseSize(row[headers_["ny"]]);
		double uhom_kV = parseDouble(row[headers_["uhom"]]);
		double uhom_V = uhom_kV * V_MULT;
		double delta_deg = parseDouble(row[headers_["delta"]]);
		double delta_rad = degToRad(delta_deg);
		const QString name = row[headers_["name"]].trimmed();
		const bool disabled = parseSize(row[headers_["sta"]]) != 0;
		switch (tip){
			case 0: { // SLACK
				double vzd_kV = parseDouble(row[headers_["vzd"]], uhom_kV);
				double vzd_V = vzd_kV * V_MULT;
				Node node = Node::makeSlack(ny, vzd_V, delta_rad, uhom_V);
				std::complex<double> ysh = parseComplexYsh(row[headers_["Ysh"]]);
				if (ysh.real() == 0.0 && ysh.imag() == 0.0)
					ysh = std::complex<double>(0.0, parseDouble(row[headers_["bsh"]]) * 1e-6);
				node.setY_shunt(ysh);
				if (disabled) node.disconnect();
				system.addNode(node);
				break;
			}
			case 2: { // PV (генератор с фиксированным V)
				double pn = parseDouble(row[headers_["pn"]]) * P_MULT;
				double pg = parseDouble(row[headers_["pg"]]) * P_MULT;
				double vzd_kV = parseDouble(row[headers_["vzd"]], uhom_kV);
				double vzd_V = vzd_kV * V_MULT;
				double qmin = parseDouble(row[headers_["qmin"]]) * Q_MULT;
				double qmax = parseDouble(row[headers_["qmax"]]) * Q_MULT;
				
				// makePV(NodeId id, double P_spec, double V_set_volts, 
				//        double V_init_volts, double delta_init, double V_nom, 
				//        double Q_min, double Q_max)
				Node node = Node::makePV(ny, pg - pn, vzd_V, vzd_V, delta_rad, uhom_V, qmin, qmax);
				std::complex<double> ysh = parseComplexYsh(row[headers_["Ysh"]]);
				if (ysh.real() == 0.0 && ysh.imag() == 0.0)
					ysh = std::complex<double>(0.0, parseDouble(row[headers_["bsh"]]) * 1e-6);
				node.setY_shunt(ysh);
				if (disabled) node.disconnect();
				system.addNode(node);
				break;
			}
			default: { // PQ (нагрузка + генерация)
				double pn = parseDouble(row[headers_["pn"]]) * P_MULT;
				double qn = parseDouble(row[headers_["qn"]]) * Q_MULT;
				double pg = parseDouble(row[headers_["pg"]]) * P_MULT;
				double qg = parseDouble(row[headers_["qg"]]) * Q_MULT;
				
				// makePQ(NodeId id, double P_spec, double Q_spec, double V_init, 
				//        double delta_init = 0.0, double V_nom = 110e3)
				Node node = Node::makePQ(ny, pn - pg, qn - qg, uhom_V, delta_rad, uhom_V);
				std::complex<double> ysh = parseComplexYsh(row[headers_["Ysh"]]);
				if (ysh.real() == 0.0 && ysh.imag() == 0.0)
					ysh = std::complex<double>(0.0, parseDouble(row[headers_["bsh"]]) * 1e-6);
				node.setY_shunt(ysh);
				if (disabled) node.disconnect();
				system.addNode(node);
				break;
			}
		}
		names_nodes_.append(name);
	}
	return true;
}

std::unordered_map<QString, size_t> CsvParser::parseHeaders(const QString& line){
	std::unordered_map<QString, size_t> headers;
	QStringList row = line.split(';');
	size_t i = 0;
	for (auto& item : row){
		headers[item.trimmed()] = i;
		++i;
	}
	return headers;
}

bool CsvParser::parseBranches(const QString& filepath, PowerSystem& system){
	QFile file(filepath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
		qWarning() << "Не удалось открыть файл " << file.errorString();
		return false;
	}
	QTextStream in(&file);
	auto encoding = QStringConverter::encodingForName("Windows-1251");
	if (encoding.has_value()) {
		in.setEncoding(*encoding);
	}

	headers_ = parseHeaders(in.readLine());
	if (headers_.empty())
		return false;
	// Валидация: это должен быть файл ветвей
	const QStringList requiredBranchHeaders = {"tip", "ip", "iq", "name", "r", "x"};
	for (const QString &h : requiredBranchHeaders) {
		if (headers_.find(h) == headers_.end()) {
			qWarning() << "Файл" << filepath << "не содержит заголовок" << h 
					   << "- это не файл ветвей (возможно, передан nodes.csv)";
			return false;
		}
	}
	names_lines_.clear();
	LineId line_counter = 1;
	
	while(!in.atEnd()){
		QString line = in.readLine();
		if (line.trimmed().isEmpty()) continue;
		QStringList row = line.split(';');
		
		if (row.size() <= headers_["tip"]) continue;
		
		int tip = row[headers_["tip"]].toInt();
		bool is_transformer = (tip == 1);
		
		NodeId ip = parseSize(row[headers_["ip"]]);
		NodeId iq = parseSize(row[headers_["iq"]]);
		
		double r = parseDouble(row[headers_["r"]]);
		double x = parseDouble(row[headers_["x"]]);
		double g = parseDouble(row[headers_["g"]]) * 1e-6;
		double b = -parseDouble(row[headers_["b"]]) * 1e-6;
		
		// Для трансформаторов берем ktr, для линий = 1.0
		double ktr_raw = is_transformer ? parseDouble(row[headers_["ktr"]], 1.0) : 1.0;
		if (ktr_raw == 0) ktr_raw = 1.0;
		// RastrWin хранит V_to/V_from, бэк ждёт V_from/V_to
		double ktr_val = 1.0 / ktr_raw;
		std::complex<double> ktr(ktr_val, 0.0);
		
		// Уникальный ID для каждой ветви
		LineId branch_id = line_counter++;
		const QString name = row[headers_["name"]].trimmed();
		Line branch{branch_id, ip, iq, r, x, ktr, std::complex<double>(g, b), is_transformer};

		system.addLine(branch);
		if (parseSize(row[headers_["sta"]]) != 0)
			system.disconnectLine(branch_id);

		names_lines_.append(name);
	}
	return true;
}