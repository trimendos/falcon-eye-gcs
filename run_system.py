import subprocess
import time
import sys

def main():
    print("=== Запуск системи FalconEye ===")
    
    # 1. Запускаємо C++ станцію (GCS)
    try:
        gcs_process = subprocess.Popen(
            ["./build/falcon_gcs"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        print("[System] C++ GCS запущено.")
    except FileNotFoundError:
        print("[ERROR] Не знайдено файл ./build/falcon_gcs. Спочатку скомпілюй проект (F7)!")
        sys.exit(1)

    # Даємо станції 0.5 секунди, щоб відкрити сокет і підготуватися
    time.sleep(0.5)

    # 2. Запускаємо Python-симулятор дрона
    sim_process = subprocess.Popen(
        ["python3", "-u","drone_sim.py"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    print("[System] Python симулятор запущено.")
    print("=== Обидва процеси працюють. Натисни Ctrl+C для виходу ===")

    # Робимо сокети неблокуючими для читання логів
    import os
    os.set_blocking(gcs_process.stdout.fileno(), False)
    os.set_blocking(sim_process.stdout.fileno(), False)

    try:
        while True:
            # Читаємо і виводимо логи C++ станції
            gcs_line = gcs_process.stdout.readline()
            if gcs_line:
                print(f"[GCS] {gcs_line.strip()}")

            # Читаємо і виводимо логи симулятора
            sim_line = sim_process.stdout.readline()
            if sim_line:
                print(f"[SIM] {sim_line.strip()}")

            # Перевіряємо, чи не закрилися процеси самі
            if gcs_process.poll() is not None and sim_process.poll() is not None:
                break

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n[System] Отримано сигнал зупинки. Закриваю процеси...")
    finally:
        # ГАРАНТОВАНО вбиваємо обидва процеси при виході
        gcs_process.terminate()
        sim_process.terminate()
        
        # Чекаємо завершення
        gcs_process.wait()
        sim_process.wait()
        print("[System] Усі процеси успішно зупинено. Порт 5005 вільний.")

if __name__ == "__main__":
    main()