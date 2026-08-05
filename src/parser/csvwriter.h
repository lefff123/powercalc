#pragma once

#include <QMap>
#include <QString>
#include "powersystem.h"

class RastrWriter
{
public:
	static bool write(const PowerSystem &system,
					  const QMap<size_t, QString> &nodeNames,
					  const QMap<size_t, QString> &lineNames,
					  const QString &nodesPath,
					  const QString &branchesPath);
};