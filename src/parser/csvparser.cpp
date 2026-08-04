#include "csvparser.h"
#include "node.h"
#include "powersystem.h"
#include "types.h"
#include <QDebug>
#include <QTextCodec>
#include <cstddef>
#include <qglobal.h>
#include <qobject.h>
#include <qstringconverter_base.h>

CsvParser::CsvParser(double S_base, QObject* parent){
	S_base_ = S_base;
	names_nodes_ = {"tip", "ny", "uhom", "pn", "qn", "pg", "qg", "qmin", "qmax", "delta"};
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

int figure_type(QString& type){

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
			switch (row[headers_["tip"]].toInt()){
				case 0 :{ //BASE
					Node node = Node::makeSlack(row[headers_["ny"]].toInt(), 
												row[headers_["uhom"]].toDouble(), row[headers_["delta"]].toDouble());
					system_->addNode(node);
					break;
				}
				default: { //LOAD + GENERATION
					Node node1 = Node::makePQ(row[headers_["ny"]].toInt(), 
											row[headers_["pn"]].toDouble() - row[headers_["pg"]].toDouble(), 
											row[headers_["qn"]].toDouble() - row[headers_["qg"]].toDouble(), 
											row[headers_["uhom"]].toDouble());
					system_->addNode(node1);
					break;
				}
			};
		}
	}
	return true;
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
		//names_lines_ = {"tip", "ip", "iq", "r", "x", "g", "b", "ktr"};
		// sel;sta;tip;ip;iq;np;groupid;name;r;x;g;b;ktr;n_anc;bd;pl_ip;ql_ip;na;i_max;i_zag
		for (size_t i = 0; i < row.size(); ++i){
			Line line();
			
		}
	}
	return true;
}

bool CsvParser::parseDouble(const QString& str, double& value){

}

bool parseInt(const QString& str, int& value){

}