#include "ros/ros.h"
#include <ros/package.h>
#include <swot_ur/ur_rtde.h>
#include <swot_msgs/SwotManipulation.h>
#include <swot_msgs/SwotObjectMatching2023.h>
#include <swot_msgs/SwotFreeSpot.h>
#include <swot_msgs/SwotWeightedBBox.h>
#include <swot_msgs/SwotBBoxCheck.h>
#include <darknet_ros_msgs/BoundingBox.h>
#include <darknet_ros_msgs/BoundingBoxes.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/WrenchStamped.h>
#include <tf2_ros/transform_listener.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <behaviortree_cpp_v3/loggers/bt_zmq_publisher.h>
#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/condition_node.h>
#include <behaviortree_cpp_v3/action_node.h>

typedef boost::array<double, 6> array6d;
typedef boost::array<long int, 6> array6i;
typedef boost::array<double, 7> array7d;

std::vector<std::string> objects_in_trays{"","",""};
std::vector<std::string> possible_last_positions{};

class Manipulation;
void registerNodes(BT::BehaviorTreeFactory& factory, Manipulation& manipulation);

class Manipulation
{    
    private:

        std::string xml_file;
        std::string last_pos;    
        std::string grasping_area; 
        const double wrench_limit;
        bool collision_detected;
        bool collision_activated;
        ros::NodeHandle nh_;
        ros::Subscriber sub_wrench;
        geometry_msgs::Pose grasping_point;
        static inline bool initialized = false;
        std::string tray;

    public:
        std::array<array6d, 3> scan_pose = {array_scan_left_yolo, array_scan_right_yolo, array_scan_mid};
        std::unique_ptr<URRTDE> rtde;
        ros::ServiceServer service_server;
        ros::ServiceClient service_client_matching;
        ros::ServiceClient service_client_free;
        swot_msgs::SwotManipulation::Request req_;
        swot_msgs::SwotManipulation::Response res_;

        double gripper_speed_ = 1.0;
        double gripper_force_ = 70.0;

        double jnt_vel_ = 1;
        double jnt_acc_ = 1;

        int move_duration = 5.0;
        double blend_ = 0.02;

        Manipulation() : last_pos("drive"), grasping_area("mid"), wrench_limit(7.0), collision_detected(false), collision_activated(false)
        {
            if(!this->initialized)
            {
                rtde = std::make_unique<URRTDE>(nh_);
                service_server = nh_.advertiseService("SwotManipulationBT", &Manipulation::callback_service_manipulation, this);
                service_client_matching = nh_.serviceClient<swot_msgs::SwotObjectMatching2023>("ObjectMatchingServer");
                service_client_free = nh_.serviceClient<swot_msgs::SwotFreeSpot>("FreeSpotServer");
                sub_wrench = nh_.subscribe("wrench", 1000, &Manipulation::callback_wrench, this);
                
                rtde->gripper_open(gripper_speed_, gripper_force_);

                ROS_INFO("ROS service started"); 
                initialized = true;
            }
        }
  
        bool virtual callback_service_manipulation(swot_msgs::SwotManipulation::Request &req, swot_msgs::SwotManipulation::Response &res)
        {              
            this->req_ = req;
            this->res_ = res;
            ros::Duration(1).sleep();

            BT::BehaviorTreeFactory factory;
            registerNodes(factory, *this);            
            nh_.param<std::string>("file", xml_file,"/home/irobot/catkin_ws/src/swot_manipulation_bt/bt_xml_structure/swot_manipulation_backup_with_decorator.xml");
            auto tree = factory.createTreeFromFile(xml_file);
            tree.tickRoot();
            return true;
        }

        virtual ~Manipulation() {}
        
        void callback_wrench(const geometry_msgs::WrenchStamped &msg)
        {
            if(collision_activated)
            {
            if (collision_detected == false)
            {
                if (std::max({std::abs(msg.wrench.force.x), std::abs(msg.wrench.force.y), std::abs(msg.wrench.force.z)}) > wrench_limit)
                {
                    ROS_INFO("wrench_limit achieved");
                    set_collision(true);
                    set_collision_activated(false);
                }
            }
            }
        }
   
