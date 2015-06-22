//
//
// this is the normal build target ESP include set
#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "driver/uart.h"
//#include "stdio.h"

#include "flash_param.h"
#include "server.h"
#include "config.h"
#include "setup.h"

#define MSG_OK "OK\r\n"
#define MSG_ERROR "ERROR\r\n"
#define MSG_INVALID_CMD "UNKNOWN COMMAND\r\n"

bool doflash = true;

// *****************************************************************************
// prints a single number to console with specified format
// *****************************************************************************
void pnum(char *fmt, int32_t n)
{
  char buf[40]={0};
  os_sprintf(buf,fmt,n);
  uart0_sendStr(buf);
}

// *****************************************************************************
//prints a string
// *****************************************************************************
void pstr(char *fmt, char *s) 
{
  char buf[40]={0};
  os_sprintf(buf,fmt,s);
  uart0_sendStr(buf);
}

// *****************************************************************************
// tokenises a string without altering it, creates a pointer stack
// *****************************************************************************
int tokenise(char *s, token_t *q)
{
    int l_queue=0;
    char *p, *p1, *p2;

    while (*s == ' ') s++; // leading spaces
    p = s;
    for(;;) {
        p1 = strpbrk(p,DELIMS); // delimiter values
        if(p1) {
            q[l_queue].s = p;
            q[l_queue++].len = p1-p;
            p = p1+1;
        } else {
            // last one
            if(*p != 0) {
                q[l_queue].s = p;
                q[l_queue++].len = strlen(p);
            }
            break;
        }
    }
    return l_queue;
}

// *****************************************************************************
// puts a token into the given out buffer
// *****************************************************************************
void get_token(char *out, token_t *tq, int ntoken)
{
    strncpy(out,tq[ntoken].s,tq[ntoken].len); 
    out[tq[ntoken].len]=0; 
}

// *****************************************************************************
// converts an IP,portstring "192.168.2.2:33200" into integer numbers
// via ip_param type
// *****************************************************************************
int ICACHE_FLASH_ATTR str2ip(int *pr, char *def)
{
  uint8_t pos=0, pos1=0, ind=0, done=0;
  char buf[4]={0};
  
  if(strlen(def) < 10) return 0;
  // extract IP
//  while(*(def+pos)) {
  for(;;) {
    if(*(def+pos)=='.' || *(def+pos)=='\0') {
      buf[pos1]=0; // terminate
      pr[ind]=atoi(buf);
      ind++;
      pos1=0;
      pos++; // past '.' 
      if(*(def+pos)=='\0') return 1;
    } else {
      if(ind > 3) return 1;
      if(pos1 > 3) return 0; // error
      if(pos > strlen(def)+1) return 0; // error
      buf[pos1++] = *(def+pos++);
    }
  }
}

// *****************************************************************************
// puts requested details from flash into a buffer for later printing
// req = 1 = ip
// req = 2 = gw
// req = 3 = netmask
// req = 4 = port
// *****************************************************************************
void ICACHE_FLASH_ATTR get_flash_ip(int req, char *buf)
{
  flash_param_t *flash_param = flash_param_get();
  
  switch(req){
    case 1: {
      os_sprintf(buf,"%d.%d.%d.%d",
        flash_param->ip[0],
        flash_param->ip[1],
        flash_param->ip[2],
        flash_param->ip[3] );
        break;
    }
    case 2: {
      os_sprintf(buf,"%d.%d.%d.%d",
        flash_param->gw[0],
        flash_param->gw[1],
        flash_param->gw[2],
        flash_param->gw[3] );
        break;
    }
    case 3: {
      os_sprintf(buf,"%d.%d.%d.%d",
        flash_param->netmask[0],
        flash_param->netmask[1],
        flash_param->netmask[2],
        flash_param->netmask[3] );
        break;
    }
    case 4: {
      os_sprintf(buf,"%d",flash_param->port);
      break;
    }
    default: *buf=0;
  }
}

// *****************************************************************************
// used inmore than one place, buffer needs to be at least 80
// *****************************************************************************
void ICACHE_FLASH_ATTR ipdetails(char *buf)
{
  char b2[20]={0};
  *buf = 0;
  
  get_flash_ip(1, b2);
  strcpy(buf,"ip=");strcat(buf,b2);strcat(buf,",");   
  get_flash_ip(2, b2);
  strcat(buf,"gw=");strcat(buf,b2);strcat(buf,","); 
  get_flash_ip(3, b2);
  strcat(buf,"netmask=");strcat(buf,b2);strcat(buf,",");    
  get_flash_ip(4, b2);
  strcat(buf,"port=");strcat(buf,b2);strcat(buf,"\r\n"); 
}

