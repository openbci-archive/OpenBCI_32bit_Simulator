#define _POSIX_SOURCE 1       // POSIX compliant source
#define _BSD_SOURCE

#include <stdio.h>                  
#include <termios.h>                
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>                  
#include <sys/signal.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define BAUDRATE B115200            // define baudrate (115200bps)
#define DEVICEPORT "/dev/ttyUSB0"         // define port
#define FALSE 0
#define TRUE 1
#define BUFFERSIZE 256

volatile int STOP=FALSE; 

/*Function declarations*/
void signal_handler_IO (int status);
//void byte_parser(unsigned char buf[], int res);
int open_port();
int open_fake_port(); //opens up the fake port for testing
void setup_port(); //to setup the real port only, not called here though
void streaming();
void clear_buffer();
void shift_buffer_down();
int close_port();
int send_to_board(char* message);
int interpret16bitAsInt32(char byte0, char byte1);
float * byte_parser (unsigned char buf[], int res); 

int wFile; //file that we will open. 
struct termios  config;

int main(int argc, char **argv) 
{
	char command[128];
	char response[48];
	int cFlag = 0;
	int r, c, ret, testmode;
	int x=0, y=0, z=0;
	
	int iterations = 1; testmode = 0;
	if(argc > 1 ) iterations = atoi(argv[1]);
	if(argc > 2) testmode = 1; //enable test mode
	// open port
	if(testmode)
	{
		printf("\nTest Mode");
		ret = open_fake_port();
		if(ret == -1) return -1; //exit if failed
	}
	else
	{
		printf("\nRegular mode");
		ret = open_port();
		if(ret == -1) return -1; //exit if failed
	}

	send_to_board("v"); //reset board
	printf("\nSent v to board");
	sleep(1);
	//printf("\nSend to board b return: %d", send_to_board("b")); //start sequence
	send_to_board("b");
	printf("\nSent b to board");	

	int d = 0;
	int zeroCounter = 0;
	int i;
	for(i=0;i<iterations; i++)
	{
		usleep(2500); //wait before sending out next request
		strcpy(response,""); //clear response
		
		printf("\nIteration %d", i);
		//printf("\nByte 1 = %02X, byte l = %02X",response[0], response[32]);
		read(wFile,response,33);
		//printf("%s\n",response);		
		byte_parser(response, 33);
		//x = interpret16bitAsInt32(response[26], response[27]);
		//y = interpret16bitAsInt32(response[28], response[29]);
		//z = interpret16bitAsInt32(response[30], response[31]);
		//x = response[27] | response[26] <<8;
		//y = response[29] | response[28] <<8;
		//z = response[31] | response[30] <<8;

	}
	send_to_board("s"); //stop transmission
	strcpy(response,"");
	read(wFile, response, 33);
	if(testmode) printf("%s\n", response);
	printf("\n****END*****\n");
	close(wFile);
	return 0;
}

/*opens real port in blocking mode (requeting 33 bytes) */
int open_port()
{
	wFile = open(DEVICEPORT, O_RDWR | O_NOCTTY); //O_NDELAY
	if(wFile == -1)
	{
		printf( "\nFailed to open port\n" );
		return -1;
	}
	//setting up port
	bzero(&config, sizeof(config));
	config.c_cflag |= BAUDRATE | CS8 | CLOCAL | CREAD;
	config.c_oflag = 0;
	config.c_cc[VMIN]=33;                            // minimum of 33 byte per read
	config.c_cc[VTIME]=0; // minimum of 0 seconds between reads
	tcflush(wFile, TCIFLUSH);
	tcsetattr(wFile,TCSANOW,&config);
	return 0;
}

/*open fake port for testing purposes*/
int open_fake_port()
{
	wFile = open("/dev/board", O_RDWR);
	if(wFile < 0)
	{
		printf( "\nFailed to open fake port, make sure the module is inserted and running (lsmod)\n" );
		return -1;
	}
}

/*write to board*/
int send_to_board(char* message){
    return write(wFile,message,strlen(message));                                // size is now dynamic
}

