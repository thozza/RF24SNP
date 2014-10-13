#############################################################################
#
# Makefile for Sensor Node server using RF24SNP on Raspberry Pi
#
# License: GPL v2 (General Public License)
# Author:  Tomas Hozza <thozza@gmail.com>
# Date:    2014/09/13 (version 1.0)
#

CCFLAGS=-Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
LDFLAGS=-lrf24-bcm -lrf24network -lrf24snp
CC=g++

BINARY=sensor-node-server

all: ${BINARY}

${BINARY}: main.o
	$(CC) $(CCFLAGS) $(LDFLAGS) main.o -o ${BINARY}

main.o: main.c
	$(CC) -c $(CCFLAGS) main.c

clean:
	rm -rf *.o
	rm -rf ${BINARY}	
