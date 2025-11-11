cmake_minimum_required(VERSION 3.12)
project(certificate_generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(certificate_tool
    src/main.cpp
    src/CertificateGenerator.cpp
)

target_include_directories(certificate_tool PRIVATE include)