/*self explanatory, from openBCIs webpage */
int interpret16bitAsInt32(char byte0, char byte1) {
    int newInt = (
      ((0xFF & byte0) << 8) |
       (0xFF & byte1)
      );
    if ((newInt & 0x00008000) > 0) {
          newInt |= 0xFFFF0000;
    } else {
          newInt &= 0x0000FFFF;
    }
    return newInt;
  }

/*Gabe's byte parser */
float * byte_parser (unsigned char buf[], int res){
	static unsigned char framenumber = -1;						// framenumber = sample number from board (0-255)
	static int channel_number = 0;								// channel number (0-7)
	static int acc_channel = 0;									// accelerometer channel (0-2)
	static int byte_count = 0;									// keeps track of channel bytes as we parse
	static int temp_val = 0;									// holds the value while converting channel values from 24 to 32 bit integers
	static float output[11];									// buffer to hold the output of the parse (all -data- bytes of one sample)
	int parse_state = 0;										// state of the parse machine (0-5)
	printf("\n######### NEW PACKET ##############");
	for (int i=0; i<res;i++){									// iterate over the contents of a packet
		//printf("%d |", i);										// print byte number (0-33)
		printf("\nPARSE STATE %d | ", parse_state);				// print current parse state
		printf("BYTE %x\n",buf[i]);								// print value of byte

		/**************************************************************
		// STATE MACHINE
		//
		*/
		switch (parse_state) {
			case 0:												// STATE 0: find end+beginning byte
				if (buf[i] == 0xC0){							// if finds end byte first, look for beginning byte next
					parse_state++;								
				}
				else if (buf[i] == 0xA0){						// if find beginning byte first, proceed to parsing sample number (state 2)
					parse_state = 2;													
				}
				break;
			case 1:												// STATE 1: Look for header (in case C0 found first)
					if (buf[i] == 0xA0){
						parse_state++;
					}else{								
						parse_state = 0;
					}
				break;
			case 2: 											// Check framenumber
					if (((buf[i]-framenumber)!=1) && (buf[i]==0)){	
						/* Do something like this to check for missing
								packets. Keep track of missing packets. */
						printf("MISSING PACKET \n");
					}
					framenumber++;
					parse_state++;
					break;
			case 3:												// get ADS channel values **CHANNEL DATA**
				temp_val |= (((unsigned int)buf[i]) << (16 - (byte_count*8))); //convert to MSB
				byte_count++;	
				if (byte_count==3){								// if 3 bytes passed, 24 bit to 32 bit conversion
					printf("CHANNEL NO. %d\n", channel_number + 1);
					if ((temp_val & 0x00800000) > 0) {
						temp_val |= 0xFF000000;
					}else{
						temp_val &= 0x00FFFFFF;
					}
					// temp_val = (4.5 / 24 / float((pow(2, 23) - 1)) * 1000000.f) * temp_val; // convert from count to bytes
					output[channel_number] = temp_val;			// place value into data output buffer
					channel_number++;
					if (channel_number==8){						// check to see if 8 channels have already been parsed
						parse_state++;
						byte_count = 0;
						temp_val = 0;
						acc_channel = 0;
					}else{
						byte_count = 0;
						temp_val = 0;
					}
				}
				break;
			case 4:												// get LIS3DH channel values 2 bytes times 3 axes **ACCELEROMETER**
				temp_val |= (((unsigned int)buf[i]) << (8 - (byte_count*8)));
				byte_count++;
				if (byte_count==2) {
					if ((temp_val & 0x00008000) > 0) {
						temp_val |= 0xFFFF0000;
					} else {
						temp_val &= 0x0000FFFF;
					}  
					// printf("channel no %d\n", channel_number);
					printf("acc channel %d\n", acc_channel);
					output[acc_channel + 8]=temp_val;			// output onto buffer
					acc_channel++;
					if (acc_channel==3) {  						// all channels arrived !
						parse_state++;
						byte_count=0;
						channel_number=0;
						temp_val=0;
					}
					else { byte_count=0; temp_val=0; }
				}
				break;

			case 5: 											// look for end byte
				if (buf[i] == 0xC0){
					parse_state = 0;
				}
				else{
					// something about synching here
					parse_state= 0;								// resync
				}
				break;

		default: parse_state=0;		
		}
	}
	return output;
}

