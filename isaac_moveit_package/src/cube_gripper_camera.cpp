#include <rclcpp/rclcpp.hpp>

#include <moveit/move_group_interface/move_group_interface.hpp>
#include <moveit/robot_state/robot_state.hpp>   // getCurrentState / copyJointGroupPositions
#include <geometry_msgs/msg/pose_stamped.hpp>

#include "isaac_moveit_package/srv/cube_position.hpp"

#include <thread>
#include <cmath>     
#include <vector>

using GetCubePose = isaac_moveit_package::srv::CubePosition;

static const rclcpp::Logger LOGGER = rclcpp::get_logger("motion_node");

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("cub_gripper_data");

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);
  std::thread spin_thread([&executor]() { executor.spin(); });

  // SERVICE CLIENT
  auto client = node->create_client<GetCubePose>("/cube_position");

  RCLCPP_INFO(LOGGER, "Waiting for get_cube_pose service...");
  if (!client->wait_for_service(std::chrono::seconds(5)))
  {
    RCLCPP_ERROR(LOGGER, "Service get_cube_pose not available");
    executor.cancel();
    spin_thread.join();
    rclcpp::shutdown();
    return 1;
  }

  auto request = std::make_shared<GetCubePose::Request>();
  auto future = client->async_send_request(request);

  if (future.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
  {
    RCLCPP_ERROR(LOGGER, "Failed to call service get_cube_pose");
    executor.cancel();
    spin_thread.join();
    rclcpp::shutdown();
    return 1;
  }

  auto response = future.get();

  RCLCPP_INFO(LOGGER,
              "Cube Position: [%.3f, %.3f, %.3f]",
              response->x, response->y, response->z);

  // MOVEIT SETUP
  moveit::planning_interface::MoveGroupInterface arm_group(node, "panda_arm");

  arm_group.setPlanningTime(5.0);
  arm_group.setMaxVelocityScalingFactor(0.8);
  arm_group.setMaxAccelerationScalingFactor(0.8);

  // Make sure the planning frame is the one we're specifying targets in.
  RCLCPP_INFO(LOGGER, "Planning frame: %s",
              arm_group.getPlanningFrame().c_str());
  RCLCPP_INFO(LOGGER, "End effector link: %s",
              arm_group.getEndEffectorLink().c_str());

  arm_group.startStateMonitor();
  rclcpp::sleep_for(std::chrono::seconds(2));

  // CURRENT JOINT STATE
  // Remember where the robot started so we can return here after grasping.
  std::vector<double> start_joint_values;
  moveit::core::RobotStatePtr current_state = arm_group.getCurrentState(5.0);

  if (current_state)
  {
    current_state->copyJointGroupPositions("panda_arm", start_joint_values);

    RCLCPP_INFO(LOGGER, "Current Joint States (saved as start position):");
    for (size_t i = 0; i < start_joint_values.size(); ++i)
      RCLCPP_INFO(LOGGER, "  Joint %zu: %.4f", i, start_joint_values[i]);
  }
  else
  {
    RCLCPP_WARN(LOGGER, "Failed to get current robot state");
  }

  // CURRENT EE POSE
  auto current_pose = arm_group.getCurrentPose();

  RCLCPP_INFO(LOGGER,
              "EE Position -> x: %.3f, y: %.3f, z: %.3f",
              current_pose.pose.position.x,
              current_pose.pose.position.y,
              current_pose.pose.position.z);

  RCLCPP_INFO(LOGGER,
              "EE Orientation -> x: %.3f, y: %.3f, z: %.3f, w: %.3f",
              current_pose.pose.orientation.x,
              current_pose.pose.orientation.y,
              current_pose.pose.orientation.z,
              current_pose.pose.orientation.w);

  // TARGET POSE  (move to cube position from service)
  const double APPROACH_Z_OFFSET = 0.105;

  geometry_msgs::msg::PoseStamped target_pose;
  target_pose.header.frame_id = "panda_link0";
  target_pose.pose.position.x = response->x;
  target_pose.pose.position.y = response->y;
  target_pose.pose.position.z = response->z + APPROACH_Z_OFFSET;

  target_pose.pose.orientation.x = 1.0;
  target_pose.pose.orientation.y = 0.0;
  target_pose.pose.orientation.z = 0.0;
  target_pose.pose.orientation.w = 0.0;


  const double GRIPPER_YAW_OFFSET = M_PI / 4.0;

  double cube_yaw = std::atan2(
      2.0 * (target_pose.pose.orientation.w * target_pose.pose.orientation.z + target_pose.pose.orientation.x * target_pose.pose.orientation.y),
      1.0 - 2.0 * (target_pose.pose.orientation.y * target_pose.pose.orientation.y + target_pose.pose.orientation.z * target_pose.pose.orientation.z));

  const double HALF_PI = M_PI / 2.0;
  double grasp_yaw = cube_yaw + GRIPPER_YAW_OFFSET;
  grasp_yaw -= HALF_PI * std::round(grasp_yaw / HALF_PI);

  const double cy = std::cos(grasp_yaw / 2.0);
  const double sy = std::sin(grasp_yaw / 2.0);

  target_pose.pose.orientation.x = cy;
  target_pose.pose.orientation.y = sy;
  target_pose.pose.orientation.z = 0.0;
  target_pose.pose.orientation.w = 0.0;


  RCLCPP_INFO(LOGGER,
              "Target Position -> x: %.3f, y: %.3f, z: %.3f (frame: %s)",
              target_pose.pose.position.x,
              target_pose.pose.position.y,
              target_pose.pose.position.z,
              target_pose.header.frame_id.c_str());

  RCLCPP_INFO(LOGGER,
              "Target Orientation -> x: %.3f, y: %.3f, z: %.3f, w: %.3f",
              target_pose.pose.orientation.x,
              target_pose.pose.orientation.y,
              target_pose.pose.orientation.z,
              target_pose.pose.orientation.w);

  // PLAN (approach)
  arm_group.setPoseTarget(target_pose);
  moveit::planning_interface::MoveGroupInterface::Plan arm_plan;

  if (arm_group.plan(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS)
  {
    RCLCPP_INFO(LOGGER, "Planning success. Executing...");

    // EXECUTE (approach)
    if (arm_group.execute(arm_plan) == moveit::core::MoveItErrorCode::SUCCESS)
    {
      RCLCPP_INFO(LOGGER, "Execution success. Robot moved to cube position.");


      rclcpp::sleep_for(std::chrono::seconds(5));

      // GRASP THE CUBE
      moveit::planning_interface::MoveGroupInterface hand_group(node, "hand");
      hand_group.setNamedTarget("close");   

      if (hand_group.move() == moveit::core::MoveItErrorCode::SUCCESS)
        RCLCPP_INFO(LOGGER, "Gripper closed. Cube grasped.");
      else
        RCLCPP_WARN(LOGGER, "Gripper close did not report success (may have stalled on contact). Continuing.");


      rclcpp::sleep_for(std::chrono::seconds(5));  

      // RETURN TO STARTING POSITION
      // Carry the cube back to where the arm started before heading to the
      if (!start_joint_values.empty())
      {
        arm_group.setJointValueTarget(start_joint_values);
      }
      else
      {
        RCLCPP_WARN(LOGGER, "No saved start joints; falling back to named target 'ready'.");
        arm_group.setNamedTarget("ready");
      }

      if (arm_group.move() == moveit::core::MoveItErrorCode::SUCCESS)
        RCLCPP_INFO(LOGGER, "Returned to starting position with the cube.");
      else
        RCLCPP_WARN(LOGGER, "Failed to return to starting position. Continuing to place.");


      rclcpp::sleep_for(std::chrono::seconds(4));

      // MOVE TO PLACE POSITION AND RELEASE
      geometry_msgs::msg::PoseStamped place_pose;
      place_pose.header.frame_id = "panda_link0";
      place_pose.pose.position.x = 0.46;
      place_pose.pose.position.y = 0.40;
      place_pose.pose.position.z = 0.3;
      place_pose.pose.orientation = target_pose.pose.orientation;

      arm_group.setPoseTarget(place_pose);

      if (arm_group.move() == moveit::core::MoveItErrorCode::SUCCESS)
      {
        RCLCPP_INFO(LOGGER, "Reached place position [0.46, 0.33, 0.3].");

        rclcpp::sleep_for(std::chrono::seconds(4));

        // Open the gripper to release the cube.
        hand_group.setNamedTarget("open");   

        if (hand_group.move() == moveit::core::MoveItErrorCode::SUCCESS)
          RCLCPP_INFO(LOGGER, "Gripper opened. Cube released.");
        else
          RCLCPP_WARN(LOGGER, "Gripper open did not report success. Continuing.");
      }
      else
      {
        RCLCPP_ERROR(LOGGER, "Failed to reach place position.");
      }
    }
    else
    {
      RCLCPP_ERROR(LOGGER, "Execution failed.");
    }
  }
  else
  {
    RCLCPP_ERROR(LOGGER, "Planning failed");
    executor.cancel();
    spin_thread.join();
    rclcpp::shutdown();
    return 1;
  }

  // CLEAN SHUTDOWN
  executor.cancel();
  spin_thread.join();
  rclcpp::shutdown();
  return 0;
}