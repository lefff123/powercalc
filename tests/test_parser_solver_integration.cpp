// test_parser_solver_integration.cpp

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "csvparser.h"
#include "powersystem.h"
#include "node.h"
#include "line.h"
#include "solver.h"
#include <iostream>
#include <iomanip>

namespace fs = std::filesystem;

namespace {
// Хелпер: записывает строку в временный файл
std::string writeTempFile(const std::string& content, const std::string& name) {
    std::string path = fs::temp_directory_path() / name;
    std::ofstream f(path);
    f << content;
    f.close();
    return path;
}
}

// ==================== Схема 1: Простая двухузловая ====================
// Slack (110 кВ) -- линия 2.42+j12.1 Ом --> PQ (50 МВт, 20 Мвар)
TEST(ParserSolverIntegration, SimpleTwoBus) {
    std::string nodes_csv =
        "sel;sta;tip;ny;name;uhom;pn;qn;pg;qg;vzd;qmin;qmax;bsh;vras;delta;npa;Ysh\n"
        "0;0;0;1;Slack;110;;;;;;;;;110.0;;0;\n"
        "0;0;1;2;Load;110;50.0;20.0;;;;;;;;;110.0;0.0;0;\n";
    
    std::string branches_csv =
        "sel;sta;tip;ip;iq;np;groupid;name;r;x;g;b;ktr;n_anc;bd;pl_ip;ql_ip;na;i_max;i_zag\n"
        "0;0;0;1;2;0;0;Line;2.42;12.10;;;;0;0;;;0;;\n";
    
    // --- Подход 1: через парсер ---
    std::string np = writeTempFile(nodes_csv, "pss_s2b_n.csv");
    std::string bp = writeTempFile(branches_csv, "pss_s2b_b.csv");
    
    PowerSystem sys_csv(100e6, 110e3);
    CsvParser parser;
    ASSERT_TRUE(parser.parseFiles(QString::fromStdString(np), 
                                   QString::fromStdString(bp), sys_csv));
    Solver solver_csv(sys_csv);
    auto res_csv = solver_csv.solve();
    ASSERT_TRUE(res_csv.converged);
    
    // --- Подход 2: напрямую ---
    PowerSystem sys_dir(100e6, 110e3);
    sys_dir.addNode(Node::makeSlack(1, 110e3, 0.0, 110e3));
    sys_dir.addNode(Node::makePQ(2, 50e6, 20e6, 110e3, 0.0, 110e3));
    sys_dir.addLine(Line(1, 1, 2, 2.42, 12.1));
    
    Solver solver_dir(sys_dir);
    auto res_dir = solver_dir.solve();
    ASSERT_TRUE(res_dir.converged);
    
    // --- Сравнение ---
    EXPECT_EQ(res_csv.iterations, res_dir.iterations);
    EXPECT_NEAR(res_csv.max_mismatch, res_dir.max_mismatch, 1e-9);
    
    for (NodeId id : {1, 2}) {
        const Node& n_csv = sys_csv.getNode(id);
        const Node& n_dir = sys_dir.getNode(id);
        
        EXPECT_NEAR(n_csv.V_mag(), n_dir.V_mag(), 1.0)      // допуск 1 В
            << "V mismatch at node " << id;
        EXPECT_NEAR(n_csv.delta(), n_dir.delta(), 1e-6)
            << "delta mismatch at node " << id;
        
        std::cout << "Node " << id 
                  << ": CSV V=" << n_csv.V_mag()/1e3 << " kV, d=" << n_csv.delta()
                  << " | DIR V=" << n_dir.V_mag()/1e3 << " kV, d=" << n_dir.delta() << "\n";
    }
    
    // Slack мощность тоже должна совпадать
    auto S_csv = sys_csv.calculateSlackPower();
    auto S_dir = sys_dir.calculateSlackPower();
    EXPECT_NEAR(S_csv.real(), S_dir.real(), 1e3);
    EXPECT_NEAR(S_csv.imag(), S_dir.imag(), 1e3);
    
    fs::remove(np);
    fs::remove(bp);
}

