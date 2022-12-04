/* mros2 example
 * Copyright (c) 2021 smorita_emb
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "mros2.h"
#include "tf2_msgs/msg/t_fmessage.hpp"
#include "std_msgs/msg/header.hpp"
#include "EthernetInterface.h"

#define IP_ADDRESS ("192.168.0.2")      /* IP address */
#define SUBNET_MASK ("255.255.255.0")   /* Subnet mask */
#define DEFAULT_GATEWAY ("192.168.0.1") /* Default gateway */

typedef struct
{
  uint32_t secs; // Transmit Time-stamp seconds.
} ntp_packet;

int main()
{
  EthernetInterface network;
  network.set_dhcp(false);
  network.set_network(IP_ADDRESS, SUBNET_MASK, DEFAULT_GATEWAY);
  nsapi_size_or_error_t result = network.connect();

  // 現在時刻を取得
  SocketAddress sockAddr;
  network.gethostbyname("time.nist.gov", &sockAddr);
  sockAddr.set_port(37);
  ntp_packet in_data;
  UDPSocket sock;
  sock.open(&network);
  char out_buffer[] = "time";
  if (0 > sock.sendto(sockAddr, out_buffer, sizeof(out_buffer)))
  {
    printf("Error sending data\n");
    return -1;
  }
  if (sock.recvfrom(&sockAddr, &in_data, sizeof(ntp_packet)) > 0)
  {
    in_data.secs = ntohl(in_data.secs) - 2208988800; // 1900-1970
  }
  sock.close();

  Timer stm_clock;
  stm_clock.reset();
  stm_clock.start();

  printf("mbed mros2 start!\r\n");
  printf("app name: pub_tf\r\n");
  mros2::init(0, NULL);
  MROS2_DEBUG("mROS 2 initialization is completed\r\n");

  mros2::Node node = mros2::Node::create_node("mros2_node");
  mros2::Publisher pub = node.create_publisher<tf2_msgs::msg::TFMessage>("tf", 10);
  osDelay(100);
  MROS2_INFO("ready to pub/sub message\r\n");
  int publish_count = 0;

  while (1)
  {
    // 必ずループごとにインスタンスを生成する
    // そうじゃないとエラーが起こる（理由不明、copyfrombufあたりがミスってる）
    tf2_msgs::msg::TFMessage tf;
    std::vector<geometry_msgs::msg::TransformStamped> trans_vec;
    trans_vec.resize(1);
    geometry_msgs::msg::TransformStamped transformstamped;
    geometry_msgs::msg::Transform transform;
    geometry_msgs::msg::Quaternion quat;
    geometry_msgs::msg::Vector3 vec;
    transformstamped.frame_id = "odom";
    transformstamped.child_frame_id = "base_link";
    vec.x = static_cast<double>(publish_count * 0.00001);
    vec.y = -static_cast<double>(publish_count * 0.00001);
    vec.z = 0.0;
    quat.x = 0.0;
    quat.y = 0.0;
    quat.z = static_cast<double>(publish_count * 0.00001);
    quat.w = static_cast<double>(publish_count * 0.00001);
    transform.rotation = quat;
    transform.translation = vec;
    transformstamped.transform = transform;
    const int usec = stm_clock.read_us();
    const int sec = static_cast<int>(usec * 0.000001);
    transformstamped.sec = in_data.secs + sec;
    transformstamped.nanosec = (usec - (sec * 1000000)) * 100;

    trans_vec.at(0) = transformstamped;
    tf.transforms = trans_vec;
    pub.publish(tf);
    osDelay(100);
    publish_count++;
  }

  mros2::spin();
  return 0;
}