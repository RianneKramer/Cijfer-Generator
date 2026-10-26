# README

The 'Cijfer Generator' is a ROS2 project that generates exam results and calculates subject results based on student grades.

## Installation Guide
### Prerequisites
To install this project, you need to have ROS2 and Docker Engine or Desktop installed on your system. Follow the instructions on the [official ROS2 website](https://docs.ros.org/en/jazzy/Installation/Alternatives/Ubuntu-Development-Setup.html) to install ROS2.
To install Docker Engine, visit the [official Docker website](https://docs.docker.com/engine/install/ubuntu/) and follow the installation instructions.

### Cloning and Building
Clone this repository to the src directory of your ROS2 workspace:
```bash
git clone https://github.com/RianneKramer/Cijfer-Generator.git
```
Building can be done in the installation directory:
```bash
cd ws/install
colcon build
```
### Starting the Project

To start the project, run the following command:
```bash
ros2 launch groep4_26_assign1_pkg launch.xml
```

## Developer Guide
### Project Structure
`groep4_26_assign1_pkg` is the package for the ros2 nodes, if you want to make use of header files `include/groep4_26_assign1_pkg` can be used. This pkg also includes a launch and parameter file.
All nodes have their own directory within the package.

`groep4_26_assign1_interface_pkg` is the package for the ros2 custom interface used in this project.
