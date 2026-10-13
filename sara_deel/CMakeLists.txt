cmake_minimum_required(VERSION 3.8)
project(sara_deel)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(groep4_26_assign1_interface_pkg REQUIRED)

add_executable(groep4_26_exam_result_gen_publisher_node src/groep4_26_exam_result_gen_publisher_node.cpp)
add_executable(groep4_26_exam_result_gen_subscriber_node src/groep4_26_exam_result_gen_subscriber_node.cpp)

add_executable(groep4_26_subject_result_cal_node src/groep4_26_subject_result_cal_node.cpp)

ament_target_dependencies(groep4_26_exam_result_gen_publisher_node rclcpp groep4_26_assign1_interface_pkg)
ament_target_dependencies(groep4_26_exam_result_gen_subscriber_node rclcpp groep4_26_assign1_interface_pkg)

ament_target_dependencies(groep4_26_subject_result_cal_node rclcpp groep4_26_assign1_interface_pkg)


install(
  TARGETS
  groep4_26_exam_result_gen_publisher_node
  groep4_26_exam_result_gen_subscriber_node
  groep4_26_subject_result_cal_node
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY config launch
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
