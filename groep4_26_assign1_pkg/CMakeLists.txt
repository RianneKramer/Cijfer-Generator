cmake_minimum_required(VERSION 3.8)
project(groep4_26_assign1_pkg)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(groep4_26_assign1_interface_pkg REQUIRED)

add_executable(result_determination
  src/result_determination.cpp
)

ament_target_dependencies(result_determination
  rclcpp
  rclcpp_action
  groep4_26_assign1_interface_pkg
)

add_executable(closing
  src/closing.cpp
)

ament_target_dependencies(closing
  rclcpp
  rclcpp_action
  groep4_26_assign1_interface_pkg
)

install(TARGETS
  result_determination
  closing
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
endif()

ament_package()