cmake_minimum_required(VERSION 3.10)

# Указываем имя проекта и язык
project(DNSServer LANGUAGES CXX)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Добавляем путь к Boost
find_package(Boost 1.83 REQUIRED COMPONENTS system)
find_package(yaml-cpp REQUIRED)

# Проверяем, найден ли Boost
if (NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found!")
endif()
if (NOT yaml-cpp_FOUND)
    message(FATAL_ERROR "yaml-cpp not found!")
endif()

set(SOURCES_DIR src/)
set(LOGGER_DIR ${SOURCES_DIR}/logger/)
set(SERVER_DIR ${SOURCES_DIR}/server/)

# Указываем исходные файлы
set(SOURCES ${LOGGER_DIR}/logger.cc
    ${SERVER_DIR}/server.cc
    ${SOURCES_DIR}/main.cc)

# Добавляем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# Линкуем Boost
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp::yaml-cpp)

# Вывод сообщений о состоянии сборки
message(STATUS "Using Boost version: ${Boost_VERSION}")
message(STATUS "Boost include directory: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
