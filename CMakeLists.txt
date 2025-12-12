cmake_minimum_required(VERSION 3.16) # Минимальная версия CMake, нужна для поддержки современных функций Qt6

project(SelfImprovementApp LANGUAGES CXX) # Имя проекта и используемый язык (C++)

# Обычные флаги: заставляем компилятор использовать UTF-8 для строк
if(CMAKE_COMPILER_ID STREQUAL "GNU" OR CMAKE_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-finput-charset=UTF-8)
    add_compile_options(-fexec-charset=UTF-8)
elseif(CMAKE_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/utf-8)
endif()

# Путь к установленной Qt (отредактируйте при необходимости)
set(CMAKE_PREFIX_PATH "Z:/Qt/6.10.0/mingw_64")

# Гарантируем, куда кладётся исполняемый файл (избегаем пустого CMAKE_RUNTIME_OUTPUT_DIRECTORY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Политика для add_custom_command с WORKING_DIRECTORY
cmake_policy(SET CMP0175 NEW)

# Включаем современный стандарт C++17 (нужен для многих фишек Qt6)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку Qt (moc, rcc, uic)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ищем и подключаем нужные модули Qt: Core (база), Widgets (интерфейс), Sql (базы данных)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)

# Исполняемый файл: перечисляем только .cpp
add_executable(SelfImprovementApp WIN32
    main.cpp
    MainWindow.cpp
    AddTaskDialog.cpp
)

# Автоматическое развертывание: используем windeployqt для копирования DLL
find_program(
    WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt
    PATHS "Z:/Qt/6.10.0/mingw_64/bin"
    NO_DEFAULT_PATH # Ищем ТОЛЬКО по указанному пути, чтобы не подцепить не ту версию
)

# Если не нашли — показать предупреждение, но не падать (опционально можно сменить на FATAL)
if(NOT WINDEPLOYQT_EXECUTABLE)
    message(WARNING "windeployqt.exe not found in Z:/Qt/6.10.0/mingw_64/bin")
endif()

message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")

# Добавляем команду, которая запускается ПОСЛЕ (POST_BUILD) создания .exe файла.
# Она запускает windeployqt в папке с .exe, и та сама подтягивает все нужные библиотеки.
add_custom_command(
    TARGET SelfImprovementApp
    POST_BUILD
    COMMAND ${WINDEPLOYQT_EXECUTABLE}
    ARGS --verbose 0 --no-opengl-sw --no-translations .
    WORKING_DIRECTORY $<TARGET_FILE_DIR:SelfImprovementApp> # Выполнять в папке, куда собрался .exe
    COMMENT "Running windeployqt to copy necessary DLLs..."
)

# Линковка: связываем наш исполняемый файл с найденными библиотеками Qt.
target_link_libraries(SelfImprovementApp PRIVATE Qt6::Core Qt6::Widgets Qt6::Sql)