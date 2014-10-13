/*
 * devduino_sensor_node.ino - Firmware for DevDuino v2.0 based sensor Node with nRF24L01+ module (with RF24SNP lib)
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
 * DevDuino v2.0 - http://www.seeedstudio.com/wiki/DevDuino_Sensor_Node_V2.0_(ATmega_328)
 * nRF24L01+ spec - https://www.sparkfun.com/datasheets/Wireless/Nordic/nRF24L01P_Product_Specification_1_0.pdf
 */

#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <RF24SNP.h>
#include <stdio.h>
#include <printf.h>
#include <dht.h>

/***************************************/
/********* NETWORK NODE CONFIG *********/
/***************************************/
#define NETWORK_CHANNEL 1
#define NODE_ADDRESS 0


/***********************************/
/********* PIN DEFINITIONS *********/
/***********************************/
#define LED_pin 9
#define DHT22_pin 3
#define RF24_CE_pin 8
#define RF24_CS_pin 7
#define MCP9700_pin A3


/*****************************/
/********* FUNCTIONS *********/
/*****************************/
float readDHT22Temp();
float readDHT22Hum();
float readMCP9700Temp();
uint16_t readVcc();


/************************************/
/********* GLOBAL VARIABLES *********/
/************************************/
// RF24 radio(CE pin, CS pin) object
RF24 radio(RF24_CE_pin, RF24_CS_pin);
RF24Network network(radio);
RF24SNPNode node(network, 3000, readVcc);

/**********************************/
/********* IMPLEMENTATION *********/
/**********************************/
void setup() {
  Serial.begin(9600);
  printf_begin();

  SPI.begin();
  radio.begin();
  network.begin(NETWORK_CHANNEL, NODE_L1_ADDRESS[NODE_ADDRESS]);

  // setup sensors
  (void)node.add_sensor(MCP9700_TEMP, readMCP9700Temp);
  (void)node.add_sensor(DHT22_TEMP, readDHT22Temp);
  (void)node.add_sensor(DHT22_HUM, readDHT22Hum);
  
  radio.printDetails();

  pinMode(LED_pin, OUTPUT);
}

void loop() {
  bool ret;
  digitalWrite(LED_pin, HIGH);
  radio.powerUp();
  network.update();
  
  Serial.println();
  Serial.println("Sending HELLO...");
  /* Send HELLO message to the server */
  node.do_hello(10 /* retries */, 2000 /* delay between retries */);
  
  digitalWrite(LED_pin, LOW);
  Serial.println("Doing work...");
  
  /* Wait for QUERY and SLEEP messages and do the work! */
  ret = node.do_work(40 /* retries */, 250 /* delay between retries */);
  
  if (ret == true)
    Serial.println("Received SLEEP msg, going to sleep");
  else
    Serial.println("Didn't receive SLEEP msg, going to sleep for defualt time");

  Serial.print("Going to sleep for ms ... ");
  Serial.println(node.get_sleep_time());

  radio.powerDown();
  node.sleep();
}

/**
 * Read the temperature from DHT22 sensor using DHTLib.
 */
float readDHT22Temp() {
  static dht sensor;
  
  delay(400);
  sensor.read22(DHT22_pin);
  Serial.print("Read Temp from RHT22 = ");
  Serial.println(sensor.temperature);
  return (float)sensor.temperature;
}

/**
 * Read the humidity from DHT22 sensor using DHTLib.
 */
float readDHT22Hum() {
  static dht sensor;
  
  delay(400);
  sensor.read22(DHT22_pin);
  Serial.print("Read Hum from RHT22 = ");
  Serial.println(sensor.humidity);
  return (float)sensor.humidity;
}

/**
 * Read the temperature from MCP9700
 */
float readMCP9700Temp() {
  float temp = analogRead(MCP9700_pin)*3.3/1024.0;
  temp = temp - 0.5;
  temp = temp / 0.01;
  Serial.print("Read Temp from MCP9700 = ");
  Serial.println(temp);
  return temp;
}

/**
 * Measure remaining voltage in battery in millivolts
 *
 * From http://www.seeedstudio.com/wiki/DevDuino_Sensor_Node_V2.0_(ATmega_328)#Measurement_voltage_power
 */
uint16_t readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(75); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return (int)result; // Vcc in millivolts
}
