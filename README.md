# README
Please keep in mind that the CMakeLists.txt, launch.xml & parameters.yaml are not yet initialized, when making a pull request to main this might cause conflicts. If you are reviewing it yourself make sure that you DO NOT overwrite the CMakeList for your own part of the project. These are shared files.

## Installation Guide
Clone this repository to the src of your workspace
I.e. 
```
home/usr/les_ws/src
```
Building can be done in the install folder so:
```bash
cd les_ws/install
colcon build
```

## Developer Guide
During the developing process all three members have their own development branch.
Be sure to occasionally push to your respective branches.
Your branch is named like: yourname.0.0.1
If you want to make a new branch you can use yourname.0.0.2
Version progression goes like Major.Minor.Patch

### Project Structure
`group4_26_assign1_pkg` is the packages for the ros2 nodes, if you want to make use of header files `include/group4_26_assign1_pkg` can be used. This pkg also includes a launch and parameter file.

`group4_26_assign1_interface_pkg` is the package for the ros2 custom interface.