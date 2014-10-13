/*
 * RF24SNP.cpp - Sensor Node Protocol (SNP) based on RF24 and RF24Network libraries
 *
 * Copyright 2014 Tomas Hozza <thozza@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 * Tomas Hozza <thozza@gmail.com>
 *
 * RF24 - http://tmrh20.github.io/RF24/
 * RF24Network - http://tmrh20.github.io/RF24Network/
 * nRF24L01+ spec - https://www.sparkfun.com/datasheets/Wireless/Nordic/nRF24L01P_Product_Specification_1_0.pdf
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <cstring>
#include "RF24SNP.h"

/**
 * Add new sensor available on the node
 */
bool RF24SNPNode::add_sensor(SensorType type, get_value_cb value_fn)
{
    if(sensors.size() == MAX_SENSORS_CNT)
        return false;
    else
        sensors.push_back(SensorCTX(type, value_fn));
    return true;
}

/**
 * Handle the HELLO message sending.
 */
void RF24SNPNode::do_hello(uint8_t retries, uint8_t interval)
{
    for (uint8_t i=0; i<retries; i++)
    {
        send_hello();
        network.update();
        if (network.available())
            break;
        delay(interval);
    }
}

/**
 * Handle QUERY and SLEEP messages.
 */
bool RF24SNPNode::do_work(uint8_t retries, uint8_t interval)
{
    RF24NetworkHeader header;
    for (uint8_t i=0; i<retries; i++)
    {
        network.update();
        // process all the messages
        while (network.available())
        {
            network.peek(header);
            // some bogus message
            if (header.from_node != SERVER_ADDRESS)
            {
                network.read(header, 0, 0);
                network.update();
                continue;
            }

            // handle the message from server
            switch (header.type) {
                case RF24SNP_MSG_TYPE_QUERY:
                    handle_query_msg();
                    break;

                case RF24SNP_MSG_TYPE_SLEEP:
                    handle_sleep_msg();
                    return true;

                // some bogus message
                default:
                    network.read(header, 0, 0);
            }
            network.update();
        }
        delay(interval);
    }
    return false;
}

/**
 * Sleep the node
 */
void RF24SNPNode::sleep()
{
    delay(sleep_time);
}

/**
 * Send HELLO message to the server
 */
void RF24SNPNode::send_hello()
{
    RF24NetworkHeader header(SERVER_ADDRESS, RF24SNP_MSG_TYPE_HELLO);
    node_state_t state;
    uint8_t i;

    if (voltage_cb != NULL)
        state.vcc = (*voltage_cb)();
    else
        state.vcc = 0;

    state.sensors_cnt = sensors.size();

    std::list<SensorCTX>::iterator it;
    for (i=0, it=sensors.begin(); i < state.sensors_cnt && it != sensors.end(); i++, it++)
    {
        state.sensors[i] = it->sensor;
    }

    network.write(header, &state, sizeof(node_state_t)); 
}

/**
 * Send requested value
 */
void RF24SNPNode::send_value(SensorType sensor)
{
    RF24NetworkHeader header(SERVER_ADDRESS, RF24SNP_MSG_TYPE_REPLY);
    measurement_value_t payload;

    std::list<SensorCTX>::iterator it;
    for (it=sensors.begin(); it != sensors.end(); it++)
    {
        memset(&payload, 0, sizeof(payload));

        if (it->sensor == sensor)
        {
            payload.sensor = sensor;
            payload.value = (it->value)();
            network.write(header, &payload, sizeof(payload));
            network.update();
        }
    }
}

/**
 * Handle a value QUERY from server and send back the value
 */
void RF24SNPNode::handle_query_msg()
{
    RF24NetworkHeader header;
    measurement_value_t payload;
    
    network.read(header, &payload, sizeof(payload));
    send_value(payload.sensor);
}

/**
 * Handle sleep message from the server
 */
void RF24SNPNode::handle_sleep_msg()
{
    RF24NetworkHeader header;
    sleep_t sleep;
    
    network.read(header, &sleep.time, sizeof(sleep.time));
    sleep_time = sleep.time;
}

/**
 * Constructor for class representing sensor node on server side
 */
SensorNode::SensorNode(RF24NetworkHeader& header, node_state_t& state)
{
    address = header.from_node;
    vcc = state.vcc;
    sensors_cnt = state.sensors_cnt;
    //sensors = state.sensors;
    //std::list<SensorCTX>::iterator it;
    uint8_t i;    
//for (it = state.sensors.begin(); it != state.sensors.end(); it++)
    for (i = 0; i<sensors_cnt; i++)
    {
        sensors[i] = (state.sensors[i]);
    }

}

uint8_t RF24SNPServer::check_messages()
{
    RF24NetworkHeader header;
    for (;;)
    {
        network.update();
        // process all the messages
        while (network.available())
        {
            network.peek(header);
            // handle the message from server
            switch (header.type) {
                case RF24SNP_MSG_TYPE_HELLO:
                case RF24SNP_MSG_TYPE_REPLY:
                    return header.type;

                // some bogus message
                default:
                    network.read(header, 0, 0);
            }
            network.update();
        }
    }
}

/**
 * Handle HELLO message from the node.
 */
SensorNode* RF24SNPServer::handle_hello_msg()
{
    RF24NetworkHeader header;
    SensorNode* node = NULL;
    node_state_t state;

    network.read(header, &state, sizeof(state));

    node = new SensorNode(header, state);

    return node;
}

/**
 * Handle REPLY message from the node.
 */
void RF24SNPServer::handle_value_msg(uint16_t* from_address, measurement_value_t* value)
{
    RF24NetworkHeader header;

    network.read(header, value, sizeof(*value));
    *from_address = header.from_node;
}

/**
 * Send QUERY message to the node.
 */
void RF24SNPServer::send_query(SensorNode& node, SensorType sensor)
{
    RF24NetworkHeader header(node.address, RF24SNP_MSG_TYPE_QUERY);
    measurement_value_t payload;

    payload.sensor = sensor;
    payload.value = 0.0;

    network.write(header, &payload, sizeof(payload));
}

/**
 * Send SLEEP message to the node.
 */
void RF24SNPServer::send_sleep(SensorNode& node, uint16_t sleep_time)
{
    RF24NetworkHeader header(node.address, RF24SNP_MSG_TYPE_SLEEP);
    sleep_t sleep;

    sleep.time = sleep_time;

    network.write(header, &sleep, sizeof(sleep));
}
