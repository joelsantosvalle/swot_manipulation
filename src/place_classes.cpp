/**
*       place_classes.cpp
*
*       @date 14.07.2023
*       @author Joel Santos
*/

#include "swot_manipulation/place_classes.h"

/**
 *      @brief Constructor of the CheckObjRequired class used to initialize the corresponding member variables.
 *      @param name The behavior tree node name.
 *      @param manipulation The Manipulation class object to access the neccesary data.
 */

CheckObjRequired::CheckObjRequired(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) 
{

}

/**
 * 	    @brief Destructor of class CheckObjRequired.
 */

CheckObjRequired::~CheckObjRequired()  = default;      

/**
 *      @brief Executes the tick operation of the node CheckObjRequired.
 *      @return The execution status of the node which in this case can be SUCCESS or FAILURE.
 */

BT::NodeStatus CheckObjRequired::tick() 
{
    manipulation_.set_collision_detected(false);
    if(manipulation_.get_request_vector()[0].tasks[manipulation_.get_task_count()].mode != "PLACE")
    {
        return BT::NodeStatus::FAILURE;
    }
    else
    {
        return BT::NodeStatus::SUCCESS;
    }
}     

/**
 *      @brief Constructor of the CheckWSFree class used to initialize the corresponding member variables.
 *      @param name The behavior tree node name.
 *      @param manipulation The Manipulation class object to access the neccesary data.
 */

CheckWSFree::CheckWSFree(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) 
{

}

/**
 * 	    @brief Destructor of class CheckWSFree.
 */

CheckWSFree::~CheckWSFree()  = default;      

/**
 *      @brief Executes the tick operation of the node CheckWSFree.
 *      @return The execution status of the node which in this case can be SUCCESS or FAILURE.
 */

BT::NodeStatus CheckWSFree::tick() 
{
    ROS_INFO("check ws free");
    swot_msgs::SwotFreeSpot srv_free;
    for(auto i = 0; i < 4; i++)
    {
        srv_free.request.ws_dimensions[i] = manipulation_.get_workspace_dimensions_matching_object().workspace_dimensions[manipulation_.index(manipulation_.get_request_vector()[0].tasks[manipulation_.get_task_count()].task)][i+1];
    }
    if (ros::service::waitForService("FreeSpotServer", ros::Duration(3.0)))
    {
        std::cout << "FreeSpotServer" << std::endl;
        std::string scan_left = "array_scan_left_yolo_" + std::to_string(manipulation_.get_workspace_dimensions_matching_object().workspace_dimensions[manipulation_.index(manipulation_.get_request_vector()[0].tasks[manipulation_.get_task_count()].task)][4]);
        manipulation_.sendTargetPosition6d(scan_left);
            if (!(manipulation_.get_service_client_free()).call(srv_free))
            {
                std::string scan_right = "array_scan_right_yolo_" + std::to_string(manipulation_.get_workspace_dimensions_matching_object().workspace_dimensions[manipulation_.index(manipulation_.get_request_vector()[0].tasks[manipulation_.get_task_count()].task)][4]);
                manipulation_.sendTargetPosition6d(scan_right);
                if (!(manipulation_.get_service_client_free()).call(srv_free))
                {
                    std::string scan_mid = "array_scan_mid_" + std::to_string(manipulation_.get_workspace_dimensions_matching_object().workspace_dimensions[manipulation_.index(manipulation_.get_request_vector()[0].tasks[manipulation_.get_task_count()].task)][4]);
                    manipulation_.sendTargetPosition6d(scan_mid);
                    if(!(manipulation_.get_service_client_free()).call(srv_free))
                    {
                        return BT::NodeStatus::FAILURE;
                    }
                }
            }
    }
    else
    {
        ROS_WARN("Couldn't find ROS Service \"SwotFreeSpot\"");
        return BT::NodeStatus::FAILURE;
    }
    for(auto i = 0; i < manipulation_.get_request_vector().size(); i++)
    {
        if(manipulation_.getPlaceTracker()[i].first[0] == manipulation_.get_task_count())
        {
            manipulation_.getPlaceTracker()[i].second.pose = srv_free.response.pose;
        }
    }
    return BT::NodeStatus::SUCCESS;
}

/**
 *      @brief Constructor of the MoveToTray class used to initialize the corresponding member variables.
 *      @param name The behavior tree node name.
 *      @param manipulation The Manipulation class object to access the neccesary data.
 */

MoveToTray::MoveToTray(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) 
{

}

/**
 * 	    @brief Destructor of class MoveToTray.
 */

MoveToTray::~MoveToTray()  = default;   

/**
 *      @brief Executes the tick operation of the node MoveToTray.
 *      @return The execution status of the node which in this case can be SUCCESS or FAILURE.
 */

