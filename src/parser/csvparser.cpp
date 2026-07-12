#include "csvparser.h"
#include "powersystem.h"
#include "types.h"

CsvParser::CsvParser(double S_base, QObject* parent){
    
}

bool CsvParser::parseFiles(const QString& nodes_filepath, const QString& branches_filepath){

}

PowerSystem& CsvParser::getSystem(){

}

void CsvParser::clear(){

}

bool CsvParser::parseNodes(const QString& filepath){

}

bool CsvParser::parseBranches(const QString& filepath){

}

NodeType CsvParser::determineNodeType(int tip, double pg){

}

bool CsvParser::parseDouble(const QString& str, double& value){

}

bool parseInt(const QString& str, int& value){

}