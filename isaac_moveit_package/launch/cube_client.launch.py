#!/usr/bin/env python3

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, TimerAction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue  # ✅ FIX: for typed params
from ament_index_python.packages import get_package_share_directory

from moveit_configs_utils import MoveItConfigsBuilder


def generate_launch_description():

    ros2_control_hardware_type = DeclareLaunchArgument(
        "ros2_control_hardware_type",
        default_value="isaac",
        description="Hardware type: [mock_components, isaac]",
    )

    use_sim_time = DeclareLaunchArgument(
        "use_sim_time",
        default_value="true",
    )

    # -------------------------
    # ✅ TIME SYNC FIX:
    # Force use_sim_time to be a BOOL, not the string "true".
    # Define once and reuse on every node below.
    # -------------------------
    use_sim_time_param = {
        "use_sim_time": ParameterValue(
            LaunchConfiguration("use_sim_time"), value_type=bool
        )
    }

    moveit_config = (
        MoveItConfigsBuilder("moveit_resources_panda")
        .robot_description(
            file_path="config/panda.urdf.xacro",
            mappings={
                "ros2_control_hardware_type": LaunchConfiguration(
                    "ros2_control_hardware_type"
                )
            },
        )
        .robot_description_semantic(file_path="config/panda.srdf")
        .trajectory_execution(file_path="config/gripper_moveit_controllers.yaml")
        .planning_pipelines(
            pipelines=["ompl", "pilz_industrial_motion_planner"]
        )
        .to_moveit_configs()
    )

    # -------------------------
    # GLOBAL PARAM SERVER FIX (IMPORTANT)
    # -------------------------
    moveit_params = moveit_config.to_dict()

    # -------------------------
    move_group_node = Node(
        package="moveit_ros_move_group",
        executable="move_group",
        output="screen",
        parameters=[
            moveit_params,
            use_sim_time_param,
        ],
    )

    rviz_config_file = os.path.join(
        get_package_share_directory("isaac_moveit"),
        "rviz2",
        "panda_moveit_config.rviz",
    )

    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        output="screen",
        arguments=["-d", rviz_config_file],
        parameters=[
            moveit_params,
            use_sim_time_param,
        ],
    )

    world2robot_tf_node = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        arguments=[
            "0.0", "-0.64", "0.0",
            "0.0", "0.0", "0.0",
            "world", "panda_link0"
        ],
        parameters=[use_sim_time_param],  # ✅ FIX: added
    )

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="both",
        parameters=[
            moveit_config.robot_description,
            use_sim_time_param,
        ],
    )

    ros2_controllers_path = os.path.join(
        get_package_share_directory("moveit_resources_panda_moveit_config"),
        "config",
        "ros2_controllers.yaml",
    )

    ros2_control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        output="screen",
        parameters=[
            ros2_controllers_path,
            use_sim_time_param,
        ],
        remappings=[
            ("/controller_manager/robot_description", "/robot_description"),
        ],
    )

    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster", "-c", "/controller_manager"],
    )

    panda_arm_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["panda_arm_controller", "-c", "/controller_manager"],
    )


    # -------------------------
    # 🔥 delayed motion node
    # -------------------------
    cube_client_cpp_node = TimerAction(
        period=5.0,   # wait for SRDF + MoveGroup + controllers
        actions=[
            Node(
                package="isaac_moveit_package",
                executable="cube_client_node",
                name="cube_client",
                output="screen",
                parameters=[
                    moveit_params,
                    use_sim_time_param,
                ],
            )
        ]
    )

    return LaunchDescription([
        ros2_control_hardware_type,
        use_sim_time,

        move_group_node,
        rviz_node,

        world2robot_tf_node,
        robot_state_publisher,

        ros2_control_node,
        joint_state_broadcaster_spawner,
        panda_arm_controller_spawner,

        #gripper_to_isaac_bridge,

        cube_client_cpp_node,
    ])