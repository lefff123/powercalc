#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "csvparser.h"
#include "powersystem.h"
#include "node.h"
#include "line.h"
#include <iostream>
#include <iomanip>

namespace fs = std::filesystem;

class CsvParserTest : public ::testing::Test {
protected:
    std::string nodes_path_;
    std::string branches_path_;

    void SetUp() override {
        std::string nodes_content = 
            "sel;sta;tip;ny;name;uhom;pn;qn;pg;qg;vzd;qmin;qmax;bsh;vras;delta;npa;Ysh\n"
            "0;0;1;1347;Onda-110;110;120.0;113.0;;;;;;200.0;121.03;0.60;0;+J200\n"
            "0;0;1;1349;Bologoe2;110;49.0;27.0;;;;;;;117.20;1.29;0;100\n"
            "0;0;1;319;Kir-330;330;822.0;200.0;;;;;;500.0;352.10;9.26;0;+J500\n"
            "0;0;1;347;Onda;330;;;;;;;;220.0;328.45;1.21;0;10+J220\n"
            "0;0;1;349;Bologoe1;330;;;;;;;;;336.73;2.27;0;\n"
            "0;0;1;392;Tupik;330;490.0;50.0;;;;;;-555.0;323.23;-9.32;0;-J555\n"
            "0;0;2;1319;Station1;21;55.0;40.0;1700.0;767.3;21.2;-100.0;1100.0;;21.20;15.68;0;\n"
            "0;0;2;1320;Station2;6;120.0;40.0;200.0;118.5;6.3;-50.0;600.0;;6.30;3.66;0;\n"
            "0;0;0;330;Balanced;330;;;-156.4;-72.8;;;;;330.00;;0;\n"
            "0;0;1;2201;Inverter;220;;;;;;;;;219.89;-0.51;0;\n"
            "0;0;0;2200;BaseDC;220;;;50.1;-3.9;;;;;220.00;;0;\n";

        std::string branches_content = 
            "sel;sta;tip;ip;iq;np;groupid;name;r;x;g;b;ktr;n_anc;bd;pl_ip;ql_ip;na;i_max;i_zag\n"
            "0;0;1;319;1319;0;0;Kir-330 - Station1;0.22;8.92;;;0.058;0;0;1640;515;0;2818;\n"
            "0;0;1;347;1347;0;0;Onda - Onda-110;0.80;27.00;;;0.375;0;0;-44;-68;0;143;\n"
            "0;0;0;349;319;0;0;Bologoe1 - Kir-330;1.79;18.24;;-192.0;;0;0;807;167;0;1413;\n"
            "0;0;0;349;330;0;0;Bologoe1 - Balanced;3.53;29.30;;-344.0;;0;0;-157;-42;0;302;\n"
            "0;0;0;349;347;0;0;Bologoe1 - Onda;4.14;25.30;;-261.0;;0;0;-96;-81;0;250;\n"
            "0;0;0;349;392;0;0;Bologoe1 - Tupik;7.29;44.11;;-465.0;;0;0;-507;-43;0;875;\n"
            "0;0;1;349;1349;0;0;Bologoe1 - Bologoe2;1.50;41.20;;;0.348;0;0;-47;-1;0;81;\n"
            "0;0;1;1347;1320;0;0;Onda-110 - Station2;0.48;10.80;;;0.049;0;0;80;70;0;506;\n"
            "0;0;0;1347;1349;0;0;Onda-110 - Bologoe2;9.90;17.70;;-449.0;;0;0;-4;-21;0;131;\n"
            "0;0;0;2200;2201;0;0;BaseDC - Inverter;1.20;8.50;;;;0;0;-50;4;0;132;\n";

        nodes_path_ = fs::temp_directory_path() / "rastr_nodes.csv";
        branches_path_ = fs::temp_directory_path() / "rastr_vetvi.csv";

        std::ofstream nodes_file(nodes_path_);
        nodes_file << nodes_content;
        nodes_file.close();

        std::ofstream branches_file(branches_path_);
        branches_file << branches_content;
        branches_file.close();
    }

    void TearDown() override {
        fs::remove(nodes_path_);
        fs::remove(branches_path_);
    }
};

TEST_F(CsvParserTest, ParseFilesSuccessfully) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    
    bool result = parser.parseFiles(QString::fromStdString(nodes_path_), 
                                     QString::fromStdString(branches_path_), 
                                     system);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(system.nodesCount(), 11u);
    EXPECT_EQ(system.linesCount(), 10u);
}

TEST_F(CsvParserTest, CheckNodeTypes) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    int slack_count = 0, pv_count = 0, pq_count = 0;
    for (const auto& node : system.getNodes()) {
        if (node.type() == NodeType::SLACK) slack_count++;
        else if (node.type() == NodeType::PV) pv_count++;
        else if (node.type() == NodeType::PQ) pq_count++;
    }

    EXPECT_EQ(slack_count, 2);
    EXPECT_EQ(pv_count, 2);
    EXPECT_EQ(pq_count, 7);
}

TEST_F(CsvParserTest, CheckSlackNode) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    const Node& slack = system.getNode(330);
    EXPECT_EQ(slack.type(), NodeType::SLACK);
    EXPECT_DOUBLE_EQ(slack.V_nom(), 330.0e3);
    EXPECT_DOUBLE_EQ(slack.V_set(), 330.0e3);
    EXPECT_DOUBLE_EQ(slack.P_spec(), 0.0);
    EXPECT_DOUBLE_EQ(slack.Q_spec(), 0.0);
}

