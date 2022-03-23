#include <Arduino.h>
#include <stdio.h>

#define LED_PIN 13

// ---- ROS ----------------------------------------------------------------------------------
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/string.h>
#include <std_msgs/msg/byte_multi_array.h>
#include <rcutils/allocator.h>
#define STRING_BUFFER_LEN 50
rcl_node_t microros_node;
rclc_support_t microros_support;
rcl_allocator_t microros_allocator;
rcl_publisher_t int_publisher;
rcl_publisher_t can_publisher;
rcl_publisher_t can_log_send;
rcl_publisher_t can_log_recieve;
std_msgs__msg__Int32 int_msg;
std_msgs__msg__String string_msg;
std_msgs__msg__ByteMultiArray can_array;


auto string2ros(const char * data) {
  const rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rosidl_runtime_c__String ret;
  ret.size = strlen(data);
  ret.capacity = ret.size + 1;
  ret.data = allocator.allocate(ret.capacity, allocator.state);
  memset(ret.data, 0, ret.capacity);
  memcpy(ret.data, data, ret.size);
  return ret;
}

void initialize_ros(){
    set_microros_transports();
    delay(2000);
    // create node
    microros_allocator = rcl_get_default_allocator();
    rclc_support_init(&microros_support, 0, NULL, &microros_allocator);
    rclc_node_init_default(&microros_node, "microros_log", "", &microros_support);

    //createe publishers
    rclc_publisher_init_default(&int_publisher, &microros_node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "int_publisher");
    rclc_publisher_init_default(&can_log_send, &microros_node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String), "can_log_send");
    rclc_publisher_init_default(&can_log_recieve, &microros_node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String), "can_log_recieve");

    can_array.data.capacity = 100;
    can_array.data.size = 8;
    rclc_publisher_init_default(&can_publisher, &microros_node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, ByteMultiArray), "can_array");
}

void execute_ros(){
    rcl_publish(&int_publisher, &int_msg, NULL);
}

// ---- CAN ----------------------------------------------------------------------------------
#include <ACAN_T4.h>
CANMessage can_frame ;

void initialize_can(){
    ACAN_T4_Settings settings(250 * 1000); // 250 kbit/s
    settings.mLoopBackMode = true;
    settings.mSelfReceptionMode = true;
    const uint32_t errorCode = ACAN_T4::can1.begin(settings);
    if (0 != errorCode) {
        Serial.println ("Invalid setting");
    }
    can_frame.id = 0x542;
    for (int i = 0; i < 8; i++) { 
        can_frame.data[i] = 0x00;
    }
}
void execute_can(){
    const bool cansend_ok = ACAN_T4::can1.tryToSend(can_frame);
    if (cansend_ok) {
        int_msg.data += 1;
        string_msg.data = string2ros("SENT");
        rcl_publish(&can_log_send, &string_msg, NULL);
        can_array.data.data = can_frame.data;
        rcl_publish(&can_publisher, &can_array, NULL);
        for (int i = 0; i < 8; i++) { Serial.println(can_array.data.data[i]); }
    }
    // if (ACAN_T4::can1.receive (can_frame)) {
    //     string_msg.data = micro_ros_string_utilities_init("RECIEVED");
    //     rcl_publish(&can_log_recieve, &string_msg, NULL);
    // }
}

// ---- ExpressLRS----------------------------------------------------------------------------
#include <CrsfSerial.h>
CrsfSerial crsf(Serial1, CRSF_BAUDRATE);


// ---- SETUP --------------------------------------------------------------------------------
void setup () {
    pinMode(LED_PIN, OUTPUT) ;
    Serial.begin (9600);
    while (!Serial) { 
        delay (50); 
        digitalWrite(LED_BUILTIN, !digitalRead (LED_BUILTIN));
    }
    initialize_can();
    initialize_ros();
}
void loop () {
    delay(200);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    execute_can();
    execute_ros();
}
