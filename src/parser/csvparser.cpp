#include "csvparser.h"
#include "powersystem.h"
#include "types.h"
#include <cstddef>
#include <qglobal.h>
#include <qobject.h>
#include <qstringconverter_base.h>
#include <QTextCodec>

CsvParser::CsvParser(double S_base, QObject* parent){
    S_base_ = S_base;
    names_nodes_ = {"tip", "uhom", "pn", "qn", "pg", "qg", "qmin", "qmax", "delta"};
	// sel;sta;tip;ny;name;uhom;pn;qn;pg;qg;vzd;qmin;qmax;bsh;vras;delta;npa;Ysh
    names_lines_ = {"tip", "ip", "iq", "r", "x", "g", "b", "ktr"};
    // sel;sta;tip;ip;iq;np;groupid;name;r;x;g;b;ktr;n_anc;bd;pl_ip;ql_ip;na;i_max;i_zag
}
bool CsvParser::parseFiles(const QString& nodes_filepath, const QString& branches_filepath){
	
}

PowerSystem& CsvParser::getSystem(){

}

void CsvParser::clear(){

}

bool CsvParser::parseNodes(const QString& filepath){
	QFile file(filepath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
		qWarning() << "Не удалось открыть файл " << file.errorString();
		return false;
	}
	QTextStream in(&file);
	in.setEncoding(QStringConverter::encodingForName("Windows-1251").value());

	parseHeaders(in.readLine());
	if (headers_.empty())
		return false;

	while(!in.atEnd()){
		QString line = in.readLine();
		QStringList row = line.split(';');
		for (size_t i = 0; i < row.size(); ++i){
			
		}
	}
}

std::unordered_map<QString, size_t> parseHeaders(const QString& line){
	std::unordered_map<QString, size_t> headers;
	QStringList row = line.split(';');
	size_t i = 0;
	for (auto& item : row){
		headers[item] = i;
		++i;
	}
	return headers;
}

bool CsvParser::parseBranches(const QString& filepath){

}

NodeType CsvParser::determineNodeType(int tip, double pg){

}

bool CsvParser::parseDouble(const QString& str, double& value){

}

bool parseInt(const QString& str, int& value){

}