TEST_F(CsvParserTest, CheckPVNode) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    const Node& pv = system.getNode(1319);
    EXPECT_EQ(pv.type(), NodeType::PV);
    EXPECT_DOUBLE_EQ(pv.V_nom(), 21.0e3);
    EXPECT_DOUBLE_EQ(pv.P_spec(), (1700.0 - 55.0) * 1e6);
    EXPECT_DOUBLE_EQ(pv.V_set(), 21.2e3);
    EXPECT_DOUBLE_EQ(pv.Q_min(), -100.0e6);
    EXPECT_DOUBLE_EQ(pv.Q_max(), 1100.0e6);
}

TEST_F(CsvParserTest, CheckPQNode) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    const Node& pq = system.getNode(1347);
    EXPECT_EQ(pq.type(), NodeType::PQ);
    EXPECT_DOUBLE_EQ(pq.V_nom(), 110.0e3);
    EXPECT_DOUBLE_EQ(pq.P_spec(), 120.0e6);
    EXPECT_DOUBLE_EQ(pq.Q_spec(), 113.0e6);
}

TEST_F(CsvParserTest, CheckTransformerLine) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    const Line& trafo = system.getLine(1);
    EXPECT_TRUE(trafo.istransformer());
    EXPECT_EQ(trafo.from(), 319u);
    EXPECT_EQ(trafo.to(), 1319u);
    EXPECT_DOUBLE_EQ(trafo.R(), 0.22);
    EXPECT_DOUBLE_EQ(trafo.X(), 8.92);
    EXPECT_DOUBLE_EQ(trafo.k_t().real(), 1.0 / 0.058);
    EXPECT_DOUBLE_EQ(trafo.k_t().imag(), 0.0);
}

TEST_F(CsvParserTest, CheckRegularLine) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    const Line& line = system.getLine(3);
    EXPECT_FALSE(line.istransformer());
    EXPECT_EQ(line.from(), 349u);
    EXPECT_EQ(line.to(), 319u);
    EXPECT_DOUBLE_EQ(line.R(), 1.79);
    EXPECT_DOUBLE_EQ(line.X(), 18.24);
    EXPECT_DOUBLE_EQ(line.k_t().real(), 1.0);
    EXPECT_DOUBLE_EQ(line.Y().imag(), 192.0e-6);
}

TEST_F(CsvParserTest, PrintAllNodes) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    std::cout << "\n========== ALL NODES ==========\n";
    std::cout << std::fixed << std::setprecision(2);
    
    for (const auto& node : system.getNodes()) {
        std::string typeStr;
        switch (node.type()) {
            case NodeType::SLACK: typeStr = "SLACK"; break;
            case NodeType::PV: typeStr = "PV   "; break;
            case NodeType::PQ: typeStr = "PQ   "; break;
        }
        
        std::cout << "Node " << std::setw(5) << node.id() 
                  << " [" << typeStr << "]"
                  << " | V_nom=" << std::setw(8) << node.V_nom()/1e3 << " kV"
                  << " | V_mag=" << std::setw(8) << node.V_mag()/1e3 << " kV"
                  << " | delta=" << std::setw(7) << node.delta() * 180.0 / M_PI << " deg"
                  << " | P=" << std::setw(10) << node.P_spec()/1e6 << " MW"
                  << " | Q=" << std::setw(10) << node.Q_spec()/1e6 << " Mvar";
        
        if (node.type() == NodeType::PV) {
            std::cout << " | V_set=" << std::setw(8) << node.V_set()/1e3 << " kV"
                      << " | Q_min=" << std::setw(10) << node.Q_min()/1e6 << " Mvar"
                      << " | Q_max=" << std::setw(10) << node.Q_max()/1e6 << " Mvar";
        }
        std::cout << "\n";
    }
    SUCCEED();
}

TEST_F(CsvParserTest, PrintAllLines) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    parser.parseFiles(QString::fromStdString(nodes_path_), 
                      QString::fromStdString(branches_path_), 
                      system);

    std::cout << "\n========== ALL LINES ==========\n";
    std::cout << std::fixed << std::setprecision(2);
    
    for (const auto& line : system.getLines()) {
        std::cout << "Line " << std::setw(3) << line.id()
                  << " | " << std::setw(5) << line.from() 
                  << " -> " << std::setw(5) << line.to()
                  << " | " << (line.istransformer() ? "Trafo" : "Line ")
                  << " | R=" << std::setw(8) << line.R()
                  << " | X=" << std::setw(8) << line.X()
                  << " | G=" << std::setw(8) << line.Y().real()
                  << " | B=" << std::setw(8) << line.Y().imag()
                  << " | ktr=" << std::setw(8) << line.k_t().real()
                  << "\n";
    }
    SUCCEED();
}

TEST_F(CsvParserTest, EmptyFilePaths) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    
    bool result = parser.parseFiles("", QString::fromStdString(branches_path_), system);
    EXPECT_FALSE(result);
    
    result = parser.parseFiles(QString::fromStdString(nodes_path_), "", system);
    EXPECT_FALSE(result);
}

TEST_F(CsvParserTest, NonExistentFile) {
    PowerSystem system(1000.0e6, 330.0e3);
    CsvParser parser;
    
    bool result = parser.parseFiles("nonexistent.csv", QString::fromStdString(branches_path_), system);
    EXPECT_FALSE(result);
}