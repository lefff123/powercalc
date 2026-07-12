import pandapower as pp
import numpy as np
import cmath

# ==============================================================================
# Вспомогательные функции
# ==============================================================================

def print_results(net, test_name):
    """Вывод результатов в формате, идентичном C++ printPowerFlowResults"""
    print(f"\n{'='*60}")
    print(f" ТЕСТ: {test_name}")
    print(f"{'='*60}")
    
    print("\n[1] Напряжения в узлах:")
    for idx, row in net.res_bus.iterrows():
        bus_name = net.bus.at[idx, 'name']
        vn_kv = net.bus.at[idx, 'vn_kv']
        vm_pu = row['vm_pu']
        va_deg = row['va_degree']
        vm_kv = vm_pu * vn_kv
        va_rad = np.radians(va_deg)
        print(f"  {bus_name}: V = {vm_kv:.4f} кВ ({vm_pu:.6f} p.u.) | Angle = {va_deg:.4f}° ({va_rad:.6f} рад)")
        
    if not net.res_ext_grid.empty:
        print("\n[2] Мощность в Slack-узле (генерация):")
        p = net.res_ext_grid.at[0, 'p_mw']
        q = net.res_ext_grid.at[0, 'q_mvar']
        print(f"  P = {p:.4f} МВт, Q = {q:.4f} Мвар")
        
    print("\n[3] Потери мощности в линиях:")
    for idx, row in net.res_line.iterrows():
        name = net.line.at[idx, 'name']
        pl = row['pl_mw']
        ql = row['ql_mvar']
        print(f"  {name}: P_потерь = {pl:.4f} МВт, Q_потерь = {ql:.4f} Мвар")

def add_complex_transformer(net, from_bus, to_bus, r_ohm, x_ohm, k_t_complex, vn_from_kv, vn_to_kv, sn_mva=100.0):
    """
    Создание трансформатора из комплексных параметров (аналог C++ Line с k_t).
    k_t_complex - комплексный коэффициент трансформации (модуль и угол).
    """
    mag = abs(k_t_complex)
    angle_deg = np.degrees(np.angle(k_t_complex))
    
    # Базисное сопротивление на стороне ВН
    z_base_from = vn_from_kv**2 / sn_mva
    r_pu = r_ohm / z_base_from
    x_pu = x_ohm / z_base_from
    z_pu = np.sqrt(r_pu**2 + x_pu**2)
    
    vkr_percent = r_pu * 100.0
    vk_percent = z_pu * 100.0
    
    # Расчет tap_step_percent относительно номинального соотношения vn_from/vn_to
    k_nom = vn_from_kv / vn_to_kv
    tap_step_percent = (mag / k_nom - 1.0) * 100.0
    tap_pos = 1 if abs(tap_step_percent) > 1e-6 else 0
    
    if abs(tap_step_percent) < 1e-6:
        tap_step_percent = 0.0

    pp.create_transformer_from_parameters(
        net, from_bus=from_bus, to_bus=to_bus,
        sn_mva=sn_mva, vn_from_kv=vn_from_kv, vn_to_kv=vn_to_kv,
        vk_percent=vk_percent, vkr_percent=vkr_percent,
        pfe_kw=0.0, i0_percent=0.0,
        shift_degree=angle_deg,
        tap_step_percent=tap_step_percent, tap_pos=tap_pos,
        tap_side='hv', 
        name=f"Line {from_bus}-{to_bus}"
    )

# ==============================================================================
# ТЕСТЫ
# ==============================================================================

def test_simple_two_bus():
    """Аналог TEST(Solver, SimpleTwoBusSystem)"""
    net = pp.create_empty_network(sn_mva=100)
    b1 = pp.create_bus(net, vn_kv=110, name="Узел 1")
    b2 = pp.create_bus(net, vn_kv=110, name="Узел 2")
    
    pp.create_ext_grid(net, b1, vm_pu=1.0, va_degree=0.0)
    pp.create_load(net, b2, p_mw=50, q_mvar=20)
    
    # R=2.42, X=12.1 (в C++ уменьшено в 5 раз)
    pp.create_line_from_parameters(net, b1, b2, length_km=1.0, 
                                   r_ohm_per_km=2.42, x_ohm_per_km=12.1, 
                                   c_nf_per_km=0, max_i_ka=10, name="Line 1-2")
    
    pp.runpp(net)
    print_results(net, "Solver.SimpleTwoBusSystem")

def test_complex_transformer():
    """Аналог TEST(Transformer, ComplexTapWithPhaseShift)"""
    net = pp.create_empty_network(sn_mva=100)
    b1 = pp.create_bus(net, vn_kv=110, name="Узел 1")
    b2 = pp.create_bus(net, vn_kv=110, name="Узел 2")
    
    pp.create_ext_grid(net, b1, vm_pu=1.0, va_degree=0.0)
    pp.create_load(net, b2, p_mw=30, q_mvar=15)
    
    # k_t = 1.05 * e^(j*10°)
    angle_rad = 10.0 * np.pi / 180.0
    k_t = 1.05 * complex(np.cos(angle_rad), np.sin(angle_rad))
    
    add_complex_transformer(net, b1, b2, r_ohm=5.0, x_ohm=25.0, 
                            k_t_complex=k_t, vn_from_kv=110, vn_to_kv=110)
                            
    pp.runpp(net)
    print_results(net, "Transformer.ComplexTapWithPhaseShift")

