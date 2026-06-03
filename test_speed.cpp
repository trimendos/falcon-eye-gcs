#include <iostream>
#include <chrono>

int main() {
    // Фіксуємо час старту
    auto start = std::chrono::high_resolution_clock::now();

    long long total = 0;
    for (int i = 0; i < 100000000; ++i) { // 100 мільйонів ітерацій
        total += i;
    }

    // Фіксуємо час фінішу
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "C++ Result: " << total << std::endl;
    std::cout << "C++ Time: " << elapsed.count() << " seconds" << std::endl;

    return 0;
}