/**
*       place.cpp
*
*       @date 14.07.2023
*       @author Joel Santos
*/

CheckObjRequired::CheckObjRequired(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
CheckObjRequired::~CheckObjRequired() override = default;      
BT::NodeStatus CheckObjRequired::tick() override
{
    manipulation_.set_collision(false);
    if (manipulation_.get_request().save == "SAVE_1")
    {
        ROS_INFO("Object from tray 1 required");
        objects_in_trays[0] = "";
    }
    else if (manipulation_.get_request().save == "SAVE_2")
    {
        ROS_INFO("Object from tray 2 required");
        objects_in_trays[1] = "";
    }
    else
    {
        ROS_INFO("Object from tray 3 required");
        objects_in_trays[2] = "";
    }
    return BT::NodeStatus::SUCCESS;
}     


CheckWSFree::CheckWSFree(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
CheckWSFree::~CheckWSFree() override = default;      
BT::NodeStatus CheckWSFree::tick() override
{
    ROS_INFO("check ws free");
    if(manipulation_.req_.task == "RED_CONTAINER" || manipulation_.req_.task == "BLUE_CONTAINER")
    {

    }
    else
    {
    swot_msgs::SwotFreeSpot srv_free;
    if (ros::service::waitForService("FreeSpotServer", ros::Duration(5.0)))
    {
        std::cout << "FreeSpotServer" << std::endl;
        (manipulation_.rtde)->joint_target(manipulation_.array_scan_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
        ros::Duration(2).sleep();
            if (!(manipulation_.service_client_free).call(srv_free))
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_scan_right, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                ros::Duration(2).sleep();
                if (!(manipulation_.service_client_free).call(srv_free))
                {
                    (manipulation_.rtde)->joint_target(manipulation_.array_scan_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);                   
                    ros::Duration(2).sleep();
                    if(!(manipulation_.service_client_free).call(srv_free))
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
    manipulation_.set_grasping_point(srv_free.response.pose);
    std::cout << manipulation_.get_grasping_point() << std::endl;
    }
    return BT::NodeStatus::SUCCESS;
}


MoveToTray::MoveToTray(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
MoveToTray::~MoveToTray() override = default;      
BT::NodeStatus MoveToTray::tick() override
{
    ROS_INFO("move to tray");
    (manipulation_.rtde)->joint_target(manipulation_.array_rotate1, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
    (manipulation_.rtde)->joint_target(manipulation_.array_rotate2, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
    manipulation_.tray_top();       
    return BT::NodeStatus::SUCCESS;   
}


PickFromTray::PickFromTray(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
PickFromTray::~PickFromTray() override = default;      
BT::NodeStatus PickFromTray::tick() override
{
    ROS_INFO("pick from tray");
    if (manipulation_.get_tray() == "SAVE_1")
    {
        (manipulation_.rtde)->joint_target(manipulation_.array_tray1_load, manipulation_.jnt_vel_, manipulation_.jnt_acc_);  
    }
    else if (manipulation_.get_tray() == "SAVE_2")
    {
        (manipulation_.rtde)->joint_target(manipulation_.array_tray2_load, manipulation_.jnt_vel_, manipulation_.jnt_acc_);         
    }
    else
    {
        (manipulation_.rtde)->joint_target(manipulation_.array_tray3_load, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
    }
    ros::Duration(0.5).sleep();

    manipulation_.set_collision(false);
    array6d free_axis = {1,1,1,0,0,0};
    array6d wrench = {0,0,-20,0,0,0};
    (manipulation_.rtde)->force_target(true, free_axis, wrench, 1.0);
    ROS_INFO("Force Mode activated");

    ros::Duration(0.5).sleep();
    manipulation_.set_collision_activated(true);

    long int  timer = 0;
    while ( (manipulation_.get_collision() == false) && (timer<100) )
    {
        ros::Duration(0.1).sleep();
        timer++;
    }

    (manipulation_.rtde)->force_target(false, free_axis , wrench, 1.0);
    ROS_INFO("Force Mode deactivated");
    manipulation_.set_collision_activated(false);
    manipulation_.set_collision(false);
    ros::Duration(0.1).sleep();
    geometry_msgs::TransformStampedConstPtr pcp_pose_;
    pcp_pose_ = ros::topic::waitForMessage<geometry_msgs::TransformStamped>("/tcp_pose", ros::Duration(2.0));

    array7d target = {pcp_pose_->transform.translation.x, pcp_pose_->transform.translation.y, pcp_pose_->transform.translation.z + 0.006, 
                    pcp_pose_->transform.rotation.x, pcp_pose_->transform.rotation.y, pcp_pose_->transform.rotation.z, 
                    pcp_pose_->transform.rotation.w};
    (manipulation_.rtde)->cart_target(1, target, manipulation_.jnt_vel_*0.2, manipulation_.jnt_acc_*0.2);

    (manipulation_.rtde)->gripper_close(manipulation_.gripper_speed_, manipulation_.gripper_force_);
    manipulation_.tray_top();          
    return BT::NodeStatus::SUCCESS;
}

MoveToDropPos::MoveToDropPos(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
MoveToDropPos::~MoveToDropPos() override = default;      
BT::NodeStatus MoveToDropPos::tick() override
{
    ROS_INFO("move to drop pos");
    (manipulation_.rtde)->joint_target(manipulation_.array_rotate2, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
    (manipulation_.rtde)->joint_target(manipulation_.array_rotate1, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
    (manipulation_.rtde)->joint_target(manipulation_.array_scan_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
    manipulation_.set_last_pos("mid");
    return BT::NodeStatus::SUCCESS;
}