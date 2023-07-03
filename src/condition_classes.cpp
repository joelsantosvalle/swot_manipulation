/**
*       place.cpp
*
*       @date 14.07.2023
*       @author Joel Santos
*/

//--------------------------------------//

/**
 *      @class NotDrive
 *      @brief Condition node that checks if the requested mode is not "DRIVE".
 */

NotDrive::NotDrive(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) 
{

}

NotDrive::~NotDrive() override = default;

/**
 *      @brief Executes the tick operation of the node NotDrive.
 *      @return The execution status of the node.
 */

BT::NodeStatus NotDrive::tick() override
{
    ROS_INFO("drive achieved");
    if(manipulation_.get_request().mode == "DRIVE") {return BT::NodeStatus::FAILURE;}
    else {return BT::NodeStatus::SUCCESS;}
}

//--------------------------------------//

/**
 *      @class NotPick
 *      @brief Condition node that checks if the requested mode is not "PICK".
 */

NotPick::NotPick(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) 
{

}

NotPick::~NotPick() override = default;

/**
 *      @brief Executes the tick operation of the node NotPick.
 *      @return The execution status of the node.
 */

BT::NodeStatus NotPick::tick() override
{
    ROS_INFO("pick achieved");
    if(manipulation_.get_request().mode == "PICK") {return BT::NodeStatus::FAILURE;}
    else {return BT::NodeStatus::SUCCESS;}
}

//--------------------------------------//

/**
 *      @class NotPlace
 *      @brief Condition node that checks if the requested mode is not "PLACE".
 */

NotPlace::NotPlace(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) 
{

}

NotPlace::~NotPlace() override = default;

/**
 *      @brief Executes the tick operation of the node NotPlace.
 *      @return The execution status of the node.
 */

BT::NodeStatus NotPlace::tick() override
{
    ROS_INFO("place achieved");
    if(manipulation_.get_request().mode == "PLACE") {return BT::NodeStatus::FAILURE;}
    else {return BT::NodeStatus::SUCCESS;}
} 

//--------------------------------------//

/**
 *      @class NotPP
 *      @brief Condition node that checks if the requested task is not "PP".
 */

NotPP::NotPP(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation)
{

}

NotPP::~NotPP() override = default;

/**
 *      @brief Executes the tick operation of the node NotPP.
 *      @return The execution status of the node.
 */

BT::NodeStatus NotPP::tick() override
{
    ROS_INFO("pp achieved");
    if(manipulation_.get_request().task == "PP") {return BT::NodeStatus::FAILURE;}
    else {return BT::NodeStatus::SUCCESS;}
}

//--------------------------------------//

/**
 *      @class NotSH
 *      @brief Condition node that checks if the requested task is not "SH".
 */
NotSH::NotSH(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) 
{

}

NotSH::~NotSH() override = default;

/**
 *      @brief Executes the tick operation of the node NotSH.
 *      @return The execution status of the node.
 */

BT::NodeStatus NotSH::tick() override
{
    ROS_INFO("sh achieved");
    if(manipulation_.get_request().task == "SH") {return BT::NodeStatus::FAILURE;}
    else {return BT::NodeStatus::SUCCESS;}
}

//--------------------------------------//

/**
 *      @class NotTT
 *      @brief Condition node that checks if the requested task is not "TT".
 */
NotTT::NotTT(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) 
{

}

NotTT::~NotTT() override = default;

/**
 *      @brief Executes the tick operation of the node NotTT.
 *      @return The execution status of the node.
 */

BT::NodeStatus NotTT::tick() override
{
    ROS_INFO("tt achieved");
    if(manipulation_.get_request().task == "TT") {return BT::NodeStatus::FAILURE;}
    else {return BT::NodeStatus::SUCCESS;}
}

//--------------------------------------//

/**
 *      @class NotWS
 *      @brief Condition node that checks if the requested task is not "WS".
 */
NotWS::NotWS(const std::string& name, Manipulation& manipulation) : BT::ConditionNode(name, {}), manipulation_(manipulation) 
{

}

NotWS::~NotWS() override = default;

/**
 *      @brief Executes the tick operation of the node NotWS.
 *      @return The execution status of the node.
 */

BT::NodeStatus NotWS::tick() override
{
    ROS_INFO("ws achieved");
    if(manipulation_.get_request().task == "WS") {return BT::NodeStatus::FAILURE;}
    else {return BT::NodeStatus::SUCCESS;}
}
