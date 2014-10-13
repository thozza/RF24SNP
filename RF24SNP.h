/*
 * RF24SNP.h - Sensor Node Protocol (SNP) based on RF24 and RF24Network libraries
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

#ifndef __RF24SNP_H__
#define __RF24SNP_H__

#ifdef ARDUINO
#include <RF24Network.h>
#else
#include <RF24Network/RF24Network.h>
#include <bcm2835.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <list>

#define RF24SNP_MSG_TYPE_HELLO  0
#define RF24SNP_MSG_TYPE_SLEEP  1
#define RF24SNP_MSG_TYPE_QUERY  2
#define RF24SNP_MSG_TYPE_REPLY  3

#define MAX_SENSORS_CNT 5
#define MAX_CHILD_NODES_CNT 5

const uint16_t SERVER_ADDRESS = 00;
const uint16_t NODE_L1_ADDRESS[MAX_CHILD_NODES_CNT] = {01, 02, 03, 04, 05};

typedef float (*get_value_cb)();
typedef uint16_t (*get_voltage_cb)();

enum class SensorType : uint8_t {
    NO_SENSOR,
    MCP9700_TEMP,
    DS18B20_TEMP,
    DHT22_TEMP,
    DHT22_HUM
};

typedef struct {
    uint16_t vcc;         // 2B
    uint8_t sensors_cnt;  // 1B
    SensorType sensors[MAX_SENSORS_CNT];  // 5*2 = 10B
} node_state_t;

typedef struct {
    SensorType sensor;    // 1B
    float value;          // 4B
} measurement_value_t;

typedef struct {
    uint16_t time;
} sleep_t;

/**
 * The sensor node logic implementation.
 */
class RF24SNPNode
{
public:
    RF24SNPNode(RF24Network& _network, uint16_t _sleep=1000, get_voltage_cb _voltage=NULL): network(_network), sleep_time(_sleep), voltage_cb(_voltage) {}

    bool add_sensor(SensorType type, get_value_cb value_fn);
    void do_hello(uint8_t retries=10, uint8_t interval=250);
    bool do_work(uint8_t retries=100, uint8_t interval=250);
    void sleep();

private:

class SensorCTX
{
public:
    SensorType sensor;
    get_value_cb value;

    SensorCTX(): sensor(SensorType::NO_SENSOR), value(NULL) {}
    SensorCTX(SensorType _sensor, get_value_cb _value_fn): sensor(_sensor), value(_value_fn) {}
};

private:
    RF24Network& network;
    std::list<SensorCTX> sensors;
    uint16_t sleep_time;
    get_voltage_cb voltage_cb;

    void send_hello();
    void send_value(SensorType sensor);

    void handle_query_msg();
    void handle_sleep_msg();
};


/**
 * Class representing a data about sensor node that advertise itself using HELLO message
 */
class SensorNode
{
public:
    SensorNode(RF24NetworkHeader& header, node_state_t& state);

    uint16_t address;
    uint16_t vcc;
    uint8_t sensors_cnt;
    SensorType sensors[MAX_SENSORS_CNT];
};


/**
 * The sensor node's server logic implementation.
 */
class RF24SNPServer
{
public:
    RF24SNPServer(RF24Network& network): network(network) {}

    uint8_t check_messages();

    SensorNode* handle_hello_msg();
    void handle_value_msg(uint16_t* from_address, measurement_value_t* value);

    void send_query(SensorNode& node, SensorType sensor);
    void send_sleep(SensorNode& node, uint16_t sleep_time);

private:
    RF24Network& network;
};

#endif /* __RF24SNP_H__ */

