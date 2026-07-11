# Simple Pick-and-Place: Isaac Sim + MoveIt2

A simple pick-and-place implementation using NVIDIA Isaac Sim for simulation
and MoveIt2 for motion planning, built on ROS 2 Jazzy.

## Description

This workspace connects Isaac Sim to MoveIt2 through a ros2_control
bridge, allowing motion plans generated in MoveIt2 to drive a simulated robot
in Isaac Sim for pick-and-place tasks.

## Setup

Clone into the `src` folder of your ROS 2 workspace:

```bash
cd ~/jazzyws/src
git clone https://github.com/Adnan4666/simple_pnp_isaac_sim_moveit2.git
```

Install dependencies and build:

```bash
cd ~/jazzyws
rosdep install --from-paths src --ignore-src -r -y
colcon build
source install/setup.bash
```

## Usage

1. Launch Isaac Sim with the robot scene loaded.
2. Start the MoveIt2 planning and control stack:

```bash
source ~/jazzyws/install/setup.bash
ros2 launch isaac_moveit_package <your_launch_file>.launch.py
```

## Demo # 1 : Isaac Sim & Rviz

1. From Isaac Sim. menu -> window -> Examples -> Robotics Examples -> ROS2 -> MOVEIT -> FRANKA MoveIt -> Load Sample Scene 
2. Run the command below:

```bash
ros2 launch isaac_moveit_package isaac_moveit.launch.py
```

3. In rviz move the goal robot (orange) to some pose and then click plan and execute button on rviz.
4. The robot in Isaac Sim and rviz will be same.


## Demo # 2 : Isaac Sim + Moveit2

1. From Isaac Sim. menu -> window -> Examples -> Robotics Examples -> ROS2 -> MOVEIT -> FRANKA MoveIt -> Load Sample Scene 
2. Run the command below:

```bash
ros2 launch isaac_moveit_package motion.launch.py
```
3. It will run src/motion.cpp code to move the robot to a pre-defined pose in cartesian space.
4. Robot motion in rviz and Isaac Sim is synchronized. 

## Demo # 3 : Isaac Sim + Moveit2 + ROS service client

1. Go to Isaac Sim .
2. Run the command below (client node):


## Demo # 4 : Pick-and-Place: Isaac Sim + MoveIt2

1. Create -> Shape -> Cube, 
2. Select cube -> Scale : set dimensions to 0.03, 0.03, 0.03 -> Translate : Z=0.15
3. Cube -> Add -> Physics -> Collision preset
4. On Stage keep the Cube under Franka
5. Select ActionGraph -> Open Graph -> create client server for the position and orientation of the cube 
6. Alternatively you can load panda.usd in Isaac-sim
7. Run the simulation
8. Run the command below:

```bash
ros2 launch isaac_moveit_package cube_gripper.launch.py
```
9. It will run run src/cube_gripper.cpp
