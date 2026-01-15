#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "path_remapper.h"

#define DEVICE_PATH "/dev/" DEVICE_NAME

int main(int argc, char **argv){
	int ret, fd, read_length;
	read_length = atoi(argv[2]);
	char *message = malloc(sizeof(char)*read_length);

	if (argc < 2){
		printf("Usage: %s [message to write] [read length]\n",argv[0]);
		return -1;
	}
	fd = open(DEVICE_PATH,O_RDWR);	
	if (fd < 0){
		printf("[path_remapper main] Failed to open device [%s]: %s\n", DEVICE_PATH, strerror(errno));
		return -1;
	}
	ret = write(fd,argv[1],strlen(argv[1]));
	if (ret < 0){
		printf("[path_remapper main] Failed to write to device [%s]: %s\n", DEVICE_PATH, strerror(errno));
		return -1;
	}
	ret = read(fd,message,read_length);
	if (ret < 0){
		printf("[path_remapper main] reading from the device [%s]: %s\n", DEVICE_PATH, strerror(errno));
		return -1;
	}
	printf("[path_remapper] read message from device ['%s']\n",message);
	return 0;
}
