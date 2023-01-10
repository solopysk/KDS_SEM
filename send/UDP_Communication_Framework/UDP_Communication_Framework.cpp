// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>
#include "md5.h"
#include <sstream>
#include <iostream>
#include "..//..//libcrc-master/src/crc16.c"


#define SENDER

#define TARGET_IP	"127.0.0.1"

#define TARGET_PORT 5555
#define LOCAL_PORT 8888


#define BUFFERS_LEN 1024


void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

bool recieve_ack(SOCKET socket, char *buffer_rx, sockaddr_in dest ) {
	fd_set readfds, masterfds;
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	FD_ZERO(&masterfds);
	FD_SET(socket, &masterfds);
	memcpy(&readfds, &masterfds, sizeof(fd_set));

	int destlen = sizeof(dest);
	memset(buffer_rx, 0, sizeof buffer_rx);
	printf("WAITING FOR ACK\n");

	if (select(socket + 1, &readfds, NULL, NULL, &timeout) < 0)
	{
		perror("on select");
		exit(1);
	}
	if (FD_ISSET(socket, &readfds))
	{
		if (recvfrom(socket, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&dest, &destlen) == SOCKET_ERROR) {
			printf("Socket error\n");
			getchar();
			exit(1);
		}
		else {
			printf("Received ACK\n");
			return 0;
		}
	}
	else
	{
		printf("Socket timeout\n");
		return 1;
	}
}
int add_crc(char* buffer,bool isData,int buffer_len) {
	char* str = (char*)calloc(1, sizeof(int));
	
	
	if (!isData) {
		int temp = crc_16((unsigned char*)(buffer), strlen(buffer));
		sprintf(str, "|%d", temp);
		strcat(buffer, str);
	}
	else {
		int temp = crc_16((unsigned char*)(buffer), buffer_len);
		sprintf(str, "|%d", temp);
		for (int i = 0; i < sizeof str;i++) {
			buffer[buffer_len + i ] = str[i];
		}
	}
	return sizeof str;
}


//**********************************************************************
int main()
{
	char* fname = "headlol.png";

	SOCKET socketS;

	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in from;


	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;


	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0) {
		printf("Binding error!\n");
		getchar(); //wait for pfile_sizes Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN * 2];
	char buffer_tx[BUFFERS_LEN * 2];
	char buffer[BUFFERS_LEN * 2];

	// ID=1 START
	// ID=2 NAME
	// ID=3 SIZE
	// ID=4 DATA
	// ID=5 HASH
	// ID=6 STOP
	//
	//
	sockaddr_in addrDest;
	FILE* fptr;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
	char name[100] = "..\\";
	strcat(name, fname);
	//sha.update(buaa,1);
	//uint8_t* digest = sha.digest();
	//std::cout << SHA256::toString(digest) << std::endl;

	//delete[] digest;
	if ((fptr = fopen(name, "rb")) == NULL) {
		printf("Error! opening file");

		// Program exits if the file pointer returns NULL.
		exit(1);
	}
	printf("Opened file succesfully\n");

	//SEND START
	memset(buffer, 0x00, sizeof buffer);
	snprintf(buffer, 10, "ID=%d", 1);
	add_crc(buffer, 0, 0);
	strncpy(buffer_tx, buffer, BUFFERS_LEN);
	printf("Sending START \n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);
	memset(buffer, 0x00, sizeof buffer);

	while (recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending START \n");
		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);
	}

	snprintf(buffer, BUFFERS_LEN, "ID=%d|%s", 2, fname);
	printf("Sending file name %s\n", fname);
	add_crc(buffer, 0, 0);
	strncpy(buffer_tx, buffer, BUFFERS_LEN);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);
	memset(buffer, 0x00, sizeof buffer);

	while (recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending file name\n");
		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);
	}

	//SEND SIZE OF FILE

	fseek(fptr, 0L, SEEK_END);
	long int file_size = ftell(fptr) / sizeof(char);
	fseek(fptr, 0L, SEEK_SET);
	int fake_hotel = snprintf(buffer, BUFFERS_LEN, "ID=%d|%d", 3, file_size);
	printf("Sending file size %dB\n", file_size);
	add_crc(buffer, 0, 0);
	strncpy(buffer_tx, buffer, BUFFERS_LEN);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));

	while (recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending file size %dB\n", file_size);
		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);
	}

	memset(buffer_tx, 0, sizeof buffer_tx);
	memset(buffer, 0x00, sizeof buffer);

	//START SENDING FILE DATA

	long data_pointer = 0;
	long bytes_sent = 0;
	bool cont = true;
	int iter = 0;
	while (cont) {
		iter++;
		char* data_buffer = (char*)calloc(BUFFERS_LEN - 31, sizeof(char));
		int bytes_read = fread(data_buffer, sizeof(char), 992, fptr);
		int max_index = 0;
		bool isContinue = true;

		snprintf(buffer, BUFFERS_LEN, "ID=%d|%d|", 4, bytes_read);
		int curr_length = 0;
		while (buffer[curr_length] != 0) {
			curr_length++;
		}
		// WRITE FULL DATA BUFFER TO BUFFER
		for (int i = 0;i < bytes_read;i++) {
			buffer[i + curr_length] = data_buffer[i];
		}
		int added = add_crc(buffer, true, curr_length + bytes_read);
		for (int i = 0;i < curr_length + bytes_read + added - 1;i++) {
			buffer_tx[i] = buffer[i];
		}

		int sent = sendto(socketS, buffer_tx, curr_length + bytes_read + added - 2, 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Sending DATA, SIZE: %d bytes\n", bytes_read, buffer_tx);

		

		while (recieve_ack(socketS, buffer_rx, addrDest)) {
			sendto(socketS, buffer_tx, curr_length + bytes_read + added - 2, 0, (sockaddr*)&addrDest, sizeof(addrDest));
			printf("Resending DATA, SIZE %d bytes\n", max_index);
			memset(buffer_tx, 0, sizeof buffer_tx);
			memset(buffer, 0x00, sizeof buffer);
		}

		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);

		bytes_sent += bytes_read;

		if (bytes_read == 0) {
			cont = false;
		}
		if (iter > 2) {
			//cont = false;
		}
		free(data_buffer);
	}
	//SEND STOP TO FINISH
	printf("Sent total of %ldB\n", bytes_sent);
	fclose(fptr);
	FILE* pepe_file = fopen(name, "rb");
	char* moje_souborove_pole = (char*)calloc(file_size, sizeof(char));
	// Pole alokovano
	int byte_read = fread(moje_souborove_pole, sizeof(char), file_size, pepe_file);

	if (byte_read != file_size) {
		std::cout << byte_read << " " << file_size << std::endl;
		printf("Cannot read file to compute hash, bye.\n");
	}
	else {
		std::string hash = md5(std::string(moje_souborove_pole, file_size));
		snprintf(buffer_tx, BUFFERS_LEN, "ID=%d|%s", 5, hash.c_str());
		add_crc(buffer_tx, 0, 0);
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		std::cout << "DEBUG: Sending hash of: " << hash << std::endl;
		std::cout << "DEBUG: BUFFER_TX IS " << buffer_tx << std::endl;
	}
	free(moje_souborove_pole);
	fclose(pepe_file);
	
	closesocket(socketS);
	exit(1);
}

