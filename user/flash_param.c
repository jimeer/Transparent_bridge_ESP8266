#include "user_interface.h"
#include "spi_flash.h"
#include "ets_sys.h"
#include "c_types.h"
#include "driver/uart.h"
#include "flash_param.h"

#define FLASH_PARAM_START_SECTOR 0x3C
#define FLASH_PARAM_SECTOR (FLASH_PARAM_START_SECTOR + 0)
#define FLASH_PARAM_ADDR (SPI_FLASH_SEC_SIZE * FLASH_PARAM_SECTOR)

static int flash_param_loaded = 0;
static flash_param_t flash_param;

void ICACHE_FLASH_ATTR flash_param_read(flash_param_t *flash_param) {
	spi_flash_read(FLASH_PARAM_ADDR, (uint32 *)flash_param, sizeof(flash_param_t));
}

void ICACHE_FLASH_ATTR flash_param_write(flash_param_t *flash_param) {
	ETS_UART_INTR_DISABLE();
	spi_flash_erase_sector(FLASH_PARAM_SECTOR);
	spi_flash_write(FLASH_PARAM_ADDR, (uint32 *) flash_param, sizeof(flash_param_t));
	ETS_UART_INTR_ENABLE();
}

flash_param_t *ICACHE_FLASH_ATTR flash_param_get(void) {
	if (!flash_param_loaded) {
		flash_param_read(&flash_param);
		flash_param_loaded = 1;
	}
	return &flash_param;
}

int ICACHE_FLASH_ATTR flash_param_set(void) {
	flash_param_write(&flash_param);
	flash_param_t tmp;
	flash_param_read(&tmp);
	if (memcmp(&tmp, &flash_param, sizeof(flash_param_t)) != 0) {
		return 0;
	}
	return 1;
}

void ICACHE_FLASH_ATTR flash_param_init_defaults(void) {	
	flash_param_t *flash_param = flash_param_get();
	flash_param->magic = FLASH_PARAM_MAGIC;
	flash_param->version = FLASH_PARAM_VERSION;
	flash_param->baud = 115200;
	flash_param->port = FLASH_PARAM_PORT;
	flash_param->uartconf0 = CALC_UARTMODE(EIGHT_BITS, NONE_BITS, ONE_STOP_BIT);
  flash_param->UseDHCP=1;
  flash_param->ip[0]=192;
  flash_param->ip[1]=168;
  flash_param->ip[2]=11;
  flash_param->ip[3]=30;
  flash_param->gw[0]=192;
  flash_param->gw[1]=168;
  flash_param->gw[2]=11;
  flash_param->gw[3]=50;
  flash_param->netmask[0]=255;
  flash_param->netmask[1]=255;
  flash_param->netmask[2]=255;
  flash_param->netmask[3]=0;
	flash_param_set();
}

flash_param_t* ICACHE_FLASH_ATTR flash_param_init(void) {
//spi_flash_erase_sector(FLASH_PARAM_SECTOR);
	flash_param_t *flash_param = flash_param_get();
	if (flash_param->magic != FLASH_PARAM_MAGIC || flash_param->version != FLASH_PARAM_VERSION) {
		flash_param_init_defaults();
	}
	return flash_param;
}
