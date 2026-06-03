# falcon_eye/drone_sim.py
import socket
import time
import math

# Налаштування UDP з'єднання
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Початкові змінні польоту
battery = 100
altitude = 0
MAX_ALTITUDE = 150
sim_time = 0.0  # Час для симуляції нахилів

print(f"Запуск симуляції польоту. Відправка UDP на {UDP_IP}:{UDP_PORT}...")

# Цикл польоту, поки є заряд батареї
while battery > 0:
    # 1. Зменшуємо батарею на 1%
    battery -= 1
    
    # 2. Збільшуємо висоту на 5 метрів, але не вище MAX_ALTITUDE
    if altitude < MAX_ALTITUDE:
        altitude = min(altitude + 5, MAX_ALTITUDE)
    
    # Симулюємо погойдування дрона в польоті
    sim_time += 0.2
    roll = int(15 * math.sin(sim_time))    # Крен від -15 до +15 градусів
    pitch = int(10 * math.cos(sim_time))   # Тангаж від -10 до +10 градусів
    
    # 3. Формуємо рядок телеметрії
    telemetry = f"BAT:{battery};ALT:{altitude};ROLL:{roll};PITCH:{pitch}"
    
    # 4. Відправляємо по UDP (кодуємо рядок у байти)
    sock.sendto(telemetry.encode('utf-8'), (UDP_IP, UDP_PORT))
    
    # Вивід у консоль для моніторингу
    print(f"Надіслано: {telemetry}")
    
    # 5. Пауза 0.5 секунди
    time.sleep(0.5)

print("Роботу завершено: батарея розряджена.")
sock.close()