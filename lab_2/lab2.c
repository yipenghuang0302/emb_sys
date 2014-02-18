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
#define SERVER_HOST IPADDR(192,168,1,117)
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

pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER; /*lock on fb and chat_row*/
int chat_row = 0; /*row to print next chat*/

/*Share read access to user_buf and user_spot with keyboard thread*/
/*Share read access to user_col and user_row with keyboard thread*/
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER; /*lock on user_buf and user_spot*/
char user_buf[USER_BUF_SIZE];
int user_spot = 0;
int user_col = 0; /*we'll start typing at left-hand edge of screen*/
int user_row = SEPARATOR + 1; /*we'll also start right below the second line of asterisks*/

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
	int setup_col, setup_row;
	if ((err = fbopen()) != 0) {
		fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
		exit(1);
	}

	/* Clear the framebuffer */
	for (setup_row = 0; setup_row < ROW_MAX; setup_row++)
		fbputspace(setup_row);

	/* Draw rows of asterisks across the top and bottom of the screen */
	for (setup_col = 0 ; setup_col < COL_MAX ; setup_col++) {
		fbputchar('*', 0, setup_col);
		fbputchar('*', SEPARATOR, setup_col);
	}

	/* Create a TCP communications socket */
	struct sockaddr_in serv_addr = { AF_INET, SERVER_PORT, { SERVER_HOST } };
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

  	memset (user_buf, '\0', USER_BUF_SIZE);

	/* Start the network thread */
	pthread_create(&network_thread, NULL, network_thread_f, NULL);

	/* Start the cursor thread */
	pthread_create(&cursor_thread, NULL, cursor_thread_f, NULL);

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
				pthread_mutex_lock(&user_mutex); /* Grab the lock */
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
				pthread_mutex_unlock(&user_mutex); /* Release the lock */
			}
			//Deal with non-ASCII keystrokes
			else{
				//backspace
				if (firstkey == 42 && user_spot > 0) {
					pthread_mutex_lock(&user_mutex); /* Grab the lock */
					user_buf[user_spot--] = ' ';
					fbputchar(' ', user_row, user_col--);
					pthread_mutex_unlock(&user_mutex); /* Release the lock */
				}
				//TODO: How to tackle arrow keys? 
				//left arrow
				if (firstkey == 80 && user_spot > 0) {
					pthread_mutex_lock(&user_mutex); /* Grab the lock */
					user_spot--;
					pthread_mutex_unlock(&user_mutex); /* Release the lock */
				}
				//right arrow
				if (firstkey == 81 && user_spot < USER_BUF_SIZE) {
					pthread_mutex_lock(&user_mutex); /* Grab the lock */
					user_spot++;
					pthread_mutex_unlock(&user_mutex); /* Release the lock */
				}

				//return
				if (firstkey == 40) {
					pthread_mutex_lock(&user_mutex); /* Grab the lock */
					user_buf[user_spot]='\0';
					write(sockfd, user_buf, user_spot-1);
					pthread_mutex_unlock(&user_mutex); /* Release the lock */


					//Print to "chat" section of screen
					int print_spot = 0;
					while (print_spot < user_spot) {
						if (user_spot-print_spot > CHAT_BUF_SIZE-1) {
							chat_print(user_buf+print_spot, CHAT_BUF_SIZE-1);
							print_spot += CHAT_BUF_SIZE-1;
						}
						else {
							chat_print(user_buf+print_spot, user_spot-print_spot);
							print_spot += user_spot-print_spot;
						}
					}

					//Reset user's text space
					pthread_mutex_lock(&user_mutex); /* Grab the lock */
					user_spot = 0;
					user_row = SEPARATOR+1;
					user_col = 0;
					pthread_mutex_unlock(&user_mutex); /* Release the lock */
					//clear user's framebuffer space
					int clear_index;
					for (clear_index = SEPARATOR+1; clear_index<ROW_MAX; clear_index++)
						fbputspace(clear_index);
				}
				is_ascii = 1;
			}
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

/*Every second, write alternating cursor and character to user_spot*/
void *cursor_thread_f() {
	for (;;) {
		pthread_mutex_lock(&chat_mutex); /* Grab the lock */
		fbputchar('_', user_row, user_col); //cursor to mark where we're typing
		pthread_mutex_unlock(&chat_mutex); /* Release the lock */
		usleep(500000);
		pthread_mutex_lock(&chat_mutex); /* Grab the lock */
		fbputchar(user_buf[user_spot], user_row, user_col); //cursor to mark where we're typing
		pthread_mutex_unlock(&chat_mutex); /* Release the lock */
		usleep(500000);
	}
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
	char temp;
	pthread_mutex_lock(&chat_mutex); /* Grab the lock */
	temp = chat_buf[size];
	/*scroll the screen if it is already full*/
	if (chat_row==SEPARATOR) {
//		fbscroll(SEPARATOR);
		int clear_row;
		for (clear_row = 0; clear_row < SEPARATOR; clear_row++)
			fbputspace(clear_row);
		chat_row=0;
	}
	chat_buf[size] = '\0';
	fbputs(chat_buf, chat_row, 0);
	chat_buf[size]=temp;	
	chat_row++;
	pthread_mutex_unlock(&chat_mutex); /* Release the lock */
}
