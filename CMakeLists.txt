cmake_minimum_required(VERSION 3.28)
project(healthlink_backend)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(crow CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)

add_executable(healthlink_backend
        src/main.cpp
        src/env_loader.h
        src/env_loader.h
        src/env_loader.h
        src/models/Appointment.h
)


target_link_libraries(healthlink_backend PRIVATE
        Crow::Crow
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)