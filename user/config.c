
// this is the normal build target ESP include set
#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "driver/uart.h"

#include "flash_param.h"
#include "server.h"
#include "config.h"

void config_gpio(void) {
	// Initialize the GPIO subsystem.
	gpio_init();
	//Set GPIO2 to output mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	//Set GPIO2 high
	gpio_output_set(BIT4, 0, BIT4, 0);
}

// *****************************************************************************
// send reset via GPIO 4
// *****************************************************************************
void ICACHE_FLASH_ATTR remote_reset()
{
  gpio_output_set(0, BIT4, BIT4, 0); // low
  os_delay_us(10000);
  gpio_output_set(BIT4, 0, BIT4, 0); // high
}

#ifdef CONFIG_STATIC

void config_execute(void) {
	uint8_t mode;
	struct station_config sta_conf;
	struct softap_config ap_conf;
	uint8_t macaddr[6] = { 0, 0, 0, 0, 0, 0 };

	// make sure the device is in AP and STA combined mode
	mode = wifi_get_opmode();
	if (mode != STATIONAP_MODE) {
		wifi_set_opmode(STATIONAP_MODE);
		os_delay_us(10000);
		system_restart();
	}

	// connect to our station
	os_bzero(&sta_conf, sizeof(struct station_config));
	wifi_station_get_config(&sta_conf);
	os_strncpy(sta_conf.ssid, STA_SSID, sizeof(sta_conf.ssid));
	os_strncpy(sta_conf.password, STA_PASSWORD, sizeof(sta_conf.password));
	wifi_station_disconnect();
	ETS_UART_INTR_DISABLE();
	wifi_station_set_config(&sta_conf);
	ETS_UART_INTR_ENABLE();
	wifi_station_connect();

	// setup the soft AP
	os_bzero(&ap_conf, sizeof(struct softap_config));
	wifi_softap_get_config(&ap_conf);
	wifi_get_macaddr(SOFTAP_IF, macaddr);
	os_strncpy(ap_conf.ssid, AP_SSID, sizeof(ap_conf.ssid));
	ap_conf.ssid_len = strlen(AP_SSID);
	os_strncpy(ap_conf.password, AP_PASSWORD, sizeof(ap_conf.password));
	//os_snprintf(&ap_conf.password[strlen(AP_PASSWORD)], sizeof(ap_conf.password) - strlen(AP_PASSWORD), "_%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
	os_sprintf(ap_conf.password[strlen(AP_PASSWORD)], "_%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
	ap_conf.authmode = AUTH_WPA_PSK;
	ap_conf.channel = 6;
	ETS_UART_INTR_DISABLE();
	wifi_softap_set_config(&ap_conf);
	ETS_UART_INTR_ENABLE();
}

#endif

#ifdef CONFIG_DYNAMIC

#define MSG_OK "OK\r\n"
#define MSG_ERROR "ERROR\r\n"
#define MSG_INVALID_CMD "UNKNOWN COMMAND\r\n"

#define MAX_ARGS 12
#define MSG_BUF_LEN 128


char *my_strdup(char *str) {
	size_t len;
	char *copy;

	len = strlen(str) + 1;
	if (!(copy = os_malloc((u_int)len)))
		return (NULL);
	os_memcpy(copy, str, len);
	return (copy);
}

char **config_parse_args(char *buf, uint8_t *argc) {
	const char delim[] = " \t";
	char *save, *tok;
	char **argv = (char **)os_malloc(sizeof(char *) * MAX_ARGS);	// note fixed length
	os_memset(argv, 0, sizeof(char *) * MAX_ARGS);

	*argc = 0;
	for (; *buf == ' ' || *buf == '\t'; ++buf); // absorb leading spaces
	for (tok = strtok_r(buf, delim, &save); tok; tok = strtok_r(NULL, delim, &save)) {
		argv[*argc] = my_strdup(tok);
		(*argc)++;
		if (*argc == MAX_ARGS) {
			break;
		}
	}
	return argv;
}

void config_parse_args_free(uint8_t argc, char *argv[]) {
	uint8_t i;
	for (i = 0; i <= argc; ++i) {
		if (argv[i])
			os_free(argv[i]);
	}
	os_free(argv);
}

void allsentstring(serverConnData *conn, char *s)
{
  if(conn) {
    espbuffsentstring(conn, s);
  } else {
    uart0_sendStr(s);
  }
}

const config_commands_t config_commands[] = {
// 		{ "RESET", &config_cmd_reset },
// 		{ "BAUD", &config_cmd_baud },
// 		{ "PORT", &config_cmd_port },
// 		{ "IP", &config_cmd_ip },    
// 		{ "MODE", &config_cmd_mode },
// 		{ "STA", &config_cmd_sta },
//		{ "AP", &config_cmd_ap },
//		{ "FLASH", &config_cmd_flash },
//		{ "GPIO2", &config_cmd_gpio2 },
//		{ "TEST", &config_cmd_test },
		{ NULL, NULL }
	};

void config_parse(serverConnData *conn, char *buf, int len) {
	char *lbuf = (char *)os_malloc(len + 1), **argv;
	uint8_t i, argc;
	// we need a '\0' end of the string
	os_memcpy(lbuf, buf, len);
	lbuf[len] = '\0';

	// command echo
	//espbuffsent(conn, lbuf, len);

	// remove any CR / LF
	for (i = 0; i < len; ++i)
		if (lbuf[i] == '\n' || lbuf[i] == '\r')
			lbuf[i] = '\0';

	// verify the command prefix
	if (os_strncmp(lbuf, "+++AT", 5) != 0) {
		return;
	}
	// parse out buffer into arguments
	argv = config_parse_args(&lbuf[5], &argc);

	if (argc == 0) {
		allsentstring(conn, MSG_OK);
	} else {
		argc--;	// to mimic C main() argc argv
		for (i = 0; config_commands[i].command; ++i) {
			if (os_strncmp(argv[0], config_commands[i].command, strlen(argv[0])) == 0) {
				config_commands[i].function(conn, argc, argv);
				break;
			}
		}
		if (!config_commands[i].command)
			allsentstring(conn, MSG_INVALID_CMD);
	}
	config_parse_args_free(argc, argv);
	os_free(lbuf);
}

#endif