void ICACHE_FLASH_ATTR setup_info()
{
  struct station_config conf;
  char buf[128]={0};
  uint8_t mode;
  // mode 
  mode = wifi_get_opmode();
  pnum("Mode: %d\r\n",mode);
  // station info
  os_bzero(&conf, sizeof(struct station_config));
  wifi_station_get_config(&conf);
  os_sprintf(buf,"SSID:%s PWD:%s SET:%d\r\n",conf.ssid,conf.password,conf.bssid_set);
  uart0_sendStr(buf);
  // dhcpc info
  mode = wifi_station_dhcpc_status();
  os_sprintf(buf,"dhcpc=%d\r\n",mode);
  uart0_sendStr(buf);
  // IP and port
  ipdetails(buf);
  uart0_sendStr(buf);
}


// *****************************************************************************
// Mode
// *****************************************************************************
void ICACHE_FLASH_ATTR u_setMode(char *s)
{
	uint8_t mode;

	if (strlen(s) == 0) {
      pnum("MODE=%d\r\n",wifi_get_opmode());
	} else {
		mode = atoi(s);
		if (mode >= 1 && mode <= 3) {
			if (wifi_get_opmode() != mode) {
				ETS_UART_INTR_DISABLE();
				wifi_set_opmode(mode);
				ETS_UART_INTR_ENABLE();
				uart0_sendStr(MSG_OK);
				os_delay_us(10000);
				system_restart();
			} else {
				uart0_sendStr(MSG_OK);
			}
		} else {
			uart0_sendStr(MSG_ERROR);
		}
	}
}

// *****************************************************************************
// Reset
// *****************************************************************************
void ICACHE_FLASH_ATTR u_reset()
{
	uart0_sendStr(MSG_OK);
	system_restart();
}

// *****************************************************************************
// BAUD
// *****************************************************************************
void ICACHE_FLASH_ATTR u_baud(char *cmd)
{
	flash_param_t *flash_param = flash_param_get();
	UartBitsNum4Char data_bits = GETUART_DATABITS(flash_param->uartconf0);
	UartParityMode parity = GETUART_PARITYMODE(flash_param->uartconf0);
	UartStopBitsNum stop_bits = GETUART_STOPBITS(flash_param->uartconf0);
	const char *stopbits[4] = { "?", "1", "1.5", "2" };
	const char *paritymodes[4] = { "E", "O", "N", "?" };
  
    int t;
    token_t queue[MAX_TOKENS];
    char buf[60]={0};
    
    t = tokenise(cmd,queue);

	if (t == 0) {
      os_sprintf(buf,"BAUD=%d %d %s %s\r\n"MSG_OK, flash_param->baud,data_bits + 5, paritymodes[parity], stopbits[stop_bits]);
      uart0_sendStr(buf);
	} else {
    get_token(buf, queue, 0);    
		uint32_t baud = atoi(buf);
		if ((baud > (UART_CLK_FREQ / 16)) || baud == 0) {
			uart0_sendStr(MSG_ERROR);
			return;
		}
		if (t > 1) {
      get_token(buf, queue, 1);
			data_bits = atoi(buf);
			if ((data_bits < 5) || (data_bits > 8)) {
				uart0_sendStr(MSG_ERROR);
				return;
			}
			data_bits -= 5;
		}
		if (t > 2) {
      get_token(buf, queue, 2);
			if (strcmp(buf, "N") == 0)
				parity = NONE_BITS;
			else if (strcmp(buf, "O") == 0)
				parity = ODD_BITS;
			else if (strcmp(buf, "E") == 0)
				parity = EVEN_BITS;
			else {
				uart0_sendStr(MSG_ERROR);
				return;
			}
		}
		if (t > 3) {
      get_token(buf, queue, 3);
			if (strcmp(buf, "1")==0)
				stop_bits = ONE_STOP_BIT;
			else if (strcmp(buf, "2")==0)
				stop_bits = TWO_STOP_BIT;
			else if (strcmp(buf, "1.5") == 0)
				stop_bits = ONE_HALF_STOP_BIT;
			else {
				uart0_sendStr(MSG_ERROR);
				return;
			}
		}
		// pump and dump fifo
		while (TRUE) {
			uint32_t fifo_cnt = READ_PERI_REG(UART_STATUS(0)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);
			if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) == 0) {
				break;
			}
		}
		os_delay_us(10000);
		uart_div_modify(UART0, UART_CLK_FREQ / baud);
		flash_param->baud = baud;
		flash_param->uartconf0 = CALC_UARTMODE(data_bits, parity, stop_bits);
		WRITE_PERI_REG(UART_CONF0(UART0), flash_param->uartconf0);
		if (doflash) {
			if (flash_param_set())
				uart0_sendStr(MSG_OK);
			else
				uart0_sendStr(MSG_ERROR);
		}
		else
			uart0_sendStr(MSG_OK);

	}
}

