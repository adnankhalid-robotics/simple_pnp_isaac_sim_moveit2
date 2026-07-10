#include <rclcpp/rclcpp.hpp>

#include <moveit/move_group_interface/move_group_interface.h>
#include <geometry_msgs/msg/pose_stamped.hpp>

static const rclcpp::Logger LOGGER = rclcpp::get_logger("motion_node");

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("motion_node");

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);
  std::thread([&executor]() { executor.spin(); }).detach();

  // MoveIt interface
  moveit::planning_interface::MoveGroupInterface move_group(node, "panda_arm");

  move_group.setPlanningTime(2.0);
  move_group.setMaxVelocityScalingFactor(1.0);
  move_group.setMaxAccelerationScalingFactor(1.0);

  // POSE GOAL
  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id = "panda_link0";
  pose.pose.position.x = 0.6;
  pose.pose.position.y = 0.0;
  pose.pose.position.z = 0.3;
  pose.pose.orientation.x = 1.0;
  pose.pose.orientation.y = 0.0;
  pose.pose.orientation.z = 0.0;
  pose.pose.orientation.w = 0.0;   

  move_group.setPoseTarget(pose);

  moveit::planning_interface::MoveGroupInterface::Plan plan1;

  if (move_group.plan(plan1) == moveit::core::MoveItErrorCode::SUCCESS)
  {
    RCLCPP_INFO(LOGGER, "Pose planning success");
    move_group.execute(plan1);
  }
  else
  {
    RCLCPP_ERROR(LOGGER, "Pose planning failed");
  }

  rclcpp::shutdown();
  return 0;
}