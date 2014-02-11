#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

#define IPADDR(a,b,c,d) (htonl(((a)<<24)|((b)<<16)|((c)<<8)|(d)))
#define SERVER_HOST IPADDR(192,168,1,1)
#define SERVER_PORT htons(42000)

#define BUFFER_SIZE 128
#define SEPARATOR 45

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /*lock on fb and disp_row*/
int disp_row = 0; /*row to print next chat*/

int main()
{
	int err, col, row;

	struct sockaddr_in serv_addr = { AF_INET, SERVER_PORT, { SERVER_HOST } };

	struct usb_keyboard_packet packet;
	int transferred;
	char keystate[12];

	if ((err = fbopen()) != 0) {
		fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
		exit(1);
	}

	/* Clear the framebuffer */
	for (row = 0; row < 47; row++)
		fbputspace(row);

	/* Draw rows of asterisks across the top and bottom of the screen */
	for (col = 0 ; col < 128 ; col++) {
		fbputchar('*', 0, col);
		fbputchar('*', SEPARATOR, col);
	}

	fbputs("Hello CSEE 4840 World!", 4, 10);

	/* Open the keyboard */
	if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
		fprintf(stderr, "Did not find a keyboard\n");
		exit(1);
	}

	/* Create a TCP communications socket */
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "Error: Could not create socket\n");
		exit(1);
	}

	/* Connect the socket to the server */
	if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
		exit(1);
	}

	/* Announce we have arrived */
	write(sockfd, "Hello World!\n", 13);

	/* Start the network thread */
	pthread_create(&network_thread, NULL, network_thread_f, NULL);

	char firstkey, seckey;
	int modkey = 1;//unset if the keypress is not an ASCII char
	col = 0;//we'll start typing at left-hand edge of screen
	row = 46;//we'll also start right below the second line of asterisks

	/* Look for and handle keypresses */
	for (;;) {
		libusb_interrupt_transfer(keyboard, endpoint_address,
				(unsigned char *) &packet, sizeof(packet),
				&transferred, 0);
		if (transferred == sizeof(packet)) {

			// Converting USB codes to ASCII
			/* Do we have to do more than the first keypress???*/
			firstkey = packet.keycode[0];
			if (packet.modifiers == USB_LSHIFT || packet.modifiers == USB_RSHIFT){
				if (firstkey >= 4 && firstkey <= 29)
					firstkey += 61;
				else{
					// DO WE ALSO HAVE TO DO KEYPAD?
					switch(firstkey){
						case 30: firstkey = '!';  
										 break;
						case 31: firstkey = '@';
										 break;
						case 32: firstkey = '#';
										 break;
						case 33: firstkey = '$';
										 break;
						case 34: firstkey = '%';
										 break;
						case 35: firstkey = '^';
										 break;
						case 36: firstkey = '&';
										 break;
						case 37: firstkey = '*';
										 break;
						case 38: firstkey = '(';
										 break;
						case 39: firstkey = ')';
										 break;
						case 45: firstkey = '_';
										 break;
						case 46: firstkey = '+';
										 break;
						case 47: firstkey = '{';
										 break;
						case 48: firstkey = '}';
										 break;
						case 49: firstkey = '|';
										 break;
						case 51: firstkey = ':';
										 break;
						case 52: firstkey = '"';
										 break;
						case 53: firstkey = '~';
										 break;
						case 54: firstkey = '<';
										 break;
						case 55: firstkey = '>';
										 break;
						case 56: firstkey = '?';
										 break;
						default: modkey = 0;
										 break;
					}
				}
			}
			else{
				if (firstkey >= 4 && firstkey <= 29)
					firstkey += 93;
				else if (firstkey >= 30 && firstkey <= 38)
					firstkey += 19;
				else{
					switch(firstkey){
						case 39: firstkey = '0';
										 break;
						case 45: firstkey = '-';
										 break;
						case 46: firstkey = '=';
										 break;
						case 47: firstkey = '[';
										 break;
						case 48: firstkey = ']';
										 break;
						case 49: firstkey = '\\';
										 break;
						case 51: firstkey = ';';
										 break;
						case 52: firstkey = '\'';
										 break;
						case 53: firstkey = '`';
										 break;
						case 54: firstkey = ',';
										 break;
						case 55: firstkey = '.';
										 break;
						case 56: firstkey = '/';
										 break;
						default: modkey = 0;
										 break;
				}
			}

			if (modkey){
				printf("%c\n", firstkey);
				fbputchar(firstkey, row, col++);
				//wraparound
				if (col == 128){
					if (row == 47)
						row--;
					else
						row++;
					col = 0;
				}
				fbputchar('_', row, col);//cursor to mark where we're typing
			}
			//Deal with non-ASCII keystrokes
			else{
				//backspace
				if (firstkey == 42){
					fbputchar(' ', row, col--);
					fbputchar('_', row, col);
				}
				//TODO: How to tackle arrow keys? 
				//We want the _ cursor over an ascii character while also showing the cursor, right?
				
				//TODO: Enter
			}



			sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
					packet.keycode[1]);
			printf("%s\n", keystate);
			fbputs(keystate, 6, 0);
			if (packet.keycode[0] == 0x29) { /* ESC pressed? */
				break;
			}
		}
	}

	/* Terminate the network thread */
	pthread_cancel(network_thread);

	/* Wait for the network thread to finish */
	pthread_join(network_thread, NULL);

	return 0;
}

void *network_thread_f(void *ignored)
{
	char recvBuf[BUFFER_SIZE];
	int n;
	/* Receive data */
	while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
		pthread_mutex_lock(&mutex); /* Grab the lock */
		/*scroll the screen if it is already full*/
		if (row==SEPARATOR) {
			fbscroll(SEPARATOR);
			row--;
		}
		recvBuf[n] = '\0';
		printf("%s", recvBuf);
		fbputs(recvBuf, row, 0);
		row++;
		pthread_mutex_unlock(&mutex); /* Release the lock */
	}
	return NULL;
}