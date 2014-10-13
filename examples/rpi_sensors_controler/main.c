/*
 * main.cpp - Sensor Node server for Raspberry Pi based on RF24SNP + RF24Network lib
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
 * RF24 - http://maniacbug.github.io/RF24/
 * RF24Network - http://tmrh20.github.io/RF24Network/
 * nRF24L01+ spec - https://www.sparkfun.com/datasheets/Wireless/Nordic/nRF24L01P_Product_Specification_1_0.pdf
 */

#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <RF24SNP.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define NETWORK_CHANNEL 1

int main(int argc, char **argv) {
	RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);
	RF24Network network(radio);
	RF24SNPServer server(network);

	uint8_t msg_type;
	int8_t req_sent = -1;
	SensorNode *node = NULL;
	SensorNode *new_node = NULL;
	uint16_t addr = 0;
	measurement_value_t value;

	radio.begin();
	network.begin(NETWORK_CHANNEL, SERVER_ADDRESS);

	radio.printDetails();

	srand(time(NULL));	

	while(1) {
		if (req_sent == 0) {
			uint16_t sleep = rand() % 3000 + 5000;
			printf("\nSending SLEEP for %d...\n", sleep);
			server.send_sleep(*node, sleep);
			req_sent--;
			delete(node);
			node = NULL;
		}

		msg_type = server.check_messages();
		
		switch(msg_type) {
			case RF24SNP_MSG_TYPE_HELLO:
				if (node != NULL) {
					new_node = server.handle_hello_msg();
					if (new_node->address == node->address) {
						printf("\nReceived HELLO from already registered node\n");
						delete(new_node);
						new_node = NULL;
						continue;
					}
					delete(node);
					node = new_node;
					new_node = NULL;
				}
				else {
					node = server.handle_hello_msg();
				}
				
				/* print received data && request values*/
				printf("\nReceived hello from node:\n");
				printf("addr=%d\tvcc=%d\tsensors_cnt=%d\n", node->address, node->vcc, node->sensors_cnt);
				for (int i = 0; i < node->sensors_cnt; i++) {
					printf("sensor %d. -> %d\n", i+1, node->sensors[i]);

					server.send_query(*node, node->sensors[i]);
					if (req_sent < 0) {
						req_sent = 0;
					}
					req_sent++;
				}
				printf("\n");
				break;

			case RF24SNP_MSG_TYPE_REPLY:
				addr = 0;
				memset(&value, 0, sizeof(value));
				server.handle_value_msg(&addr, &value);

				printf("\nReceived REPLY message from node:\n");
				printf("addr=%d\tsensor=%d\tvalue=%f\n\n", addr, value.sensor, value.value);

				req_sent--;
				break;

			default:
				printf("Bogus message");
				break;
		}
	}

	return 0;
}
