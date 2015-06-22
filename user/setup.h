//
#ifndef __SETUPU_H__
#define __SETUPU_H__

#define RESET 'r'
#define BAUD  'b'
#define PORT  'p'
#define IP    'i'    
#define MODE  'm'
#define STA   's'
#define AP    'a'
#define FLASH 'f'
#define GPIO2 'g'
#define INFO  'x'
#define TEST  't'

#define MAX_TOKENS  10 // far too many
#define DELIMS ",;"
// string tokeniser stack, each value represents a position
// and length of the original string
typedef struct {
	const char *s;
	int len;
} token_t;


// forward
int str2ip(int *pr, char *def);
void get_flash_ip(int req, char *buf);
void ipdetails(char *buf);

#endif