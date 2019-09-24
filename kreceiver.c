#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
    	msg *received_message = NULL;
	int data_size, time;
	unsigned char error_counter = 0, seq = 0;
	unsigned short crc, receive_crc;
	char last_message, file_name[30] = "recv_";
	FILE *file;

	init(HOST, PORT);	

	/* Receive init message */	
	while (error_counter < 3 && received_message == NULL) {
		received_message =  receive_message_timeout(5000);

		if (received_message == NULL) {
        		printf("Didn't receive init message. Timeout error. End of transmission.\n");
			error_counter++;

		} else if (seq != received_message->payload[2]) {
			error_counter++;
		}else {
			crc = crc16_ccitt(received_message->payload, received_message->len - 3);
			memcpy(&receive_crc, &received_message->payload[received_message->len - 3], 2);
	
			if (crc == receive_crc) {
				send_package("", 0, seq, 'Y');
				last_message = 'Y';

			} else {
				send_package("", 0, seq, 'N');
				last_message = 'N';

				seq = (seq + 1) % 64;
				error_counter++;
				received_message = NULL;
			}
		}
	}

	if (received_message == NULL) {
		printf("[%s] Too many errors. End of transmission.\n", argv[0]);
		return -1;
	}

	time = received_message->payload[5] * 1000;

	/* Receive file data */
	while (1) {
		seq = (seq + 1) % 64;
		error_counter = 0;
		received_message = NULL;

		while (received_message == NULL) {
			received_message = receive_message_timeout(time);

    			if (received_message == NULL) {
				error_counter++;
				
				if (error_counter == 3) {
					printf("[%s] Too many errors. End of transmission.\n", argv[0]);
					return -1;
				}
				
				/* Send last message */
				if (last_message == 'N') {
					send_package("", 0, seq - 1, last_message);
					seq = (seq + 1) % 64;
				} else {
        				send_package("", 0, seq - 1, last_message);
				}
    			
			} else if (received_message->payload[2] != seq) {
				error_counter++;

				if (error_counter == 3) {
					printf("[%s] Too many errors. End of transmission.\n", argv[0]);
					return -1;
				}
				
				/* Send last message */
				if (last_message == 'N') {
					send_package("", 0, seq, last_message);
					seq = (seq + 1) % 64;
				} else {
					send_package("", 0, seq - 1, last_message);
				}
			
				received_message = NULL;

			} else {
				/* Check the message */
				crc = crc16_ccitt(received_message->payload, received_message->len - 3);
				memcpy(&receive_crc, &received_message->payload[received_message->len - 3], 2);

				if (crc == receive_crc) {
					printf("[%s] Received message with seq: %d\n", argv[0], received_message->payload[2]);
					/* Send ACK */
					send_package("", 0, seq, 'Y');
					last_message = 'Y';
					
					/* Open file if packege is header */
					if (received_message->payload[3] == 'F') {
						memcpy(file_name + 5, &received_message->payload[4], received_message->payload[1] - 5);
						file_name[5 + received_message->payload[4] + 1] = '\0';
					
						file = fopen(file_name, "wb");
					}
					
					/* Copy data in file */
					if (received_message->payload[3] == 'D') {
						data_size = (unsigned char) received_message->payload[1] - 5;
						fwrite(&received_message->payload[4], data_size, 1, file);
					}

					/* Close the file */
					if (received_message->payload[3] == 'Z') {
						fclose(file);
					}

					/* End transmission */
					if (received_message->payload[3] == 'B') {
						printf("[%s] End of transmission.\n", argv[0]);
						return 0;
					}

				} else {
					error_counter++;
					received_message = NULL;

					if (error_counter == 3) {
						printf("[%s] Too many errors. End of transmission.\n", argv[0]);
						return -1;
					}

					/* Send NACK */
					send_package("", 0, seq, 'N');
					last_message = 'N';
					seq = (seq + 1) % 64;
				}
			}
		}
	}

	return 0;
}
