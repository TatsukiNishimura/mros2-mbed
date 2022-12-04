
#include "tf2_msgs/msg/t_fmessage.hpp"


template mros2::Publisher mros2::Node::create_publisher<tf2_msgs::msg::TFMessage>(std::string topic_name, int qos);
template void mros2::Publisher::publish(tf2_msgs::msg::TFMessage &msg);


