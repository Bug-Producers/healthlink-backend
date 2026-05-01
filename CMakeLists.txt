cmake_minimum_required(VERSION 3.28)

project(healthlink_backend VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_compile_options(/bigobj)
endif()

set(VCPKG_APPLOCAL_DEPS ON)

find_package(Crow CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(healthlink_backend
        src/main.cpp
        src/utils/env_loader.h
        src/utils/StringUtils.h
        src/models/Notification.h
        src/repositories/NotificationRepository.h
        src/services/MongoConnection.h
        src/services/MongoService.h
        src/services/FirebaseAuth.h
        src/models/Appointment.h
        src/models/AppointmentStatus.h
        src/models/Department.h
        src/models/Doctor.h
        src/models/Patient.h
        src/models/PatientHistory.h
        src/models/Payment.h
        src/models/Rating.h
        src/models/TimeSlot.h
        src/models/WeeklySchedule.h
        src/core/linked_list/LinkedList.h
        src/core/linked_list/Node.h
        src/core/queue/Queue.h
        src/core/queue/QueueNode.h
        src/core/stack/Stack.h
        src/core/stack/StackNode.h
        src/repositories/DoctorRepository.h
        src/repositories/PatientRepository.h
        src/repositories/ScheduleRepository.h
        src/repositories/AppointmentRepository.h
        src/repositories/PaymentRepository.h
        src/repositories/RatingRepository.h
        src/repositories/PatientHistoryRepository.h
        src/controllers/DoctorController.h
        src/controllers/PatientController.h
        src/controllers/TestController.h
)

target_link_libraries(healthlink_backend PRIVATE
        Crow::Crow
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)

install(TARGETS healthlink_backend
        RUNTIME DESTINATION bin)

if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:healthlink_backend> DESTINATION bin)
endif()

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "HealthLink Backend")
set(CPACK_PACKAGE_VENDOR "HealthLink")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "HealthLink Backend Server (Crow + MongoDB + OpenSSL)")
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_DISPLAY_NAME "HealthLink Backend Server")
set(CPACK_NSIS_PACKAGE_NAME "HealthLink Backend")
set(CPACK_NSIS_CONTACT "admin@healthlink.local")
set(CPACK_PACKAGE_FILE_NAME "HealthLink-Backend-Installer")

include(CPack)