// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "md5.h"
#include "ws2tcpip.h"
#include <string.h>
#include "..//..//libcrc-master/src/crc16.c"

#define RECEIVER

#define TARGET_IP	"127.0.0.1"

#define TARGET_PORT 8888
#define LOCAL_PORT 5555

#define BUFFERS_LEN 1024

void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void send_ack(SOCKET socket,char *buffer_tx, sockaddr_in addrDest) {
	char ack[10];
	snprintf(ack, 10, "ACK");
	strncpy(buffer_tx, ack, BUFFERS_LEN);
	sendto(socket, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	printf("ACK SEND\n");
}


//**********************************************************************
int main()
{
	SOCKET socketS;
	
	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in from;

	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0){
		printf("Binding error!\n");
	    getchar(); //wait for press Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN];
	char buffer_tx[BUFFERS_LEN];

	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);


	memset(buffer_rx, 0x00, sizeof buffer_rx);
	printf("Waiting for datagram ...\n");
	char fname[100];
	char crc_str[16];
	int crc = 0;
	char *f_data = NULL;
	FILE* fptr;
	bool didWrite = 0;
	int index = 0;
	long data_size = 0;
	char size_str[100];
	int total_read = 0;
	int read_amount = 0;
	bool com_started = false;
	char msg_type = '0';
	int msg_len = 0;
	memset(fname, 0x00, sizeof fname);
	while (1) {
		if (recvfrom(socketS, buffer_rx, 1024, 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
			printf("Socket error! %d\n", WSAGetLastError());
			getchar();
		}
		
		else {
			msg_type = buffer_rx[3];
		}
		char* pointer = NULL;
		//printf("MSG type %c\n", msg_type);

		// Waits for start msg
		if (com_started != true && msg_type != '1') {
			printf("Recieved msg without start\n");
			continue;
		}
		else {

		}

		//Fetch start msg
		if (msg_type == '1') {
			com_started = true;
			pointer = strstr(buffer_rx, "ID=1");
			pointer += 5 * sizeof(char);
			msg_len = 4;

			for (int i = 0; i < 16;i++) {
				crc_str[i] = pointer[i];
			}
			crc = strtol(crc_str, NULL, 10);
			printf("COM STARTED\n");
		}

		//Fetch name of file
		if (msg_type == '2') {
			pointer = strstr(buffer_rx, "ID=2");
			pointer += 5 * sizeof(char);
			msg_len += 4;

			int i = 0;
			while (pointer[0] != '|') {
				fname[i] = pointer[0];
				i++;
				pointer += sizeof(char);
			}
			msg_len += i + 1;
			printf("FILENAME: %s\n", fname);

			pointer += sizeof(char);
			for (int i = 0; i < 16;i++) {
				crc_str[i] = pointer[i];
			}
			crc = strtol(crc_str, NULL, 10);
		}

		//Fetch size of file and allocate memory
		if (msg_type == '3') {

			pointer = strstr(buffer_rx, "ID=3");
			pointer += 5 * sizeof(char);
			msg_len += 4;

			int i = 0;
			while (pointer[0] != '|') {
				size_str[i] = pointer[0];
				i++;
				pointer += sizeof(char);
			}
			msg_len += i + 1;
			data_size = strtol(size_str, NULL, 10);
			printf("DATA_SIZE: %d\n", data_size);
			pointer += sizeof(char);

			for (i = 0; i < 16;i++) {
				crc_str[i] = pointer[i];
			}
			crc = strtol(crc_str, NULL, 10);

			f_data = (char*)calloc(data_size + 5, sizeof(char));
			memset(f_data, 0x00, data_size * sizeof(char));
			if (f_data == NULL) {
				printf("ERROR: cannot allocate memory.\n");
				exit(1);
			}
		}

		//Fetch data and save it to file_buffer
		if (msg_type == '4') {

			pointer = strstr(buffer_rx, "ID=4");
			char* num_bytes = (char*)calloc(10, sizeof(char));
			pointer += 5 * sizeof(char);
			msg_len += 5;
			int i = 0;
			while (pointer[0] != '|') {
				num_bytes[i] = pointer[0];
				i++;
				pointer += sizeof(char);
			}
			msg_len += i + 1;

			read_amount = strtol(num_bytes, NULL, 10);
			char* data_buffer = (char*)calloc(read_amount + 2, sizeof(char));
			pointer += sizeof(char);
			int bytes_read = 0;
			for (int i = 0;i < read_amount;i++) {
				bytes_read++;
				f_data[i + total_read] = pointer[0];
				data_buffer[i] = pointer[0];
				pointer++;

			}
			pointer++;
			msg_len += bytes_read;
			for (i = 0; i < 16;i++) {
				crc_str[i] = pointer[i];
			}

			crc = strtol(crc_str, NULL, 10);
			total_read += read_amount;
			index++;
			printf("DEBUG: Received: %d B\n", read_amount);
			free(data_buffer);
		}
		if (total_read >= data_size && data_size != 0 && read_amount == 0 && didWrite==false) {
			didWrite = true;
			printf("DEBUG: Finished writing to file\n", index, data_size);
			fptr = fopen(fname, "wb");
			int ret = fwrite(f_data, sizeof(char), data_size, fptr);
			fclose(fptr);
		}
		if (msg_type == '5') {

			char* pointer = strstr(buffer_rx, "ID=5");
			pointer=pointer + 5*sizeof(char);
			char* hash = (char*)calloc(64, sizeof(char));
			int i = 0;
			while (pointer[0] != '|') {
				hash[i] = pointer[0];
				i++;
				pointer += sizeof(char);
			}
			std::cout << "DEBUG: Received hash: " << hash << std::endl;
			FILE* pepe_file = fopen(fname, "rb");
			char* moje_souborove_pole = (char*)calloc(data_size, sizeof(char));
			int byte_read = fread(moje_souborove_pole, sizeof(char), data_size, pepe_file);
			if (byte_read != data_size) {
				printf("Cannot read file, bye.\n");
				std::cout << byte_read << " " << data_size << std::endl;
			}
			else {
				std::string vysledek = md5(std::string(f_data, data_size));
				std::cout << "DEBUG: Computed hash: " << vysledek << std::endl;
			}
		}

		//CRC control and ACK 
		char* msg_no_crc = (char*)calloc(msg_len, sizeof(char));
		for (int i = 0; i < msg_len; i++) {
			msg_no_crc[i] = buffer_rx[i];
		}
		int t = crc_16((unsigned char*)(msg_no_crc), msg_len);
		if (t == crc) {
			send_ack(socketS, buffer_tx, from);
		}
		else {

		}
		free(msg_no_crc);
		msg_len = 0;
		
		//Transfer ended
		

		// Pole alokovano
		
		memset(buffer_rx, 0, sizeof buffer_rx);
	}
	
	closesocket(socketS);
	//**********************************************************************

	getchar(); //wait for press Enter
	return 0;
}
