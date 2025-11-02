cmake_minimum_required(VERSION 3.16) # min CMake version

project(SelfImprovementApp LANGUAGES CXX) # project name (CXX => C++)

set(CMAKE_PREFIX_PATH "Z:/Qt/6.10.0/mingw_64") # Qt library path for CMake

set(CMAKE_CXX_STANDARD 17) # standart language C++ = 17
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(CMAKE_AUTOMOC ON) # AUTO special instruments Qt for file processing
set(CMAKE_AUTORCC ON) 
set(CMAKE_AUTOUIC ON) 

find_package(Qt6 REQUIRED COMPONENTS Core Widgets) # find Qt and connect components

add_executable(SelfImprovementApp main.cpp)
# tell CMake to create executable file (named SelfImprovementApp) from the source main.cpp

target_link_libraries(SelfImprovementApp PRIVATE Qt6::Widgets) # link Qt lib