        void set_last_pos(std::string last_pos){ this->last_pos = last_pos;};
        std::string get_last_pos() const {return this->last_pos;};
        
        void set_grasping_area(std::string grasping_area){ this->grasping_area = grasping_area;};
        std::string get_grasping_area() const {return this->grasping_area;};

        void set_collision(bool collision){ this->collision_detected = collision;};
        bool get_collision() const {return this->collision_detected;};

        void set_collision_activated(bool collision){ this->collision_activated = collision;};
        bool get_collision_activated() const {return this->collision_activated;};
        
        void set_tray(std::string tray){ this->tray = tray;};
        std::string get_tray() const {return this->tray;};
    
        void set_grasping_point(geometry_msgs::Pose grasping) {this->grasping_point = grasping;};
        geometry_msgs::Pose get_grasping_point() const {return this->grasping_point;};
    
        swot_msgs::SwotManipulation::Request get_request() const {return this->req_;};
        swot_msgs::SwotManipulation::Response get_response() const {return this->res_;};
        ros::NodeHandle get_nh() {return nh_;};

        array6d array_scan_mid = {2.40435886383057, -1.83808960537099, 0.975212875996725, -0.674065129165985, -1.63826924959292, -3.8627772966968};
        array6d array_scan_left = {3.681095838546753, -1.2161747378161927, 0.6712544600116175, -1.0322970908931275, -1.663314167653219, -2.6715996901141565};
        array6d array_scan_left_yolo = {3.31208157539368, -1.67535986522817, 1.43538600603213, -1.27126656592403, -1.62322599092592, -3.0253372828113};
        array6d array_scan_right_yolo = {2.03880262374878, -1.67188944439077, 1.43529826799502, -1.32640195012603, -1.58402139345278, -4.25680452982058};
        array6d array_scan_left_pc = {3.681095838546753, -1.2161747378161927, 0.6712544600116175, -1.0322970908931275, -1.663314167653219, -2.6715996901141565};
        array6d array_scan_right_pc = {1.9794570207595825, -1.405427412395813, 0.7290337721454065, -0.7683202785304566, -1.4733336607562464, -4.272872988377706};
        array6d array_scan_right = {1.9794570207595825, -1.405427412395813, 0.7290337721454065, -0.7683202785304566, -1.4733336607562464, -4.272872988377706};
        array6d array_pick_mid = {2.50433206558228, -1.73584236721181, 2.36165410677065, -2.17094959835195, -1.55130368867983, -3.78159839311709};
        array6d array_pick_left_left = {3.57079100608826, -1.25637282550845, 1.81794149080385, -2.13930000881338, -1.54926091829409, -2.72351652780642};
        array6d array_pick_left = {3.0599627494812, -1.64873184780263, 2.17455464998354, -2.03081049541616, -1.54161435762514, -3.26244932809939};
        array6d array_pick_right = {2.05809712409973, -1.44568693757568, 2.1275957266437, -2.25774492839956, -1.57374316850771, -4.27933890024294};
        array6d array_pick_right_right = {1.8982390165329, -1.10214848936115, 1.67234355608095, -2.18172802547597, -1.57016212144961, -4.43200451532473};
        array6d array_rotate1 = {2.3692173957824707, -2.3164030514159144, 1.3390710989581507, -0.9904833000949402, -2.1601603666888636, -2.726298157368795};
        array6d array_rotate2 = {0.9031553864479065, -2.4277192554869593, 1.0507047812091272, -0.964714304809906, -1.9267485777484339, -2.7257021109210413};
        array6d array_rotate3 = {-0.290535275136129, -1.32841757059608, 0.46738320985903, -0.702364699249603, -1.57764035860171, -3.45398217836489};
        array6d array_tray1_top = {0.064236424863339, -1.42052191615615, 0.902454201375143, -1.04965449989352, -1.59723025957216, -3.07310563722719};
        array6d array_tray2_top = {-0.255208794270651, -1.54578061894093, 1.05577546754946, -1.06061519802127, -1.55526000658144, -3.37846404710879};
        array6d array_tray3_top = {-0.53069526353945, -1.57270397762441, 1.04742652574648, -1.04646314800296, -1.56945592561831, -3.61158138910402};
        array6d array_tray1_load = {0.064320102334023, -1.53433151290331, 1.48070460954775, -1.51407157376919, -1.59717017809023, -3.07259160677065};
        array6d array_tray2_load = {-0.255173508320944, -1.6467939815917, 1.58283216158022, -1.48665781438861, -1.55522424379458, -3.37798530260195};
        array6d array_tray3_load = {-0.530647579823629, -1.6887427769103, 1.65178472200503, -1.53478486955676, -1.56944162050356, -3.6110408941852};
        array6d array_drive = {3.18401956558228, -2.55628885845327, 1.20438319841494, -0.691585080032684, -1.76227599779238, -3.09013063112368};
        array6d free_backup_1 = {3.5078, -1.3333, 1.7648, -2.033566, -1.58985, -4.33499};
        array6d free_backup_2 = {2.1859, -1.2849, 2.01598, -2.326377, -1.567803, -2.50999};
};