BT::NodeStatus MoveToTray::tick() 
{
    ROS_INFO("move to tray");
    manipulation_.sendTargetPosition6d("array_rotate1");
    manipulation_.sendTargetPosition6d("array_rotate2");
    manipulation_.tray_top();       
    return BT::NodeStatus::SUCCESS;   
}

/**
 *      @brief Constructor of the PickFromTray class used to initialize the corresponding member variables.
 *      @param name The behavior tree node name.
 *      @param manipulation The Manipulation class object to access the neccesary data.
 */

PickFromTray::PickFromTray(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) 
{

}

/**
 * 	    @brief Destructor of class PickFromTray.
 */

PickFromTray::~PickFromTray()  = default; 

/**
 *      @brief Executes the tick operation of the node PickFromTray.
 *      @return The execution status of the node which in this case can be SUCCESS or FAILURE.
 */

BT::NodeStatus PickFromTray::tick() 
{
    ROS_INFO("pick from tray");
    if (manipulation_.get_request_vector()[0].tasks[manipulation_.get_task_count()].save == "SAVE_1")
    {
        manipulation_.sendTargetPosition6d("array_tray1_load");
    }
    else if (manipulation_.get_request_vector()[0].tasks[manipulation_.get_task_count()].save == "SAVE_2")
    {
        manipulation_.sendTargetPosition6d("array_tray2_load");
    }
    else
    {
        manipulation_.sendTargetPosition6d("array_tray3_load");
    }
    std::string object;
    for(auto i = 0; i < manipulation_.get_request_vector().size(); i++)
    {
        if(manipulation_.getPlaceTracker()[i].first[0] == manipulation_.get_task_count())
        {
            object = manipulation_.getPickTracker()[i].first.substr(2);
        }
    }
    manipulation_.get_obj_mani_height() = manipulation_.get_manipulation_height_object().manipulation_heights[manipulation_.index_height(object)];
    ros::Duration(0.5).sleep();
    manipulation_.set_collision_detected(false);
    array6d free_axis = {1,1,1,0,0,0};
    array6d wrench = {0,0,-20,0,0,0};
    (manipulation_.getRTDE())->force_target(true, free_axis, wrench, 1.0);
    ROS_INFO("Force Mode activated");

    ros::Duration(0.5).sleep();
    manipulation_.set_collision_activated(true);

    long int  timer = 0;
    while ( (manipulation_.get_collision_detected() == false) && (timer<100) )
    {
        ros::Duration(0.1).sleep();
        timer++;
    }

    (manipulation_.getRTDE())->force_target(false, free_axis , wrench, 1.0);
    ROS_INFO("Force Mode deactivated");
    manipulation_.set_collision_activated(false);
    manipulation_.set_collision_detected(false);
    ros::Duration(0.1).sleep();
    geometry_msgs::TransformStampedConstPtr pcp_pose_;
    pcp_pose_ = ros::topic::waitForMessage<geometry_msgs::TransformStamped>("/tcp_pose", ros::Duration(2.0));

    array7d target = {pcp_pose_->transform.translation.x, pcp_pose_->transform.translation.y, pcp_pose_->transform.translation.z + manipulation_.get_obj_mani_height(), 
                    pcp_pose_->transform.rotation.x, pcp_pose_->transform.rotation.y, pcp_pose_->transform.rotation.z, 
                    pcp_pose_->transform.rotation.w};
    (manipulation_.getRTDE())->cart_target(1, target, manipulation_.get_jnt_vel_()*0.2, manipulation_.get_jnt_acc_()*0.2);

    (manipulation_.getRTDE())->gripper_close(manipulation_.get_gripper_speed_(), manipulation_.get_gripper_force_());
    manipulation_.tray_top();          
    return BT::NodeStatus::SUCCESS;
}

/**
 *      @brief Constructor of the MoveToDropPos class used to initialize the corresponding member variables.
 *      @param name The behavior tree node name.
 *      @param manipulation The Manipulation class object to access the neccesary data.
 */

MoveToDropPos::MoveToDropPos(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) 
{

}

/**
 * 	    @brief Destructor of class MoveToDropPos.
 */

MoveToDropPos::~MoveToDropPos()  = default;

/**
 *      @brief Executes the tick operation of the node MoveToDropPos.
 *      @return The execution status of the node which in this case can be SUCCESS or FAILURE.
 */

BT::NodeStatus MoveToDropPos::tick() 
{
    ROS_INFO("move to drop pos");
    manipulation_.sendTargetPosition6d("array_rotate2");
    manipulation_.sendTargetPosition6d("array_rotate1");
    manipulation_.sendTargetPosition6d("array_scan_mid");
    manipulation_.set_last_pos("mid");
    return BT::NodeStatus::SUCCESS;
}