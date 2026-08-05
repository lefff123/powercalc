#include "csvwriter.h"

#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QStringList>
#include <cmath>

static QString num(double v)
{
	return QString::number(v, 'g', 10);
}

bool RastrWriter::write(const PowerSystem &sys,
						const QMap<size_t, QString> &nodeNames,
						const QMap<size_t, QString> &lineNames,
						const QString &nodesPath,
						const QString &branchesPath)
{
	// Узлы
	QFile nf(nodesPath);
	if (!nf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream out(&nf);
	auto enc = QStringConverter::encodingForName("Windows-1251");
	if (enc) out.setEncoding(*enc);

	out << "sel;sta;tip;ny;name;uhom;pn;qn;pg;qg;vzd;qmin;qmax;bsh;vras;delta;npa;Ysh\n";
	for (const Node &n : sys.getNodes()) {
		int tip = 1;
		if (n.type() == NodeType::SLACK) tip = 0;
		else if (n.type() == NodeType::PV) tip = 2;

		double pn = 0, qn = 0, pg = 0, qg = 0;
		if (n.type() == NodeType::PQ) {
			if (n.P_spec() >= 0) pn = n.P_spec() / 1e6; else pg = -n.P_spec() / 1e6;
			if (n.Q_spec() >= 0) qn = n.Q_spec() / 1e6; else qg = -n.Q_spec() / 1e6;
		} else if (n.type() == NodeType::PV) {
			if (n.P_spec() >= 0) pg = n.P_spec() / 1e6; else pn = -n.P_spec() / 1e6;
		}

		const double gsh = n.Y_shunt().real() * 1e6;   // real (conductance)
		const double bsh = n.Y_shunt().imag() * 1e6;   // imag (susceptance)

		QString yshStr;
		if (gsh != 0.0 || bsh != 0.0) {
			if (gsh != 0.0)
				yshStr = QString("%1%2J%3")
					.arg(num(gsh))
					.arg(bsh >= 0 ? "+" : "-")
					.arg(num(std::abs(bsh)));
			else
				yshStr = QString("%1J%2").arg(bsh >= 0 ? "+" : "-").arg(num(std::abs(bsh)));
		}

		QStringList row;
		row << "0"
			<< (n.isEnabled() ? "0" : "1")
			<< QString::number(tip)
			<< QString::number(n.id())
			<< nodeNames.value(n.id(), QString::number(n.id()))
			<< num(n.V_nom() / 1e3)
			<< num(pn) << num(qn) << num(pg) << num(qg)
			<< (n.type() != NodeType::PQ ? num(n.V_set() / 1e3) : QString())
			<< (n.type() == NodeType::PV ? num(n.Q_min() / 1e6) : QString())
			<< (n.type() == NodeType::PV ? num(n.Q_max() / 1e6) : QString())
			<< (bsh != 0 ? num(bsh) : QString())           // bsh
			<< num(n.V_mag() / 1e3)
			<< num(n.delta() * 180.0 / M_PI)
			<< "0"
			<< yshStr;                                       // Ysh
		out << row.join(';') << "\n";
	}
	nf.close();

	// Ветви
	QFile bf(branchesPath);
	if (!bf.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	QTextStream bout(&bf);
	if (enc) bout.setEncoding(*enc);

	bout << "sel;sta;tip;ip;iq;np;groupid;name;r;x;g;b;ktr;n_anc;bd;pl_ip;ql_ip;na;i_max;i_zag\n";
	for (const Line &l : sys.getLines()) {
		QStringList row;
		row << "0"
			<< (l.isEnabled() ? "0" : "1")
			<< (l.istransformer() ? "1" : "0")
			<< QString::number(l.from())
			<< QString::number(l.to())
			<< "0" << "0"
			<< lineNames.value(l.id(), QString::number(l.id()))
			<< num(l.R()) << num(l.X())
			<< num(l.Y().real() * 1e6)
			<< (l.Y().imag() != 0 ? num(-l.Y().imag() * 1e6) : QString())
			<< (l.istransformer() ? num(1.0 / l.k_t().real()) : QString())
			<< "0" << "0" << QString() << QString() << "0" << QString() << QString();
		bout << row.join(';') << "\n";
	}
	return true;
}