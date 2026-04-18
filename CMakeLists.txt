cmake_minimum_required(VERSION 3.28)

project(healthlink_backend)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
)

FetchContent_MakeAvailable(asio)

FetchContent_Declare(
        crow
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.2.1
)

FetchContent_MakeAvailable(crow)

find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)

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
        src/core/tree/Tree.h
        src/core/tree/TreeNode.h
        src/core/router/ApiRouter.h
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
        $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>
        $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>
)