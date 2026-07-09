import pandapower as pp
import numpy as np
'''
Проверка на либе, которая считает точно (точно : ) )
Растрвина под рукой не было, считаю тут
'''

def run_test(test_name, buses, lines, loads, slack_bus_id, expected_convergence=True):
    print(f"\n{'='*60}")
    print(f" ТЕСТ: {test_name}")
    print(f"{'='*60}")
    
    # Создаем сеть с базисом 100 МВА
    net = pp.create_empty_network(sn_mva=100)
    
    # 1. Создаем узлы (шины)
    bus_map = {}
    for bus_id, vn_kv in buses.items():
        idx = pp.create_bus(net, vn_kv=vn_kv, name=f"Узел {bus_id}")
        bus_map[bus_id] = idx
        
    # 2. Создаем Slack-узел (внешняя сеть)
    slack_idx = bus_map[slack_bus_id]
    pp.create_ext_grid(net, slack_idx, vm_pu=1.0, va_degree=0.0)
    
    # 3. Создаем нагрузки (PQ-узлы)
    for load_id, (bus_id, p_mw, q_mvar) in loads.items():
        bus_idx = bus_map[bus_id]
        pp.create_load(net, bus_idx, p_mw=p_mw, q_mvar=q_mvar, name=f"Нагрузка {load_id}")
        
    # 4. Создаем линии
    for line_id, (from_id, to_id, r_ohm, x_ohm) in lines.items():
        from_idx = bus_map[from_id]
        to_idx = bus_map[to_id]
        pp.create_line_from_parameters(
            net, from_bus=from_idx, to_bus=to_idx, length_km=1.0,
            r_ohm_per_km=r_ohm, x_ohm_per_km=x_ohm, c_nf_per_km=0.0,
            max_i_ka=10.0, name=f"Линия {line_id}"
        )
        
    # 5. Запускаем расчет установившегося режима
    try:
        pp.runpp(net)
        converged = True
    except pp.auxiliary.LoadflowNotConverged:
        converged = False
        print(f"\n⚠️  РАСЧЕТ НЕ СОШЕЛСЯ (ожидалось: {expected_convergence})")
        
    if converged != expected_convergence:
        print(f"⚠️  ВНИМАНИЕ: сходимость={converged}, но ожидалось {expected_convergence}")
    
    if not converged:
        return
        
    # 6. Выводим результаты
    print("\n[1] Напряжения в узлах:")
    for bus_id, idx in bus_map.items():
        vm_pu = net.res_bus.at[idx, 'vm_pu']
        va_deg = net.res_bus.at[idx, 'va_degree']
        vm_kv = vm_pu * 110.0
        va_rad = np.radians(va_deg)
        print(f"  Узел {bus_id}: V = {vm_kv:.4f} кВ ({vm_pu:.6f} p.u.) | Angle = {va_deg:.4f}° ({va_rad:.6f} рад)")
        
    print("\n[2] Мощность в Slack-узле (генерация):")
    p_slack = net.res_ext_grid.at[0, 'p_mw']
    q_slack = net.res_ext_grid.at[0, 'q_mvar']
    print(f"  P = {p_slack:.4f} МВт, Q = {q_slack:.4f} Мвар")
    
    print("\n[3] Потери мощности в линиях:")
    for line_id in lines.keys():
        line_idx = net.line[net.line['name'] == f"Линия {line_id}"].index[0]
        pl_mw = net.res_line.at[line_idx, 'pl_mw']
        ql_mvar = net.res_line.at[line_idx, 'ql_mvar']
        print(f"  {line_id}: P_потерь = {pl_mw:.4f} МВт, Q_потерь = {ql_mvar:.4f} Мвар")

# --- ЗАПУСК ТЕСТОВ ---

# Тест 1: SimpleTwoBusSystem (с уменьшенным сопротивлением)
buses1 = {1: 110, 2: 110}
lines1 = {'1-2': (1, 2, 2.42, 12.1)}  # Уменьшено в 5 раз
loads1 = {2: (2, 50, 20)}
run_test("1. SimpleTwoBusSystem", buses1, lines1, loads1, slack_bus_id=1, expected_convergence=True)

# Тест 2: ThreeBusSystem (без изменений)
buses2 = {1: 110, 2: 110, 3: 110}
lines2 = {'1-2': (1, 2, 10, 50), '2-3': (2, 3, 8, 40), '1-3': (1, 3, 12, 60)}
loads2 = {2: (2, 30, 10), 3: (3, 40, 15)}
run_test("2. ThreeBusSystem", buses2, lines2, loads2, slack_bus_id=1, expected_convergence=True)

# Тест 3: NoConvergence (огромная нагрузка и сопротивление)
buses3 = {1: 110, 2: 110}
lines3 = {'1-2': (1, 2, 100, 500)}
loads3 = {2: (2, 500, 200)}
run_test("3. NoConvergence", buses3, lines3, loads3, slack_bus_id=1, expected_convergence=False)

# Тест 4: DifferentLoadLevels (с уменьшенным сопротивлением)
print("\n" + "="*60)
print(" ТЕСТ 4: DifferentLoadLevels (разные уровни нагрузки)")
print("="*60)
for load_mw in [10, 30, 50, 70]:
    buses = {1: 110, 2: 110}
    lines = {'1-2': (1, 2, 2.42, 12.1)}  # Уменьшено в 5 раз
    loads = {2: (2, load_mw, load_mw * 0.4)}
    run_test(f"4.{load_mw}. Load = {load_mw} МВт", buses, lines, loads, slack_bus_id=1, expected_convergence=True)

# Тест 5: RingNetwork (без изменений)
buses5 = {1: 110, 2: 110, 3: 110}
lines5 = {'1-2': (1, 2, 10, 50), '2-3': (2, 3, 8, 40), '3-1': (3, 1, 12, 60)}
loads5 = {2: (2, 40, 15), 3: (3, 30, 10)}
run_test("5. RingNetwork", buses5, lines5, loads5, slack_bus_id=1, expected_convergence=True)

# Тест 6: MultiplePQNodes (с уменьшенными сопротивлениями)
buses6 = {1: 110, 2: 110, 3: 110, 4: 110, 5: 110}
lines6 = {
    '1-2': (1, 2, 1.6, 8.0),   # Уменьшено в 5 раз
    '2-3': (2, 3, 2.0, 10.0),  # Уменьшено в 5 раз
    '3-4': (3, 4, 2.4, 12.0),  # Уменьшено в 5 раз
    '4-5': (4, 5, 3.0, 15.0)   # Уменьшено в 5 раз
}
loads6 = {2: (2, 20, 8), 3: (3, 25, 10), 4: (4, 30, 12), 5: (5, 15, 6)}
run_test("6. MultiplePQNodes", buses6, lines6, loads6, slack_bus_id=1, expected_convergence=True)

print("\n" + "="*60)
print(" ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ")
print("="*60)