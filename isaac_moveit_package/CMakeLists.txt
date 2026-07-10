cmake_minimum_required(VERSION 3.5)
project(isaac_moveit_package)

# =========================
# CORE DEPENDENCIES
# =========================
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)

# --- Added for cube-yaw orientation math in cube_gripper_data ---
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

# =========================
# ROS2 INTERFACES (SERVICES)
# =========================
find_package(rosidl_default_generators REQUIRED)
find_package(rosidl_default_runtime REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/CubePose.srv"
)

ament_export_dependencies(rosidl_default_runtime)

# =========================
# EXECUTABLES
# =========================

add_executable(motion_node src/motion.cpp)
ament_target_dependencies(motion_node
  rclcpp
  moveit_ros_planning_interface
)

install(TARGETS motion_node
  DESTINATION lib/${PROJECT_NAME}
)


# -------------------------

add_executable(cube_gripper_node src/cube_gripper.cpp)
ament_target_dependencies(cube_gripper_node
  rclcpp
  moveit_ros_planning_interface
)

install(TARGETS cube_gripper_node
  DESTINATION lib/${PROJECT_NAME}
)


# -------------------------

add_executable(gripper_node src/gripper.cpp)
ament_target_dependencies(gripper_node
  rclcpp
  moveit_ros_planning_interface
)

install(TARGETS gripper_node
  DESTINATION lib/${PROJECT_NAME}
)


# -------------------------
# SERVICE CLIENT NODE
# -------------------------

add_executable(cube_client_node src/cube_client.cpp)
ament_target_dependencies(cube_client_node
  rclcpp
)

install(TARGETS cube_client_node
  DESTINATION lib/${PROJECT_NAME}
)





# =========================
# TYPESUPPORT LINKING (IMPORTANT FIX)
# =========================

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} "rosidl_typesupport_cpp"
)

target_link_libraries(cube_client_node
  "${cpp_typesupport_target}"
)

target_link_libraries(cube_gripper_node
  "${cpp_typesupport_target}"
)

target_link_libraries(gripper_node
  "${cpp_typesupport_target}"
)



# =========================
# INSTALL FILES
# =========================

install(DIRECTORY
  rviz2
  launch
  DESTINATION share/${PROJECT_NAME}
)

install(PROGRAMS
  scripts/gripper_to_isaac.py
  DESTINATION lib/${PROJECT_NAME}
)

# =========================
# FINAL
# =========================
ament_package()