// *****************************************************************************
// PORT
// *****************************************************************************
void ICACHE_FLASH_ATTR u_port(char *cmd) {
	flash_param_t *flash_param = flash_param_get();
  int t;
  token_t queue[MAX_TOKENS];
  char buf[60]={0};
    
  t = tokenise(cmd,queue);
	if (t == 0) {
      pnum("PORT=%d\r\n"MSG_OK, flash_param->port);
	} else if (t != 1)
		uart0_sendStr(MSG_ERROR);
	else {
    get_token(buf, queue, 0);
		uint32_t port = atoi(buf);
		if ((port == 0)||(port>65535)) {
			uart0_sendStr(MSG_ERROR);
		} else {
			if (port != flash_param->port) {
				flash_param->port = port;
				if (flash_param_set())
					uart0_sendStr(MSG_OK);
				else
					uart0_sendStr(MSG_ERROR);
				os_delay_us(10000);
				system_restart();
			} else {
				uart0_sendStr(MSG_OK);
			}
		}
	}
}

// *****************************************************************************
// IP
// <ipaddress>  <gateway>  <mask>
// *****************************************************************************
void ICACHE_FLASH_ATTR u_ip(char *cmd) 
{
	flash_param_t *flash_param = flash_param_get();
  int ipx[4], done=0, j;
  int t;
  token_t queue[MAX_TOKENS];
  char buf[60]={0};
    
  t = tokenise(cmd,queue);
  
	if (t == 0) {
      ipdetails(buf);
      uart0_sendStr(buf);
     return;
	} 
  if (t == 3) {
      // ip
      get_token(buf, queue, 0);
		  if(str2ip(ipx, buf)) {
          for(j=0;j<4;j++) flash_param->ip[j] = ipx[j];
          done++;
      }
      // gw
      get_token(buf, queue, 1);
  		if(str2ip(ipx, buf)) {
          for(j=0;j<4;j++) flash_param->gw[j] = ipx[j];
          done++;
      }
      // mask
      get_token(buf, queue, 2);
  		if(str2ip(ipx, buf)) {
          for(j=0;j<4;j++) flash_param->netmask[j] = ipx[j];
          done++;
      }
      // if okay wtite it up
      if(done==3){
          uart0_sendStr(MSG_OK);
          flash_param_set();
          os_delay_us(10000);
          system_restart();
      } else {
          uart0_sendStr(MSG_ERROR);
      }
	} else
    uart0_sendStr(MSG_ERROR);
}  

// *****************************************************************************
// STA
// spaces are not supported in the ssid or password
// *****************************************************************************
void ICACHE_FLASH_ATTR u_sta(char *cmd) 
{
	struct station_config sta_conf;
  int t;
  token_t queue[MAX_TOKENS];
  char buf[60]={0};
    
  t = tokenise(cmd,queue);
  
	os_bzero(&sta_conf, sizeof(struct station_config));
	wifi_station_get_config(&sta_conf);

	if (t == 0) {
      os_sprintf(buf, "SSID=%s PASSWORD=%s\r\n"MSG_OK, sta_conf.ssid, sta_conf.password);
      uart0_sendStr(buf);
	 } else if (t != 2)
		uart0_sendStr( MSG_ERROR);
	else {
    get_token(buf, queue, 0); // ssid
		os_strncpy(sta_conf.ssid, buf, sizeof(sta_conf.ssid));
    get_token(buf, queue, 1); // pasword
		os_strncpy(sta_conf.password, buf, sizeof(sta_conf.password));
		uart0_sendStr(MSG_OK);
		wifi_station_disconnect();
		ETS_UART_INTR_DISABLE();
		wifi_station_set_config(&sta_conf);
		ETS_UART_INTR_ENABLE();
		wifi_station_connect();
	}
}