class NotDrive : public BT::ConditionNode
{
    private:
        Manipulation& manipulation_;

    public:
        NotDrive(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) {}
        ~NotDrive() override = default;
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("drive achieved");
            if(manipulation_.get_request().mode == "DRIVE")
            {
                return BT::NodeStatus::FAILURE;
            }
            else
            {
                return BT::NodeStatus::SUCCESS;
            }
        }
};

class NotPick : public BT::ConditionNode
{
    private:
        Manipulation& manipulation_;

    public:
        NotPick(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) {}
        ~NotPick() override = default;
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("pick achieved");
            if(manipulation_.get_request().mode == "PICK")
            {
                return BT::NodeStatus::FAILURE;
            }
            else
            {
                return BT::NodeStatus::SUCCESS;
            }
        }
        
};

class NotPlace : public BT::ConditionNode
{
    private:
        Manipulation& manipulation_;

    public: 
        NotPlace(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) {}
        ~NotPlace() override = default;
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("place achieved");
            if(manipulation_.get_request().mode == "PLACE")
            { 
                return BT::NodeStatus::FAILURE;
            }
            else
            { 
                return BT::NodeStatus::SUCCESS;
            }
        } 
};

class NotPP : public BT::ConditionNode
{
    private:
        Manipulation& manipulation_;

    public: 
        NotPP(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) {}
        ~NotPP() override = default;
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("pp achieved");
            if(manipulation_.get_request().task == "PP")
            {   
                return BT::NodeStatus::FAILURE;
            }
            else
            {
                return BT::NodeStatus::SUCCESS;
            }
        }
};

class NotSH : public BT::ConditionNode
{
    private:
        Manipulation& manipulation_;

    public: 
        NotSH(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) {}
        ~NotSH() override = default;
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("sh achieved");
            if(manipulation_.get_request().task == "SH")
            {
                return BT::NodeStatus::FAILURE;
            }
            else
            { 
                return BT::NodeStatus::SUCCESS;
            }
        }
};

class NotTT : public BT::ConditionNode
{
    private:
        Manipulation& manipulation_;

    public: 

        NotTT(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) {}
        ~NotTT() override = default;
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("tt achieved");
            if(manipulation_.get_request().task == "TT")
            {             
                return BT::NodeStatus::FAILURE;
            }
            else
            {
                return BT::NodeStatus::SUCCESS;
            }
        }
};

class NotWS : public BT::ConditionNode
{
    private:
        Manipulation& manipulation_;

    public: 
        NotWS(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) {}
        ~NotWS() override = default;
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("ws achieved");
            if(manipulation_.get_request().task == "WS")
            {
                return BT::NodeStatus::FAILURE;
            }
            else
            {
                return BT::NodeStatus::SUCCESS;
            }
        }
};

