cmake_minimum_required(VERSION 3.16) # Минимальная версия CMake, нужна для поддержки современных функций Qt6

project(SelfImprovementApp LANGUAGES CXX) # Имя проекта и используемый язык (C++)

# --- Глобальный фикс кодировки ---
# Windows по умолчанию любит старые кодировки.
# Этими флагами мы заставляем компилятор (GCC/Clang) использовать UTF-8
# как для исходников, так и для строк внутри скомпилированной программы.
# Это нужно, чтобы русский текст в приложении отображался корректно.
if(CMAKE_COMPILER_ID STREQUAL "GNU" OR CMAKE_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-finput-charset=UTF-8)
    add_compile_options(-fexec-charset=UTF-8)
elseif(CMAKE_COMPILER_ID STREQUAL "MSVC")
    # Для Visual Studio свой флаг
    add_compile_options(/utf-8)
endif()
# --- Конец фикса ---

# Путь к установленной библиотеке Qt.
# ВАЖНО: Если перенесем проект на другой комп, этот путь придется менять!
set(CMAKE_PREFIX_PATH "Z:/Qt/6.10.0/mingw_64")

# Включаем современный стандарт C++17 (нужен для многих фишек Qt6)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем магию Qt:
# AUTOMOC - автоматически обрабатывает файлы с Q_OBJECT
# AUTORCC - автоматически компилирует файлы ресурсов (.qrc)
# AUTOUIC - автоматически создает код из файлов форм (.ui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ищем и подключаем нужные модули Qt: Core (база), Widgets (интерфейс), Sql (базы данных)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)

# Создаем исполняемый файл. Перечисляем ВСЕ .cpp и .h файлы проекта.
add_executable(SelfImprovementApp
    main.cpp
    MainWindow.h
    MainWindow.cpp
    AddTaskDialog.h
    AddTaskDialog.cpp
)

# --- Автоматическое развертывание (Deployment) ---
# Ищем утилиту windeployqt.exe в папке с Qt.
# Она нужна, чтобы автоматически копировать .dll файлы рядом с нашим .exe.
find_program(
    WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt
    PATHS "Z:/Qt/6.10.0/mingw_64/bin"
    NO_DEFAULT_PATH # Ищем ТОЛЬКО по указанному пути, чтобы не подцепить не ту версию
)

# Если не нашли — ругаемся и останавливаем сборку.
if(NOT WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "windeployqt.exe not found! Check the path 'Z:/Qt/6.10.0/mingw_64/bin'")
endif()

message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")

# Добавляем команду, которая запускается ПОСЛЕ (POST_BUILD) создания .exe файла.
# Она запускает windeployqt в папке с .exe, и та сама подтягивает все нужные библиотеки.
add_custom_command(
    TARGET SelfImprovementApp
    POST_BUILD
    COMMAND ${WINDEPLOYQT_EXECUTABLE}
    ARGS --verbose 0 --no-opengl-sw --no-translations .
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} # Выполнять в папке, куда собрался .exe (например, build/)
    COMMENT "Running windeployqt to copy necessary DLLs..."
)

# Линковка: связываем наш исполняемый файл с найденными библиотеками Qt.
target_link_libraries(SelfImprovementApp PRIVATE Qt6::Core Qt6::Widgets Qt6::Sql)