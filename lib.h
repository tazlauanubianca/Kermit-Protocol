#ifndef LIB
#define LIB
#define MAXL 250
#define TIME 5

typedef struct {
    int len;
    char payload[1400];
} msg;

/* Mini-Kermit structure */
typedef struct __attribute__((__packed__)) {
	char soh;
	unsigned char len;
	unsigned char seq;
	char type;
	char *data;
	unsigned short check;
	char mark;
} mini_kermit;

typedef struct __attribute__((__packed__)) {
	char maxl;
	char time;
	char npad;
	char padc;
	char eol;
	char qctl;
	char qbin;
	char chkt;
	char rept;
	char capa;
	char r;
} init_package_data;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

void initialize_init_package_data(init_package_data *S_package_data) {
	S_package_data->maxl = MAXL;
	S_package_data->time = TIME;
	S_package_data->npad = 0x00;
	S_package_data->padc = 0x00;
	S_package_data->eol = 0x0D;
	S_package_data->qctl = 0x00;
	S_package_data->qbin = 0x00;
	S_package_data->chkt = 0x00;
	S_package_data->rept = 0x00;
	S_package_data->capa = 0x00;
	S_package_data->r = 0x00;
}

void send_init(int seq) {
	init_package_data S_package_data;
	mini_kermit S_package;
	msg init;

	initialize_init_package_data(&S_package_data);
	
	S_package.data = malloc(sizeof(S_package_data));
	if (S_package.data == NULL) { 
		perror("Can't allocate memory.\n");
	}

	S_package.soh = 0x01;
	S_package.len = 5 + sizeof(init_package_data);
	S_package.seq = seq;
	S_package.type = 'S';
	
	memcpy(S_package.data, &S_package_data, sizeof(S_package_data));

	memcpy(init.payload, &S_package.soh, 1);
	memcpy(init.payload + 1, &S_package.len, 1);
	memcpy(init.payload + 2, &S_package.seq, 1);
	memcpy(init.payload + 3, &S_package.type, 1);
	memcpy(init.payload + 4, S_package.data, 11);

	S_package.check = crc16_ccitt(init.payload, 15);
	S_package.mark = 0x0D;

	memcpy(init.payload + 15, &S_package.check, 2);
	memcpy(init.payload + 17, &S_package.mark, 1);

	init.len = S_package.len + 2;
	send_message(&init);
}

void send_package(char *data, unsigned char len, unsigned char seq, char type) {
	mini_kermit package;
	msg init;

	if (len != 0) {
		package.data = malloc(sizeof(char) * len);
		if (package.data == NULL) {
			perror("Can't allocate memory.\n");
		}

		memset(package.data, 0, len);
		memcpy(package.data, data, len);
	}

	package.soh = 0x01;
	package.len = 5 + len;
	package.seq = seq;
	package.type = type;

	memcpy(init.payload, &package.soh, 1);
	memcpy(init.payload + 1, &package.len, 1);
	memcpy(init.payload + 2, &package.seq, 1);
	memcpy(init.payload + 3, &package.type, 1);

	if (len != 0) {
		memcpy(init.payload + 4, package.data, len);
	}

	package.check = crc16_ccitt(init.payload, len + 4);
	package.mark = 0x0D;

	memcpy(init.payload + 4 + len, &package.check, 2);
	memcpy(init.payload + 6 + len, &package.mark, 1);

	init.len = package.len + 2;
	send_message(&init);
}
#endif

