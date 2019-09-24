#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define MAXL 250
#define TIME 5

int send_to_receiver(int seq, char *data, int size, char type) {
	int error_counter = 0, expected_seq = 0;
	msg *receive_message = NULL;

	while (error_counter < 3 && (receive_message == NULL || seq != expected_seq)) {
		send_package(data, size, seq, type);

		receive_message = receive_message_timeout(TIME * 1000);

		if (receive_message == NULL) {
			printf("[%s] Didn't receive the message. Timeout error.\n", "./ksender");
			error_counter++;

		} else if (receive_message->payload[3] == 'Y') {
			expected_seq = receive_message->payload[2];

			if (expected_seq != seq) {
				printf("[%s] Didn't receive the correct message. Send again.\n", "./ksender");
				error_counter++;
			} else {
				printf("[%s] ACK received with seq: %d\n", "./ksender", seq);
			}	
		} else {
			printf("[%s] NACK received with seq: %d\n", "./ksender", receive_message->payload[2]);
			seq = (seq + 1) % 64;

			error_counter++;
			receive_message = NULL;
		}
	}
			
	if (receive_message == NULL) {
		printf("[%s] Too many errors. End of transmission.\n", "./ksender");
		return -1;
	}
			
	return seq;
}

int main(int argc, char** argv) {
	int seq = 0, file_size, name_length, count_packages,
	    current_package, rest, error_counter = 0, result;
	char *file_name, *file_name_copy, *data;
	FILE *file;
	msg *receive_message = NULL;

	/* Initialize transmission. */
	init(HOST, PORT);
	
	/* Send init package. */
	while (receive_message == NULL && error_counter < 3) {
		send_init(seq);

		/* Receive ACK for init package. */
		receive_message = receive_message_timeout(TIME * 1000);
	
		if (receive_message == NULL) {
			printf("[%s] Timeout error.\n", "./ksender");
			error_counter++;

		} else if (receive_message->payload[3] == 'Y') {
        		printf("[%s] Got reply with payload: %d\n", argv[0], receive_message->payload[2]);

    		} else {
			printf("[%s] NACK received with seq: %d\n", argv[0], receive_message->payload[2]);
			seq = (seq + 1) % 64;
			error_counter++;
			receive_message = NULL;
		}
	}

	if (receive_message == NULL) {
		printf("[%s] Too many errors. End of transmission.\n", argv[0]);
		return 0;
	}
	
	seq = (seq + 1) % 64;

	/* Send all the files. */
	for (int i = 1; i < argc; i++) {

		/* Read input file */
		file_name = argv[i];
		file = fopen(file_name, "rb");

		if (file == NULL) {
			perror("Can't open file.\n");
			return -1;
		}

		/* Count file size. */
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
	
		/* Read the data from the file. */
		data = malloc(sizeof(char) * file_size);

		if (data == NULL) {
			perror("Can't allocate memory.\n");
			return -1;
		}

		fread(data, file_size, 1, file);
		
		/* Close the file after the reading is done. */
		fclose(file);

		/* File name length. */
		name_length = 0;
		file_name_copy = argv[i];
	
		while(*file_name_copy != '\0') {
			file_name_copy++;
			name_length++;
		}
	
		char name[name_length];
		memcpy(name, file_name, name_length + 1);
		
		/* Send header package. */
		result = send_to_receiver(seq, name, name_length + 1, 'F');
		
		if(result == -1) {
			return -1;
		}

		seq = (result + 1) % 64;

		/* Send data from file in packages. */
		count_packages = file_size / MAXL;
		current_package = 0;	

		while (current_package < count_packages) {
			result = send_to_receiver(seq, data + current_package * MAXL, MAXL, 'D');
		
			if(result == -1) {
				return -1;
			}
			
			seq = (result + 1) % 64;
			current_package++;
		}

		rest = file_size - count_packages * MAXL;
		result = send_to_receiver(seq, data + count_packages * MAXL, rest, 'D');
		
		if (result == -1) {
			return -1;
		}
			
		seq = (result + 1) % 64;
		
		/* Send EOF package. */
		result = send_to_receiver(seq, "", 0, 'Z');
		
		if (result == -1) {
			return 0;
		}

		seq = (result + 1) % 64;
	}

	/* Send EOT package. */
	result = send_to_receiver(seq, "", 0, 'B');
		
	if(result == -1) {
		return -1;
	}

	printf("[%s] End of transmission.\n", argv[0]);

    	return 0;
}
