cat > README.md << 'EOF'
# Simple Pick-and-Place: Isaac Sim + MoveIt2

A simple pick-and-place implementation using NVIDIA Isaac Sim for simulation
and MoveIt2 for motion planning, built on ROS 2 Jazzy.

## Description

This workspace connects Isaac Sim to MoveIt2 through a topic-based ros2_control
bridge, allowing motion plans generated in MoveIt2 to drive a simulated robot
in Isaac Sim for pick-and-place tasks.

## Dependencies

- ROS 2 Jazzy
- MoveIt2
- NVIDIA Isaac Sim
- `topic_based_ros2_control`

## Packages

- `isaac_moveit_package` – core pick-and-place nodes and launch files
- `moveit_resources` – robot description and MoveIt config resources
- `topic_based_ros2_control` – ros2_control hardware interface bridging topics

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

3. Use the MoveIt2 RViz plugin (or your pick-and-place node) to plan and
   execute motions, which will be mirrored in Isaac Sim.

## License

See the LICENSE file for details.
EOF
