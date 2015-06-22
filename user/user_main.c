/*
 * File	: user_main.c
 * This file is part of Espressif's AT+ command set program.
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.	If not, see <http://www.gnu.org/licenses/>.
 */
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "driver/uart.h"
#include "task.h"

#include "server.h"
#include "config.h"
#include "setup.h"
#include "flash_param.h"
#include "user_interface.h"

os_event_t		recvTaskQueue[recvTaskQueueLen];
extern  serverConnData connData[MAX_CONN];

#define MAX_UARTBUFFER (MAX_TXBUFFER/4)
static uint8 uartbuffer[MAX_UARTBUFFER];

// *****************************************************************************
// sends string to all connected remote devices
// *****************************************************************************
void remote_output(char *s, uint16_t len)
{
  int i;
		for (i = 0; i < MAX_CONN; ++i) {
			if (connData[i].conn) { 
				espbuffsent(&connData[i], s, len);
      }
    }
}

// *****************************************************************************
// Receives input from the console and deals with it here
// a command is esc[nn..] <command>
// where esc must be at the start of a line and followed by [
// state 0 
// *****************************************************************************
void console_input(os_event_t *events)
{
  static char incmd=0, state=0, idx=0; // +++ st astrt of line
  static char cmd[128]={"    "}, cn;
  int j, c;
  char tmp[2];
  
  while(READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {
      WRITE_PERI_REG(0X60000914, 0x73); //WTD
      c = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
    
      //tmp[0]=c;tmp[1]=0;uart0_sendStr(tmp);// echo

      //pnum("\tstate=%d\r\n",state);
      // state < 3 is normal data, only state 2 will accept esc[
      if(state < 3) {
        //tmp[0]=c;tmp[1]=0;uart0_sendStr(tmp);// echo
        tmp[0]=c;tmp[1]=0;remote_output(tmp, 1);  /// send remote
        if((c == '\r') || (c == '\n')) { state = 2; idx=0; } 
        if(idx > 2) {
          state = 0;
          idx = 0;
          for(j=0;j<6;j++) cmd[j]=0;
        }
        if(state == 2) cmd[idx++] = c; 
        //pnum("1=%x",cmd[1]);pnum("2=%x",cmd[2]);
        if((cmd[1]==27) && (cmd[2]=='[')) state = 3;         
      }
    
      // command interpreter
      if(state==5) {
          tmp[0]=c;tmp[1]=0;uart0_sendStr(tmp);// echo
          if((c == '\r') || (c == '\n')) {        
            int t;        
            cmd[idx]=0; // terminate         
            switch(cn) {
              case RESET: { u_reset(); break; } 
              case TEST: { u_test(cmd); break; }
              case BAUD: { u_baud(cmd); break; }
              case PORT: { u_port(cmd); break; }
              case IP: { u_ip(cmd); break; }
              case MODE: { u_setMode(cmd); break; }
              case STA: { u_sta(cmd); break; }
              case AP: { u_ap(cmd); break; }
              case FLASH: { u_flash(cmd); break; }
              case GPIO2: { u_gpio2(cmd); break; }
              case INFO: { setup_info(); break; }
            }
            for(j=0;j<128;j++) cmd[j]=0; // clear buffer
            idx=0;
            state=0;
            break; // just in case          
          }
          cmd[idx++] = c;
      }
      // at this point we have receive the command
      if(state==4) {
          cn = c; // this is command
          tmp[0]=c;tmp[1]=0;uart0_sendStr(tmp);// echo
          uart0_sendStr("*");
          for(j=0;j<128;j++) cmd[j]=0; // clear buffer
          idx=0;
          state = 5;
      }
      // begins gathering command number, we have recieved esc[
      // next char will be the command
      if(state==3) {
        tmp[0]=c;tmp[1]=0;uart0_sendStr(tmp);// echo
        state = 4;    
      }
  }
  // finished
  WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	if(UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST))
	{
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	}
	else if(UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST))
	{
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}
	ETS_UART_INTR_ENABLE();
  
}


// UartDev is defined and initialized in rom code.
extern UartDevice    UartDev;

void user_init(void)
{

	uint8_t i;
  struct ip_info ipinfo;
//  char buf[80]={0};
	//wifi_set_opmode(3); //STA+AP

	#ifdef CONFIG_DYNAMIC
		flash_param_t *flash_param;
		flash_param_init();
		flash_param = flash_param_get();
		UartDev.data_bits = GETUART_DATABITS(flash_param->uartconf0);
		UartDev.parity = GETUART_PARITYMODE(flash_param->uartconf0);
		UartDev.stop_bits = GETUART_STOPBITS(flash_param->uartconf0);
//		uart_init(flash_param->baud, BIT_RATE_115200);
	#else
		UartDev.data_bits = EIGHT_BITS;
		UartDev.parity = NONE_BITS;
		UartDev.stop_bits = ONE_STOP_BIT;
		uart_init(BIT_RATE_115200, BIT_RATE_115200);
	#endif
//	os_printf("size flash_param_t %d\n", sizeof(flash_param_t));


	#ifdef CONFIG_STATIC
		// refresh wifi config
		config_execute();
	#endif

	#ifdef CONFIG_DYNAMIC
		serverInit(flash_param->port);
	#else
		serverInit(23);
	#endif

		config_gpio();

//   wifi_station_dhcpc_stop();
//   os_delay_us(10000);
  IP4_ADDR(&ipinfo.ip, flash_param->ip[0],flash_param->ip[1],flash_param->ip[2],flash_param->ip[3]);
  IP4_ADDR(&ipinfo.gw, flash_param->gw[0],flash_param->gw[1],flash_param->gw[2],flash_param->gw[3]);
  IP4_ADDR(&ipinfo.netmask, flash_param->netmask[0],flash_param->netmask[1],flash_param->netmask[2],flash_param->netmask[3]);
  wifi_set_ip_info(STATION_IF,&ipinfo);
  os_delay_us(1000000);  // 
//	uart0_sendStr("\r\n\r\n");
//  uart0_sendStr("TBREADY:\r\n");
//  os_delay_us(50000);  // 

// set up uart AFTER tasker
	system_os_task(console_input, recvTaskPrio, recvTaskQueue, recvTaskQueueLen);
  uart_init(flash_param->baud, BIT_RATE_115200);
	uart0_sendStr("\r\n\r\n");
  uart0_sendStr("TBREADY:\r\n");
}