class MoveToScan : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        MoveToScan(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~MoveToScan() override = default;      
        virtual BT::NodeStatus tick() override
        {
            manipulation_.set_collision(false);
            ROS_INFO("move to scan");
            (manipulation_.rtde)->joint_target(manipulation_.array_scan_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            return BT::NodeStatus::SUCCESS; 
        }               
}; 

class ScanWorkSpace : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;
        int count;

    public:
        ScanWorkSpace(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation), count(0) {}
        ~ScanWorkSpace() override = default;      
        virtual BT::NodeStatus tick() override
        {
            swot_msgs::SwotObjectMatching2023 srv_match;
            srv_match.request.object = manipulation_.get_request().object;
            ROS_INFO("scan workspace");
            for(auto i = 0; i < 3; i++)
            {
                (manipulation_.rtde)->joint_target(manipulation_.scan_pose[i], manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                ros::Duration(1).sleep();
                if(ros::service::waitForService("ObjectMatchingServer", ros::Duration(5.0)) == false)
                {
                    return BT::NodeStatus::FAILURE;   
                }
                if(!(manipulation_.service_client_matching).call(srv_match))
                {
                    ROS_WARN("Couldn't find ROS Service \"SwotObjectMatching\"");
                    return BT::NodeStatus::FAILURE;
                }
                if (srv_match.response.posture == "STANDING" || srv_match.response.posture == "FAILED")
                {
                    return BT::NodeStatus::FAILURE;
                }
                ros::Duration(2).sleep();
                if(srv_match.response.status == "FINISHED")
                {
                    break;
                }
            }

            manipulation_.set_grasping_point(srv_match.response.pose);
            return BT::NodeStatus::SUCCESS;
        }
};

class GetGraspAndMoveGrasp : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        GetGraspAndMoveGrasp(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~GetGraspAndMoveGrasp() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("get grasp and move grasp");
            double left_left_thresh = 0.2;
            double left_thresh = 0.1;
            double right_thresh = -0.1;
            double right_right_thresh = -0.2;
            
            if (manipulation_.get_grasping_point().position.y >= left_left_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("left_left");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < left_left_thresh && manipulation_.get_grasping_point().position.y >= left_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("left");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < left_thresh && manipulation_.get_grasping_point().position.y >= right_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("mid");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < right_thresh && manipulation_.get_grasping_point().position.y >= right_right_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_right, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("right");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < right_right_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("right_right");
                return BT::NodeStatus::SUCCESS;
            }
        
            return BT::NodeStatus::FAILURE;  
        }
};

class PickObject : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        PickObject(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}) , manipulation_(manipulation){}
        ~PickObject() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("pick object");
            
            array7d target = {manipulation_.get_grasping_point().position.x, manipulation_.get_grasping_point().position.y, manipulation_.get_grasping_point().position.z + 0.07,
            manipulation_.get_grasping_point().orientation.x, manipulation_.get_grasping_point().orientation.y, manipulation_.get_grasping_point().orientation.z,
            manipulation_.get_grasping_point().orientation.w};
            (manipulation_.rtde)->cart_target(1, target, manipulation_.jnt_vel_, manipulation_.jnt_acc_);

            ROS_INFO("Move to grasping_point");

            array7d target2 = {manipulation_.get_grasping_point().position.x, manipulation_.get_grasping_point().position.y, manipulation_.get_grasping_point().position.z + 0.001,
            manipulation_.get_grasping_point().orientation.x, manipulation_.get_grasping_point().orientation.y, manipulation_.get_grasping_point().orientation.z,
            manipulation_.get_grasping_point().orientation.w};
            (manipulation_.rtde)->cart_target(1, target2, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                
            ros::Duration(1).sleep();
            (manipulation_.rtde)->gripper_close(manipulation_.gripper_speed_, manipulation_.gripper_force_);
            ros::Duration(3).sleep();
            ROS_INFO("no collision_detected");           
            return BT::NodeStatus::SUCCESS;
        }
};

class CheckObjectPicked : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        CheckObjectPicked(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~CheckObjectPicked() override = default;      
        virtual BT::NodeStatus tick() override
        {
            /*
            ROS_INFO("Checking object picked"); 
            if((manipulation_.gripper)->get_object_detected() == true)
            {
                return BT::NodeStatus::SUCCESS;
            }    
            else if((manipulation_.gripper)->get_current_position() > 10)
            {
                manipulation_.move(move_to_scan_mid);
                manipulation_.move(move_to_drive);
                //(manipulation_.ur)->joint_rad_list({move_to_scan_mid, move_to_drive});
                manipulation_.set_last_pos("drive");
                manipulation_.res_.status = "FAILED";
                return BT::NodeStatus::SUCCESS;
            }
            else
            {
                manipulation_.res_.status = "FAILED";
                return BT::NodeStatus::SUCCESS;
            }
            */
            return BT::NodeStatus::SUCCESS;
        }
};