def test_pv_node_with_limits():
    """Аналог TEST(PVLimits, PVReachesQmax)"""
    net = pp.create_empty_network(sn_mva=100)
    b1 = pp.create_bus(net, vn_kv=110, name="Узел 1")
    b2 = pp.create_bus(net, vn_kv=110, name="Узел 2")
    b3 = pp.create_bus(net, vn_kv=110, name="Узел 3")
    
    pp.create_ext_grid(net, b1, vm_pu=1.0, va_degree=0.0)
    
    # PV-узел: P=50 МВт, V=108 кВ (108/110 p.u.), Q_min=-10, Q_max=10
    pp.create_gen(net, b2, p_mw=50, vm_pu=108/110, 
                  min_q_mvar=-10, max_q_mvar=10, name="Gen 2")
                  
    pp.create_load(net, b3, p_mw=30, q_mvar=20)
    
    pp.create_line_from_parameters(net, b1, b2, length_km=1.0, r_ohm_per_km=5.0, x_ohm_per_km=25.0, c_nf_per_km=0, max_i_ka=10, name="Line 1-2")
    pp.create_line_from_parameters(net, b2, b3, length_km=1.0, r_ohm_per_km=4.0, x_ohm_per_km=20.0, c_nf_per_km=0, max_i_ka=10, name="Line 2-3")
    
    # enforce_q_lims=True заставляет pandapower конвертировать PV в PQ при выходе за лимиты
    pp.runpp(net, enforce_q_lims=True) 
    
    print_results(net, "PVLimits.PVReachesQmax")
    # Проверка: генератор должен выдать ровно 10 Мвар (Q_max)
    q_gen = net.res_gen.at[0, 'q_mvar']
    print(f"  [Проверка] Q генератора = {q_gen:.4f} Мвар (ожидалось ~10.0)")

def test_line_with_shunt():
    """Аналог TEST(Shunt, LineWithShuntAndLoad)"""
    net = pp.create_empty_network(sn_mva=100)
    b1 = pp.create_bus(net, vn_kv=110, name="Узел 1")
    b2 = pp.create_bus(net, vn_kv=110, name="Узел 2")
    
    pp.create_ext_grid(net, b1, vm_pu=1.0, va_degree=0.0)
    pp.create_load(net, b2, p_mw=50, q_mvar=20)
    
    # Y_shunt = 0 + j0.0005 См
    # В pandapower для линии это c_nf_per_km и g_us_per_km
    # B = 0.0005, C = B / (2 * pi * 50)
    b_val = 0.0005
    c_farad = b_val / (2 * np.pi * 50)
    c_nf = c_farad * 1e9
    
    pp.create_line_from_parameters(net, b1, b2, length_km=1.0, 
                                   r_ohm_per_km=2.42, x_ohm_per_km=12.1, 
                                   c_nf_per_km=c_nf, g_us_per_km=0.0, 
                                   max_i_ka=10, name="Line 1-2")
                                   
    pp.runpp(net)
    print_results(net, "Shunt.LineWithShuntAndLoad")

def test_reconfiguration():
    """Аналог TEST(Reconfiguration, DisconnectOneOfParallelLines)"""
    net = pp.create_empty_network(sn_mva=100)
    b1 = pp.create_bus(net, vn_kv=110, name="Узел 1")
    b2 = pp.create_bus(net, vn_kv=110, name="Узел 2")
    
    pp.create_ext_grid(net, b1, vm_pu=1.0, va_degree=0.0)
    pp.create_load(net, b2, p_mw=50, q_mvar=20)
    
    # Две параллельные линии
    l1 = pp.create_line_from_parameters(net, b1, b2, length_km=1.0, r_ohm_per_km=2.42, x_ohm_per_km=12.1, c_nf_per_km=0, max_i_ka=10, name="Line 1")
    l2 = pp.create_line_from_parameters(net, b1, b2, length_km=1.0, r_ohm_per_km=2.42, x_ohm_per_km=12.1, c_nf_per_km=0, max_i_ka=10, name="Line 2")
    
    print("--- Считаем с двумя линиями ---")
    pp.runpp(net)
    print_results(net, "Reconfig.BeforeDisconnect")
    v_before = net.res_bus.at[b2, 'vm_pu']
    
    # Отключаем вторую линию (аналог sys.disconnectLine(2))
    net.line.loc[l2, 'in_service'] = False
    
    print("\n--- Считаем с одной линией ---")
    pp.runpp(net)
    print_results(net, "Reconfig.AfterDisconnect")
    v_after = net.res_bus.at[b2, 'vm_pu']
    
    print(f"  [Проверка] Напряжение упало: {v_before:.4f} -> {v_after:.4f} (Ожидалось: True)")

if __name__ == "__main__":
    test_simple_two_bus()
    test_complex_transformer()
    test_pv_node_with_limits()
    test_line_with_shunt()
    test_reconfiguration()
    
    print("\n" + "="*60)
    print(" ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ")
    print("="*60)