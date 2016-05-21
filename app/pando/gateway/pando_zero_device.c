/*******************************************************
 * File name: pando_zero_device.c
 * Author:  Chongguang Li
 * Versions: 1.0
 * Description:This module is used to process the gateway business.
 * History:
 *   1.Date:
 *     Author:
 *     Modification:
 *********************************************************/
#include "pando_channel.h"
#include "../protocol/sub_device_protocol.h"
#include "pando_system_time.h"
#include "pando_zero_device.h"
#include "user_interface.h"
#include "../pando_upgrade.h"

#define COMMON_COMMAND_UPGRADE 65529
#define COMMON_COMMAND_REBOOT  65535
#define COMMON_COMMAND_SYN_TIME 65531

static void ICACHE_FLASH_ATTR
pando_plantform_upgrade(UPGRADDE_RESULT result)
{

	if(UPGRADE_FAIL == result)
	{
		// TODO
	}
}

/******************************************************************************
 * FunctionName : zero_device_data_process.
 * Description  : process the received data of zero device(zero device is the gateway itself).
 * Parameters   : uint.
 * Returns      : none.
*******************************************************************************/
static void ICACHE_FLASH_ATTR
zero_device_data_process(uint8_t * buffer, uint16_t length)
{
	struct pando_command cmd_body;
	uint16_t type = 0;
	uint16_t len = 0;
	struct sub_device_buffer * device_buffer = (struct sub_device_buffer *)os_malloc(sizeof(struct sub_device_buffer));
	if(device_buffer == NULL)
	{
		PRINTF("%s:malloc error!\n", __func__);
		return;
	}
	device_buffer->buffer = (uint8*)os_malloc(length);
	if(device_buffer->buffer == NULL)
	{
		PRINTF("%s:malloc error!\n", __func__);
		return;
	}
	os_memcpy(device_buffer->buffer, buffer, length);
	device_buffer->buffer_length = length;

	struct TLVs *cmd_param = get_sub_device_command(device_buffer, &cmd_body);
	if(COMMON_COMMAND_SYN_TIME == cmd_body.command_id )
	{
		PRINTF("PANDO: synchronize time\n");
		uint64 time = get_next_uint64(cmd_param);
	    show_package((uint8*)(&time), sizeof(time));
	    pando_set_system_time(time);
	}
	else if(COMMON_COMMAND_UPGRADE == cmd_body.command_id)
	{
		PRINTF("receive ota upgrade command\n");

	    char  ota_url[128] = {0};
	    char  md5_string[128] = {0};
	    uint16_t url_length, md5_length;
		//"http://static.pandocloud.com/ota/esp8266/ 10.116.111.165:20301
	    //865b1929c74a2a62e6921f754568604d::96faa38e938d5dfb15c68c1b1890e475

	    char *temp = get_next_uri(cmd_param, &url_length);
	    os_memcpy(ota_url, temp, length);

	    ota_url[length + 1] = 0;
	    PRINTF("ota_url: %s\n", ota_url);

	    temp = get_next_bytes(cmd_param, &md5_length);
	    os_memcpy(temp, md5_string, md5_length);
	    md5_string[length+1] = '\0';
	    PRINTF("md5:%s\n", md5_string);
	    //espconn_secure_disconnect(access_conn);
	    gateway_upgrade_begin(ota_url, md5_string,  pando_plantform_upgrade);
	}

	if(device_buffer != NULL)
	{
		if(device_buffer->buffer != NULL)
		{
			os_free(device_buffer->buffer);
			device_buffer->buffer = NULL;
		}

		os_free(device_buffer);
		device_buffer = NULL;
	}
}


/******************************************************************************
 * FunctionName : ipando_zero_device_init
 * Description  : initialize the zero device(zero device is the gateway itself).
 * Parameters   : none.
 * Returns      : none.
*******************************************************************************/
void ICACHE_FLASH_ATTR
pando_zero_device_init(void)
{
	on_subdevice_channel_recv(PANDO_CHANNEL_PORT_0, zero_device_data_process);
}