// ==================== Схема 2: Трансформатор 110/10 кВ ====================
// Slack 110 кВ -- транс (k=11, R=0.5, X=10 Ом) --> PQ 10 кВ (20 МВт, 10 Мвар)
TEST(ParserSolverIntegration, Transformer110to10) {
    std::string nodes_csv =
        "sel;sta;tip;ny;name;uhom;pn;qn;pg;qg;vzd;qmin;qmax;bsh;vras;delta;npa;Ysh\n"
        "0;0;0;1;SlackHV;110;;;;;;;;;110.0;;0;\n"
        "0;0;1;2;LoadLV;10;20.0;10.0;;;;;;;;;10.0;0.0;0;\n";
    
    std::string branches_csv =
        "sel;sta;tip;ip;iq;np;groupid;name;r;x;g;b;ktr;n_anc;bd;pl_ip;ql_ip;na;i_max;i_zag\n"
        "0;0;1;1;2;0;0;Trafo;0.5;10.0;;;11.0;0;0;;;0;;\n";
    
    // --- Через парсер ---
    std::string np = writeTempFile(nodes_csv, "pss_tr_n.csv");
    std::string bp = writeTempFile(branches_csv, "pss_tr_b.csv");
    
    PowerSystem sys_csv(100e6, 110e3);
    CsvParser parser;
    ASSERT_TRUE(parser.parseFiles(QString::fromStdString(np), 
                                   QString::fromStdString(bp), sys_csv));
    
    // Проверяем, что базисные напряжения рассчитались правильно
    EXPECT_DOUBLE_EQ(sys_csv.V_base(1), 110e3);
    EXPECT_DOUBLE_EQ(sys_csv.V_base(2), 10e3);
    
    Solver solver_csv(sys_csv);
    auto res_csv = solver_csv.solve();
    ASSERT_TRUE(res_csv.converged);
    
    // --- Напрямую ---
    PowerSystem sys_dir(100e6, 110e3);
    sys_dir.addNode(Node::makeSlack(1, 110e3, 0.0, 110e3));
    sys_dir.addNode(Node::makePQ(2, 20e6, 10e6, 10e3, 0.0, 10e3));
    sys_dir.addLine(Line(1, 1, 2, 0.5, 10.0, std::complex<double>(11.0, 0.0)));
    
    Solver solver_dir(sys_dir);
    auto res_dir = solver_dir.solve();
    ASSERT_TRUE(res_dir.converged);
    
    // --- Сравнение ---
    EXPECT_EQ(res_csv.iterations, res_dir.iterations);
    
    for (NodeId id : {1, 2}) {
        const Node& n_csv = sys_csv.getNode(id);
        const Node& n_dir = sys_dir.getNode(id);
        
        EXPECT_NEAR(n_csv.V_mag(), n_dir.V_mag(), 1.0)
            << "V mismatch at node " << id;
        EXPECT_NEAR(n_csv.delta(), n_dir.delta(), 1e-6)
            << "delta mismatch at node " << id;
        
        std::cout << "Node " << id 
                  << ": CSV V=" << n_csv.V_mag()/1e3 << " kV, d=" << n_csv.delta()
                  << " | DIR V=" << n_dir.V_mag()/1e3 << " kV, d=" << n_dir.delta() << "\n";
    }
    
    // Slack мощность
    auto S_csv = sys_csv.calculateSlackPower();
    auto S_dir = sys_dir.calculateSlackPower();
    EXPECT_NEAR(S_csv.real(), S_dir.real(), 1e3);
    EXPECT_NEAR(S_csv.imag(), S_dir.imag(), 1e3);
    
    fs::remove(np);
    fs::remove(bp);
}

