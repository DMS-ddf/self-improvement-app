# Указываем минимально необходимую версию CMake
cmake_minimum_required(VERSION 3.16)

# Название нашего проекта. CXX означает, что проект на C++
project(SelfImprovementApp LANGUAGES CXX)

# --- НАША ПРАВКА ---
# Указываем CMake, где искать установленную библиотеку Qt.
# Замените '6.10.0' на вашу точную версию, если она отличается.
set(CMAKE_PREFIX_PATH "Z:/Qt/6.10.0/mingw_64")
# --- КОНЕЦ ПРАВКИ ---

# Устанавливаем стандарт языка C++ (17 - современный и хороший выбор)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Это три "магические" строчки для работы с Qt. Они автоматически
# вызывают специальные инструменты Qt для обработки файлов.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Самая важная часть: ищем на компьютере установленную библиотеку Qt6
# и подключаем ее компонент Widgets, который отвечает за все элементы интерфейса.
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Говорим CMake, что нужно создать исполняемый файл (программу)
# с именем SelfImprovementApp из исходного файла main.cpp
add_executable(SelfImprovementApp
    main.cpp
)

# "Прилинковываем" (присоединяем) библиотеку Qt Widgets к нашей программе.
# Без этого компилятор не поймет, что такое QApplication или QWidget.
target_link_libraries(SelfImprovementApp PRIVATE Qt6::Widgets)