cmake_minimum_required(VERSION 3.12)
project(FalconEyeGCS VERSION 1.0 LANGUAGES CXX)

# Встановлюємо стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Кажемо CMake знайти бібліотеку Qt5 (модуль Widgets)
find_package(Qt5 COMPONENTS Widgets Network REQUIRED)

# 2. Вмикаємо автоматичну обробку файлів Qt компілятором (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 3. Створюємо виконуваний файл
add_executable(falcon_gcs main.cpp)

# 4. Лінкуємо (підключаємо) бібліотеку Qt5 до нашої програми
target_link_libraries(falcon_gcs PRIVATE Qt5::Widgets Qt5::Network)