class CheckObjRequired : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
    
        CheckObjRequired(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~CheckObjRequired() override = default;      
        virtual BT::NodeStatus tick() override
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
};

class MoveUp : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        MoveUp(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~MoveUp() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("move up ");
            ROS_INFO("Moving up + 7cm");
           
            array7d target = {manipulation_.get_grasping_point().position.x, manipulation_.get_grasping_point().position.y, manipulation_.get_grasping_point().position.z + 0.07,
            manipulation_.get_grasping_point().orientation.x, manipulation_.get_grasping_point().orientation.y, manipulation_.get_grasping_point().orientation.z,
            manipulation_.get_grasping_point().orientation.w};
            (manipulation_.rtde)->cart_target(1, target, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            
            if (manipulation_.get_grasping_area() == "left_left")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            }
            else if (manipulation_.get_grasping_area() == "left")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            }
            else if (manipulation_.get_grasping_area() == "right")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_right, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            }
            else if (manipulation_.get_grasping_area() == "right_right")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_right_right, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            }
            else
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            }            
            return BT::NodeStatus::SUCCESS;
        }      
};

class DropObjectInTray : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        DropObjectInTray(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~DropObjectInTray() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("drop object in tray");
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate1, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate2, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate3, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            if (objects_in_trays[0] == "" && manipulation_.get_request().save == "SAVE_1")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray1_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                (manipulation_.rtde)->joint_target(manipulation_.array_tray1_load, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                ros::Duration(1).sleep();
                manipulation_.set_tray("SAVE_1");
                objects_in_trays[0] = manipulation_.get_request().object;
            }
            else if (objects_in_trays[1] == "" && manipulation_.get_request().save == "SAVE_2")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray2_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                (manipulation_.rtde)->joint_target(manipulation_.array_tray2_load, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                ros::Duration(1).sleep();
                manipulation_.set_tray("SAVE_2");
                objects_in_trays[1] = manipulation_.get_request().object;
            }
            else
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray3_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                (manipulation_.rtde)->joint_target(manipulation_.array_tray3_load, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                ros::Duration(1).sleep();
                manipulation_.set_tray("SAVE_3");
                objects_in_trays[2] = manipulation_.get_request().object;
            }
            (manipulation_.rtde)->gripper_open(manipulation_.gripper_speed_, manipulation_.gripper_force_);
            manipulation_.set_last_pos("tray");
            return BT::NodeStatus::SUCCESS;
        }
};