// *****************************************************************************
// AP
// spaces are not supported in the ssid or password
// *****************************************************************************
void ICACHE_FLASH_ATTR u_ap(char *cmd) 
{
#define MAXAUTHMODES 5
	struct softap_config ap_conf;
  int t;
  token_t queue[MAX_TOKENS];
  char buf[80]={0};
    
  t = tokenise(cmd,queue);
  
	os_bzero(&ap_conf, sizeof(struct softap_config));
	wifi_softap_get_config(&ap_conf);
  
	if (t == 0) {
      char buf[80];
      os_sprintf(buf, "SSID=%s PASSWORD=%s AUTHMODE=%d CHANNEL=%d\r\n"MSG_OK, ap_conf.ssid, ap_conf.password, ap_conf.authmode, ap_conf.channel);
      uart0_sendStr(buf);   
	} else if (t > 4)
		uart0_sendStr(MSG_ERROR);
	else { //argc > 0
    get_token(buf, queue, 0); // ssid
		os_strncpy(ap_conf.ssid, buf, sizeof(ap_conf.ssid));
		ap_conf.ssid_len = strlen(buf); //without set ssid_len, no connection to AP is possible
		if (t == 1) { //  no password
			os_bzero(ap_conf.password, sizeof(ap_conf.password));
			ap_conf.authmode = AUTH_OPEN;
		} else { // with password
      get_token(buf, queue, 1); // password
			os_strncpy(ap_conf.password, buf, sizeof(ap_conf.password));
			if (t > 2) { // authmode
        get_token(buf, queue, 2);
				int amode = atoi(buf);  
				if ((amode < 1) || (amode>4)) {
					uart0_sendStr(MSG_ERROR);
					return;
				}
				ap_conf.authmode = amode;
			}
			if (t > 3) { //channel
        get_token(buf, queue, 3);
				int chan = atoi(buf);
				if ((chan < 1) || (chan>13)){
					uart0_sendStr(MSG_ERROR);
					return;
				}
				ap_conf.channel = chan;
			}
		}
		uart0_sendStr(MSG_OK);
		ETS_UART_INTR_DISABLE();
		wifi_softap_set_config(&ap_conf);
		ETS_UART_INTR_ENABLE();
	}
}

// *****************************************************************************
// FLASH
// *****************************************************************************
void ICACHE_FLASH_ATTR u_flash(char *cmd) 
{
  int t;
  token_t queue[MAX_TOKENS];
  char buf[80]={0};
    
  t = tokenise(cmd,queue);
  
	bool err = false;
	if (t == 0) {
      pnum("FLASH=%d\r\n", doflash);
	} else if (t != 1)
		err=true;
	else {
    get_token(buf, queue, 0);
		if (strcmp(buf, "1") == 0)
			doflash = true;
		else if (strcmp(buf, "0") == 0)
			doflash = false;
		else
			err=true;
	}
	if (err)
		uart0_sendStr(MSG_ERROR);
	else
		uart0_sendStr(MSG_OK);
}

// *****************************************************************************
// GPIO
// *****************************************************************************
void ICACHE_FLASH_ATTR u_gpio2(char *cmd) 
{
  int t;
  token_t queue[MAX_TOKENS];
  char buf[80]={0};
    
  t = tokenise(cmd,queue);
  
	if (t == 0)
      uart0_sendStr("Args: 0=low, 1=high, 2 <delay in ms>=reset (delay optional).\r\n"); 
	else {
		uint32_t gpiodelay = 100;
		if (t == 2) {
      get_token(buf, queue, 1);
			gpiodelay = atoi(buf);
		}
    get_token(buf, queue, 0);
		uint8_t gpio = atoi(buf);
		if (gpio < 3) {
			if (gpio == 0) {
				gpio_output_set(0, BIT2, BIT2, 0);
				uart0_sendStr("LOW\r\n");
			}
			if (gpio == 1) {
				gpio_output_set(BIT2, 0, BIT2, 0);
				uart0_sendStr("HIGH\r\n");
			}
			if (gpio == 2) {
				gpio_output_set(0, BIT2, BIT2, 0);
				os_delay_us(gpiodelay*1000);
				gpio_output_set(BIT2, 0, BIT2, 0);
				pnum("RESET %d ms\r\n",gpiodelay); 
			}
		} else {
			uart0_sendStr(MSG_ERROR);
		}
	}
}

// *****************************************************************************
// test
// *****************************************************************************
void ICACHE_FLASH_ATTR u_test(char *cmd)
{
    int i,t;
    token_t queue[MAX_TOKENS];
    char buf[60]={0};
    
    t = tokenise(cmd,queue);
    
    pnum("\nntoken=%d",t);
    for (i = 0; i < t; i++) {
        pnum("\ntoken %d\t",i);
        pnum("len=%d\t",queue[i].len);
        //strncpy(temp,tq[i].s,tq[i].len);
        //temp[tq[i].len]=0;
        get_token(buf, queue, i);
        uart0_sendStr(buf); 
    } 
}

