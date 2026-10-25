cmake_minimum_required(VERSION 3.8)
project(groep4_26_assign1_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(groep4_26_assign1_interface_pkg REQUIRED)

# DBManager
find_package(PostgreSQL REQUIRED)

add_executable(dbmanager
        src/DatabaseManager/DBManager.cpp
        include/groep4_26_assign1_pkg/DatabaseManager/DBManager.hpp
        src/DatabaseManager/Database.cpp
        include/groep4_26_assign1_pkg/DatabaseManager/Database.hpp
        include/groep4_26_assign1_pkg/DatabaseManager/SubjectStudent.hpp
        include/groep4_26_assign1_pkg/DatabaseManager/ExamResult.hpp
        include/groep4_26_assign1_pkg/DatabaseManager/StudentResult.hpp
)
ament_target_dependencies(dbmanager rclcpp PostgreSQL groep4_26_assign1_interface_pkg)
install(TARGETS dbmanager DESTINATION lib/${PROJECT_NAME})

# Exam result generator
add_executable(groep4_26_exam_result_gen_node src/ExamResultGenerator/groep4_26_exam_result_gen_node.cpp)
ament_target_dependencies(groep4_26_exam_result_gen_node rclcpp groep4_26_assign1_interface_pkg)

# Subject result calculator
add_executable(groep4_26_subject_result_cal_node src/SubjectResultCalculator/groep4_26_subject_result_cal_node.cpp)
ament_target_dependencies(groep4_26_subject_result_cal_node rclcpp groep4_26_assign1_interface_pkg)

# Result determination and year closing
add_executable(result_determination src/Result_Determination/result_determination.cpp)
ament_target_dependencies(result_determination rclcpp rclcpp_action groep4_26_assign1_interface_pkg)

add_executable(closing src/Closing/closing.cpp)
ament_target_dependencies(closing rclcpp rclcpp_action groep4_26_assign1_interface_pkg)

install(TARGETS
  groep4_26_exam_result_gen_node
  groep4_26_subject_result_cal_node
  result_determination
  closing
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch parameters
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
