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

#define CHAT_BUF_SIZE 128
#define USER_BUF_SIZE 1024

#define SEPARATOR 45 /*location of bar separating chat and user space*/
#define ROW_MAX 48
#define COL_MAX 128

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
void *network_thread_f();
pthread_t cursor_thread;
void *cursor_thread_f();
void chat_print();

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /*lock on fb and chat_row*/
int chat_row = 0; /*row to print next chat*/

int main()
{

	int err;

	/* Open the keyboard */
	struct usb_keyboard_packet packet;
	int transferred;
	char firstkey;
	int is_ascii = 1; /*unset if the keypress is not an ASCII char*/
	if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
		fprintf(stderr, "Did not find a keyboard\n");
		exit(1);
	}

	/*initiate framebuffer*/
	int user_col, user_row;
	if ((err = fbopen()) != 0) {
		fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
		exit(1);
	}

	/* Clear the framebuffer */
	for (user_row = 0; user_row < ROW_MAX; user_row++)
		fbputspace(user_row);

	/* Draw rows of asterisks across the top and bottom of the screen */
	for (user_col = 0 ; user_col < COL_MAX ; user_col++) {
		fbputchar('*', 0, user_col);
		fbputchar('*', SEPARATOR, user_col);
	}

	/* Create a TCP communications socket */
	struct sockaddr_in serv_addr = { AF_INET, SERVER_PORT, { SERVER_HOST } };
	char user_buf[USER_BUF_SIZE];
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

	/* Start the cursor thread */
	pthread_create(&cursor_thread, NULL, cursor_thread_f, NULL);

	user_col = 0; /*we'll start typing at left-hand edge of screen*/
	user_row = SEPARATOR + 1; /*we'll also start right below the second line of asterisks*/

	int user_spot = 0;
  	memset (user_buf, '\0', USER_BUF_SIZE);

	/* Look for and handle keypresses */
	for (;;) {
		libusb_interrupt_transfer(keyboard, endpoint_address,
				(unsigned char *) &packet, sizeof(packet),
				&transferred, 0);
		if (transferred == sizeof(packet)) {

			// Converting USB codes to ASCII
			/* Do we have to do more than the first keypress???*/
			firstkey = packet.keycode[0];
			if ( packet.modifiers & USB_LSHIFT || packet.modifiers & USB_RSHIFT ) {

				/*Capital letters*/
				if (firstkey >= 4 && firstkey <= 29) {
					firstkey += 61;
				}
				else{
					// DO WE ALSO HAVE TO DO KEYPAD?
					switch(firstkey) {
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
						case 44: firstkey = ' ';
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
						default: is_ascii = 0;
										 break;
					}
				}
			}

			/*Lowercase letters*/
			else{
				if (firstkey >= 4 && firstkey <= 29) {
					firstkey += 93;
				}
				else if (firstkey >= 30 && firstkey <= 38)
					firstkey += 19;
				else{
					switch(firstkey) {
						case 39: firstkey = '0';
										 break;
						case 44: firstkey = ' ';
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
						case 0: break;
						default: is_ascii = 0;
										 break;
					}
				}
			}

			if (is_ascii) {
				/*print to user space and accumulate*/
				if (firstkey) {
					fbputchar(firstkey, user_row, user_col++);
					user_buf[user_spot++] = firstkey;
				}
				//wraparound
				if (user_col == COL_MAX) {
					user_row++;
					if (user_row == ROW_MAX)
						user_row = SEPARATOR+1;
					user_col = 0;
				}
				fbputchar('_', user_row, user_col);//cursor to mark where we're typing
			}
			//Deal with non-ASCII keystrokes
			else{
				//backspace
				if (firstkey == 42) {
					user_buf[user_spot--] = ' ';
					fbputchar(' ', user_row, user_col--);
					fbputchar('_', user_row, user_col);
				}
				//TODO: How to tackle arrow keys? 
				//We want the _ cursor over an ascii character while also showing the cursor, right?

				if (firstkey == 40) {
					user_buf[user_spot]='\0';
					write(sockfd, user_buf, user_spot-1);

					//clear user's framebuffer space
					int clear_index;
					for (clear_index = SEPARATOR+1; clear_index<ROW_MAX; clear_index++)
						fbputspace(clear_index);

					//Print to "chat" section of screen
					int print_spot = 0;
					while (print_spot < user_spot) {
						if (user_spot-print_spot > CHAT_BUF_SIZE-1) {
							chat_print(user_buf+user_spot, CHAT_BUF_SIZE-1);
							print_spot += CHAT_BUF_SIZE-1;
						}
						else {
							chat_print(user_buf+user_spot, user_spot-print_spot);
						}
					}

					//Reset user's text space
					user_spot = 0;
					user_row = SEPARATOR+1;
					user_col = 0;
				}
				is_ascii = 1;
			}

			/*
				 sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
				 packet.keycode[1]);
				 printf("%s\n", keystate);
				 fbputs(keystate, 6, 0);
			 */
			if (packet.keycode[0] == 0x29) { /* ESC pressed? */
				break;
			}
		}
	}

	/* Terminate and wait for the other threads */
	pthread_cancel(network_thread);
	pthread_join(network_thread, NULL);
	pthread_cancel(cursor_thread);
	pthread_join(cursor_thread, NULL);

	return 0;
}

void *cursor_thread_f() {
	/*Pseudocode*/
	/*Share read access to user_buf and user_spot with keyboard thread*/
	/*Every second, write alternating cursor and character to user_spot*/
}

void *network_thread_f() {
	char chat_buf[CHAT_BUF_SIZE];
	int n;
	/* Receive data */
	while ( (n = read(sockfd, &chat_buf, CHAT_BUF_SIZE - 1)) > 0 ) {
		chat_print(chat_buf, n);
	}
	return NULL;
}

void chat_print(char *chat_buf, int size) {
	pthread_mutex_lock(&mutex); /* Grab the lock */
	/*scroll the screen if it is already full*/
	if (chat_row==SEPARATOR) {
		fbscroll(SEPARATOR);
		chat_row--;
	}
	chat_buf[size] = '\0';
	printf("%s", chat_buf);
	fbputs(chat_buf, chat_row, 0);
	chat_row++;
	pthread_mutex_unlock(&mutex); /* Release the lock */
}