class MoveHomePos : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        MoveHomePos(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~MoveHomePos() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("move home pos");
            if(manipulation_.get_last_pos() == "tray")
            {
                if(manipulation_.get_request().save == "SAVE_1")
                {
                    (manipulation_.rtde)->joint_target(manipulation_.array_tray1_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                }
                else if(manipulation_.get_request().save == "SAVE_2")
                {
                    (manipulation_.rtde)->joint_target(manipulation_.array_tray2_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                }
                else
                {
                    (manipulation_.rtde)->joint_target(manipulation_.array_tray3_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                }
                (manipulation_.rtde)->joint_target(manipulation_.array_rotate3, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                (manipulation_.rtde)->joint_target(manipulation_.array_rotate2, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                (manipulation_.rtde)->joint_target(manipulation_.array_rotate1, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                (manipulation_.rtde)->joint_target(manipulation_.array_scan_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.res_.status = "FINISHED";
                return BT::NodeStatus::SUCCESS; 
            }
            else
            {  
                (manipulation_.rtde)->joint_target(manipulation_.array_scan_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.res_.status = "FINISHED";
                return BT::NodeStatus::SUCCESS;
            }
        }
};

class CheckWSFree : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        CheckWSFree(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~CheckWSFree() override = default;      
        virtual BT::NodeStatus tick() override
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
                if (!(manipulation_.service_client_free).call(srv_free))
                {
                    (manipulation_.rtde)->joint_target(manipulation_.array_scan_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                    if (!(manipulation_.service_client_free).call(srv_free))
                    {
                        (manipulation_.rtde)->joint_target(manipulation_.array_scan_right, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                        if (!(manipulation_.service_client_free).call(srv_free))
                        {
                            (manipulation_.rtde)->joint_target(manipulation_.array_scan_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);                   
                            if(!(manipulation_.service_client_free).call(srv_free))
                            {
                                return BT::NodeStatus::FAILURE;
                            }
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
};

class DropObj : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        DropObj(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~DropObj() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("drop obj"); 
            ROS_INFO("Move to grasping_point + 7cm");
            /*
            (manipulation_.ur)->cart_quat(manipulation_.get_grasping_point().position.x, manipulation_.get_grasping_point().position.y, manipulation_.get_grasping_point().position.z + 0.07,
            manipulation_.get_grasping_point().orientation.x, manipulation_.get_grasping_point().orientation.y, manipulation_.get_grasping_point().orientation.z,
            manipulation_.get_grasping_point().orientation.w, ::move_duration);
            ROS_INFO("Move to grasping_point");

            (manipulation_.ur)->cart_quat(manipulation_.get_grasping_point().position.x, manipulation_.get_grasping_point().position.y, manipulation_.get_grasping_point().position.z,
            manipulation_.get_grasping_point().orientation.x, manipulation_.get_grasping_point().orientation.y, manipulation_.get_grasping_point().orientation.z,
            manipulation_.get_grasping_point().orientation.w, 7);

            ros::Duration(0.5).sleep();
            if (manipulation_.get_collision() == true)
            {
                tf2_ros::Buffer tfBuffer;
                tf2_ros::TransformListener tfListener(tfBuffer);
                ros::Duration(0.5).sleep();
                geometry_msgs::TransformStamped transformStamped;

                try
                {
                    transformStamped = tfBuffer.lookupTransform("base", "tool0_controller", ros::Time(0));
                    std::cout << "transformStamped: " << transformStamped << std::endl;
                    (manipulation_.ur)->cart_quat(transformStamped.transform.translation.x, transformStamped.transform.translation.y, transformStamped.transform.translation.z + 0.002,
                                 transformStamped.transform.rotation.x, transformStamped.transform.rotation.y, transformStamped.transform.rotation.z,
                                 transformStamped.transform.rotation.w, 2);
                }
                catch (tf2::TransformException &ex)
                {
                    ROS_INFO("tf base2cam error");
                    ROS_WARN("%s", ex.what());
                    ros::Duration(1.0).sleep();
                }
                ROS_INFO("collision_detected");
                manipulation_.set_collision(false);
            }
            ROS_INFO("no collision_detected");
            (manipulation_.rtde)->gripper_open(gripper_speed_, gripper_force_);
            (manipulation_.ur)->cart_quat(manipulation_.get_grasping_point().position.x, manipulation_.get_grasping_point().position.y, manipulation_.get_grasping_point().position.z + 0.07,
                     manipulation_.get_grasping_point().orientation.x, manipulation_.get_grasping_point().orientation.y, manipulation_.get_grasping_point().orientation.z,
                     manipulation_.get_grasping_point().orientation.w, ::move_duration);
            */       
            return BT::NodeStatus::SUCCESS;
        }
};



class MoveDown : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        MoveDown(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~MoveDown() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("get grasp and move grasp");
            double left_left_thresh = 0.2;
            double left_thresh = 0.1;
            double right_thresh = -0.1;
            double right_right_thresh = -0.2;
            
            if (manipulation_.get_grasping_point().position.y >= left_left_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("left_left");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < left_left_thresh && manipulation_.get_grasping_point().position.y >= left_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("left");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < left_thresh && manipulation_.get_grasping_point().position.y >= right_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("mid");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < right_thresh && manipulation_.get_grasping_point().position.y >= right_right_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_right, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("right");
                return BT::NodeStatus::SUCCESS;
            }
            else if (manipulation_.get_grasping_point().position.y < right_right_thresh)
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_pick_left, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
                manipulation_.set_grasping_area("right_right");
                return BT::NodeStatus::SUCCESS;
            }
        
            return BT::NodeStatus::FAILURE;  
        }
};



class MoveToDrivePose : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        MoveToDrivePose(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}) , manipulation_(manipulation){}
        ~MoveToDrivePose() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("move to drive pos");
            (manipulation_.rtde)->joint_target(manipulation_.array_drive, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            manipulation_.res_.status = "FINISHED";
            return BT::NodeStatus::SUCCESS;
        }
};

class MoveToDropPos : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        MoveToDropPos(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~MoveToDropPos() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("move to drop pos");
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate3, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate2, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate1, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            (manipulation_.rtde)->joint_target(manipulation_.array_scan_mid, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            return BT::NodeStatus::SUCCESS;
        }
};

class MoveToTray : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        MoveToTray(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~MoveToTray() override = default;      
        virtual BT::NodeStatus tick() override
        {
            ROS_INFO("move to tray");
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate3, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate2, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            (manipulation_.rtde)->joint_target(manipulation_.array_rotate1, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            if (manipulation_.get_tray() == "SAVE_1")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray1_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);    
            }
            else if (manipulation_.get_tray() == "SAVE_2")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray2_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);          
            }
            else
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray3_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_); 
            }       
            return BT::NodeStatus::SUCCESS;   
        }
};

class PickFromTray : public BT::SyncActionNode
{
    private:
        Manipulation& manipulation_;

    public:
        PickFromTray(const std::string& name, Manipulation& manipulation) : BT::SyncActionNode(name, {}), manipulation_(manipulation) {}
        ~PickFromTray() override = default;      
        virtual BT::NodeStatus tick() override
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
            /*
            if (manipulation_.get_collision() == true)
            {
                tf2_ros::Buffer tfBuffer;
                tf2_ros::TransformListener tfListener(tfBuffer);
                ros::Duration(0.5).sleep();
                geometry_msgs::TransformStamped transformStamped;

                try
                {
                    transformStamped = tfBuffer.lookupTransform("base", "tool0_controller", ros::Time(0));
                    std::cout << "transformStamped: " << transformStamped << std::endl;
                    (manipulation_.ur)->cart_quat(transformStamped.transform.translation.x, transformStamped.transform.translation.y, transformStamped.transform.translation.z + 0.002,
                                 transformStamped.transform.rotation.x, transformStamped.transform.rotation.y, transformStamped.transform.rotation.z,
                                 transformStamped.transform.rotation.w, 2);
                }
                catch (tf2::TransformException &ex)
                {
                    ROS_INFO("tf base2cam error");
                    ROS_WARN("%s", ex.what());
                    ros::Duration(1.0).sleep();
                }
                ROS_INFO("collision_detected");
                manipulation_.set_collision(false);
            }
            */
            (manipulation_.rtde)->gripper_close(manipulation_.gripper_speed_, manipulation_.gripper_force_);
            if (manipulation_.get_tray() == "SAVE_1")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray1_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_); 
            }
            else if (manipulation_.get_tray() == "SAVE_2")
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray2_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            }
            else
            {
                (manipulation_.rtde)->joint_target(manipulation_.array_tray3_top, manipulation_.jnt_vel_, manipulation_.jnt_acc_);
            }            
            return BT::NodeStatus::SUCCESS;
        }
};



