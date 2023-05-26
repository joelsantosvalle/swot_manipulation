cmake_minimum_required(VERSION 3.0.2)
project(swot_manipulation_bt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  ur_ros_driver
  message_generation
  cartesian_control_msgs
  geometry_msgs
  pcl_ros
  swot_ur
  swot_msgs
  darknet_ros_msgs
  tf2_ros
  tf
  tf2
  sensor_msgs
  std_srvs
  std_msgs
  behaviortree_cpp_v3
  actionlib_msgs
  actionlib
  control_msgs
  actionlib
  std_srvs
)

catkin_package(
 INCLUDE_DIRS include
 CATKIN_DEPENDS roscpp swot_ur swot_msgs 
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${swot_ur_INCLUDE_DIRS}
)

link_directories($ENV{HOME}/catkin_ws/devel/lib)

add_executable(swot_manipulation_bt src/swot_manipulation.cpp)
add_dependencies(swot_manipulation_bt ${${PROJECT_NAME}_EXPORTED_TARGETS}  ${catkin_EXPORTED_TARGETS})
target_link_libraries(swot_manipulation_bt ${catkin_LIBRARIES} behaviortree_cpp_v3 robotiq_2f_gripper ur_pass_through ur_rtde)

add_executable(swot_manipulation_enhanced src/swot_manipulation_enhanced.cpp)
add_dependencies(swot_manipulation_enhanced ${${PROJECT_NAME}_EXPORTED_TARGETS}  ${catkin_EXPORTED_TARGETS})
target_link_libraries(swot_manipulation_enhanced ${catkin_LIBRARIES} behaviortree_cpp_v3 robotiq_2f_gripper ur_pass_through ur_rtde)