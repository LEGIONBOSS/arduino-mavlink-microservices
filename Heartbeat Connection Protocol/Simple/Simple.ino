/*
    File:    Simple.ino
    Created: 2025.08.22. 13:11:33
    Author:  Daniel Szilagyi
*/

#include "mavlink2/common/mavlink.h" // https://github.com/mavlink/c_library_v2

#define DEBUG_SERIAL Serial
#define MAVLINK_SERIAL Serial1

const uint32_t debug_baud = 115200;
const uint32_t mavlink_baud = 57600;
const uint32_t heartbeat_timer = 1000; // ms
const uint8_t compid = MAV_COMP_ID_USER1;

uint32_t heartbeat_timer_last = 0;
uint32_t now = 0;
uint8_t sysid = 1;
bool sysid_set = false;

void mavlink_read(void);
void mavlink_send_heartbeat(void);

void setup(void)
{
    MAVLINK_SERIAL.begin(mavlink_baud);

#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.begin(debug_baud);
    DEBUG_SERIAL.println("setup");
#endif
}

void loop(void)
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println("loop");
#endif

    while (true)
    {
        // Read and process incoming MAVLink
        mavlink_read();

        // Save current time
        now = millis();

        // Send scheduled MAVLink heartbeat
        if (sysid_set && now - heartbeat_timer_last >= heartbeat_timer)
        {
            mavlink_send_heartbeat();
            heartbeat_timer_last = now;
        }
    }
}

void mavlink_read(void)
{
    mavlink_status_t status;
    mavlink_message_t msg;
    uint8_t chan = MAVLINK_COMM_0;

    while (MAVLINK_SERIAL.available() > 0)
    {
        uint8_t byte = MAVLINK_SERIAL.read();
        if (mavlink_parse_char(chan, byte, &msg, &status))
        {
            switch (msg.msgid)
            {
                case MAVLINK_MSG_ID_HEARTBEAT:
                {
                    // Set sysid to the same as the autopilot
                    if (msg.compid == MAV_COMP_ID_AUTOPILOT1 && (!sysid_set || sysid != msg.sysid))
                    {
                        sysid = msg.sysid;
                        sysid_set = true;

#ifdef DEBUG_SERIAL
                        DEBUG_SERIAL.print("sysid set to ");
                        DEBUG_SERIAL.println(sysid);
#endif
                    }

#ifdef DEBUG_SERIAL
                    DEBUG_SERIAL.println("MAVLink heartbeat in");
#endif
                }
                break;

                default: break;
            }
        }
    }
}

void mavlink_send_heartbeat(void)
{
    mavlink_message_t msg;

    mavlink_msg_heartbeat_pack(sysid,								// system_id
                               compid,								// component_id
                               &msg,								// msg pointer
                               MAV_TYPE_ONBOARD_CONTROLLER,			// type
                               MAV_AUTOPILOT_INVALID,				// autopilot
                               MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,	// base_mode
                               0,									// custom_mode
                               MAV_STATE_ACTIVE);					// system_status

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    MAVLINK_SERIAL.write(buf, len);

#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println("MAVLink heartbeat out");
#endif
}