void registerNodes(BT::BehaviorTreeFactory& factory, Manipulation& manipulation)
{
        
        BT::NodeBuilder builder_1 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<NotPick>(name,  std::ref(manipulation));};
        factory.registerBuilder<NotPick>("NotPick", builder_1);

        BT::NodeBuilder builder_2 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<NotDrive>(name,  std::ref(manipulation));};
        factory.registerBuilder<NotDrive>("NotDrive", builder_2);

        BT::NodeBuilder builder_3 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<NotPlace>(name,  std::ref(manipulation));};
        factory.registerBuilder<NotPlace>("NotPlace", builder_3);        
        
        BT::NodeBuilder builder_4 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<NotPP>(name,  std::ref(manipulation));};
        factory.registerBuilder<NotPP>("NotPP", builder_4);
        
        BT::NodeBuilder builder_5 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<NotWS>(name,  std::ref(manipulation));};
        factory.registerBuilder<NotWS>("NotWS", builder_5);

        BT::NodeBuilder builder_6 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<NotSH>(name,  std::ref(manipulation));};
        factory.registerBuilder<NotSH>("NotSH", builder_6);

        BT::NodeBuilder builder_7 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<NotTT>(name,  std::ref(manipulation));};
        factory.registerBuilder<NotTT>("NotTT", builder_7);

        BT::NodeBuilder builder_8 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<CheckObjRequired>(name,  std::ref(manipulation));};
        factory.registerBuilder<CheckObjRequired>("CheckObjRequired", builder_8);

        BT::NodeBuilder builder_9 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<CheckObjectPicked>(name, std::ref(manipulation));};
        factory.registerBuilder<CheckObjectPicked>("CheckObjectPicked", builder_9);

        BT::NodeBuilder builder_10 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<CheckWSFree>(name, std::ref(manipulation));};
        factory.registerBuilder<CheckWSFree>("CheckWSFree", builder_10);

        BT::NodeBuilder builder_11 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<DropObjectInTray>(name,  std::ref(manipulation));};
        factory.registerBuilder<DropObjectInTray>("DropObjectInTray", builder_11);

        BT::NodeBuilder builder_12 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<DropObj>(name,  std::ref(manipulation));};
        factory.registerBuilder<DropObj>("DropObj", builder_12);

        BT::NodeBuilder builder_13 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<GetGraspAndMoveGrasp>(name,  std::ref(manipulation));};
        factory.registerBuilder<GetGraspAndMoveGrasp>("GetGraspAndMoveGrasp", builder_13);

        BT::NodeBuilder builder_14 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<MoveDown>(name,  std::ref(manipulation));};
        factory.registerBuilder<MoveDown>("MoveDown", builder_14);

        BT::NodeBuilder builder_15 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<MoveHomePos>(name,  std::ref(manipulation));};
        factory.registerBuilder<MoveHomePos>("MoveHomePos", builder_15);

        BT::NodeBuilder builder_16 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<MoveToDrivePose>(name,  std::ref(manipulation));};
        factory.registerBuilder<MoveToDrivePose>("MoveToDrivePose", builder_16);

        BT::NodeBuilder builder_17 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<MoveToDropPos>(name,  std::ref(manipulation));};
        factory.registerBuilder<MoveToDropPos>("MoveToDropPos", builder_17); 
        
        BT::NodeBuilder builder_18 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<MoveToScan>(name,  std::ref(manipulation));};
        factory.registerBuilder<MoveToScan>("MoveToScan", builder_18);

        BT::NodeBuilder builder_19 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<MoveToTray>(name,  std::ref(manipulation));};
        factory.registerBuilder<MoveToTray>("MoveToTray", builder_19);

        BT::NodeBuilder builder_20 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<MoveUp>(name,  std::ref(manipulation));};       
        factory.registerBuilder<MoveUp>("MoveUp", builder_20);

        BT::NodeBuilder builder_21 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<PickFromTray>(name,  std::ref(manipulation));};
        factory.registerBuilder<PickFromTray>("PickFromTray", builder_21);

        BT::NodeBuilder builder_22 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<PickObject>(name,  std::ref(manipulation));};
        factory.registerBuilder<PickObject>("PickObject", builder_22); 

        BT::NodeBuilder builder_23 = [&](const std::string& name, const BT::NodeConfiguration& config) {return std::make_unique<ScanWorkSpace>(name,  std::ref(manipulation));};       
        factory.registerBuilder<ScanWorkSpace>("ScanWorkSpace", builder_23);

}

int main(int argc, char **argv)
{

    ros::init(argc, argv, "swot_manipulation_bt_joel");

    Manipulation manipulation;
    ros::MultiThreadedSpinner spinner(8);
    spinner.spin();
    
    return 0;
}