// ==================== Схема 3: Трёхузловая с шунтом ====================
// Slack (110) --- линия с шунтом (Y=j0.001 См) --> PQ 2 (30 МВт, 10 Мвар)
// PQ 2 --- линия --> PQ 3 (20 МВт, 8 Мвар)
TEST(ParserSolverIntegration, ThreeBusWithShunt) {
    std::string nodes_csv =
        "sel;sta;tip;ny;name;uhom;pn;qn;pg;qg;vzd;qmin;qmax;bsh;vras;delta;npa;Ysh\n"
        "0;0;0;1;Slack;110;;;;;;;;;110.0;;0;\n"
        "0;0;1;2;Load2;110;30.0;10.0;;;;;;;;;110.0;0.0;0;\n"
        "0;0;1;3;Load3;110;20.0;8.0;;;;;;;;;110.0;0.0;0;\n";
    
    std::string branches_csv =
        "sel;sta;tip;ip;iq;np;groupid;name;r;x;g;b;ktr;n_anc;bd;pl_ip;ql_ip;na;i_max;i_zag\n"
        "0;0;0;1;2;0;0;Line1;5.0;25.0;;0.001;;0;0;;;0;;\n"
        "0;0;0;2;3;0;0;Line2;4.0;20.0;;;;0;0;;;0;;\n";
    
    // --- Через парсер ---
    std::string np = writeTempFile(nodes_csv, "pss_3b_n.csv");
    std::string bp = writeTempFile(branches_csv, "pss_3b_b.csv");
    
    PowerSystem sys_csv(100e6, 110e3);
    CsvParser parser;
    ASSERT_TRUE(parser.parseFiles(QString::fromStdString(np), 
                                   QString::fromStdString(bp), sys_csv));
    
    // Проверяем, что шунт попал в линию
    EXPECT_NEAR(sys_csv.getLines()[0].Y().imag(), 0.001, 1e-9);
    
    Solver solver_csv(sys_csv);
    auto res_csv = solver_csv.solve();
    ASSERT_TRUE(res_csv.converged);
    
    // --- Напрямую ---
    PowerSystem sys_dir(100e6, 110e3);
    sys_dir.addNode(Node::makeSlack(1, 110e3, 0.0, 110e3));
    sys_dir.addNode(Node::makePQ(2, 30e6, 10e6, 110e3, 0.0, 110e3));
    sys_dir.addNode(Node::makePQ(3, 20e6, 8e6, 110e3, 0.0, 110e3));
    sys_dir.addLine(Line(1, 1, 2, 5.0, 25.0, std::complex<double>(1.0, 0.0), 
                         std::complex<double>(0.0, 0.001)));
    sys_dir.addLine(Line(2, 2, 3, 4.0, 20.0));
    
    Solver solver_dir(sys_dir);
    auto res_dir = solver_dir.solve();
    ASSERT_TRUE(res_dir.converged);
    
    // --- Сравнение ---
    EXPECT_EQ(res_csv.iterations, res_dir.iterations);
    
    for (NodeId id : {1, 2, 3}) {
        const Node& n_csv = sys_csv.getNode(id);
        const Node& n_dir = sys_dir.getNode(id);
        
        EXPECT_NEAR(n_csv.V_mag(), n_dir.V_mag(), 1.0)
            << "V mismatch at node " << id;
        EXPECT_NEAR(n_csv.delta(), n_dir.delta(), 1e-6)
            << "delta mismatch at node " << id;
        
        std::cout << "Node " << id 
                  << ": CSV V=" << n_csv.V_mag()/1e3 << " kV, d=" << n_csv.delta()
                  << " | DIR V=" << n_dir.V_mag()/1e3 << " kV, d=" << n_dir.delta() << "\n";
    }
    
    // Проверка баланса мощностей в обоих случаях
    auto S_csv = sys_csv.calculateSlackPower();
    auto flows_csv = sys_csv.calculateLineFlows();
    double P_load_csv = 30e6 + 20e6;
    double Q_load_csv = 10e6 + 8e6;
    std::complex<double> total_loss_csv(0, 0);
    for (const auto& f : flows_csv) total_loss_csv += f.S_loss;
    
    EXPECT_NEAR(S_csv.real(), P_load_csv + total_loss_csv.real(), 1e3);
    EXPECT_NEAR(S_csv.imag(), Q_load_csv + total_loss_csv.imag(), 1e3);
    
    fs::remove(np);
    fs::remove(bp);
}