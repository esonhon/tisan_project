/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Martin d'Allens <martin.dallens@gmail.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

// FIXME: sprintf->snprintf everywhere.
// FIXME: support null characters in responses.

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "../upgrade/upgrade.h"
#include "pando_upgrade.h"
#include "gateway/pando_storage_interface.h"
#include "../upgrade/upgrade.h"
#include "c_types.h"
#include "../util/converter.h"

// Suport different Content-Type header
//#define HTTP_HEADER_CONTENT_TYPE "application/json"


#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

//Internal state.

typedef struct {
	char * path;
	int port;
	char * post_data;
	char * hostname;
} request_args;

static upgrade_callback user_upgrade_callback;

static uint8 md5_string[16] = {0};

static char * ICACHE_FLASH_ATTR my_strdup(const char * str)
{
	if(str == NULL)
	{
		return NULL;
	}
	char * new_str = (char *)os_malloc(os_strlen(str) + 1 /*null character*/);
	if (new_str == NULL)
    {
		return NULL;
    }
	os_strcpy(new_str, str);
	return new_str;
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : upgrade callback function.
 * Parameters   : arg: the callback parameter.
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;
    //struct espconn *pespconn = server->pespconn;
    
    if (server->upgrade_flag == true) 
    {
        PRINTF("upgrade successfully\n");

        pando_storage_clean(); // clean the pando configuration message, device re-register.

        system_upgrade_reboot(); //reboot.
    } 

    else 
    {
        PRINTF("upgrade failed");

        user_upgrade_callback(UPGRADE_FAIL);
    }

    if(server->url!=NULL)
    {
        os_free(server->url);
    }

    if(server!=NULL)
    {
        os_free(server);
    }

}

static void ICACHE_FLASH_ATTR dns_callback(const char * hostname, ip_addr_t * addr, void * arg)
{
	request_args * req = (request_args *)arg;

	if (addr == NULL) 
    {
		PRINTF("DNS failed %s\n", hostname);
		if (user_upgrade_callback != NULL)
        { 
            // Callback is optional.
			user_upgrade_callback(UPGRADE_FAIL);
		}
	}
	else 
    {
		PRINTF("DNS found %s " IPSTR "\n", hostname, IP2STR(addr));
        
        struct upgrade_server_info* server = NULL;
        server = (struct upgrade_server_info *)os_malloc(sizeof(struct upgrade_server_info));
        PRINTF("server:%d\n", server);
        os_memcpy(server->ip, addr, 4);
        server->port = 80;
        server->check_cb = user_esp_platform_upgrade_rsp;
        server->check_times = 300000;
        server->url = (uint8 *)os_malloc(512);
        os_memcpy(server->md5, md5_string, 16);

        uint8 user_bin[31] = {0};

        if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
        {
        	PRINTF("download user2\n");
        	os_memcpy(user_bin, "esp8266_512_plug_111_user2.bin", 31);
        }

        else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
        {
        	PRINTF("download user1\n");
            os_memcpy(user_bin, "esp8266_512_plug_111_user1.bin", 31);
        }

        os_sprintf(server->url, "GET %s%s HTTP/1.1\r\nHost: %s\r\n"pheadbuffer"", req->path,\
                  user_bin, req->hostname);
        PRINTF("url:%s", server->url);

        if(pando_system_upgrade_start(server) == false)
        {
        	PRINTF("upgrade is already started\n");
        }
    }
    if(req->hostname!=NULL)
    {
        os_free(req->hostname);
    }
    if(req->path!=NULL)
    {
        os_free(req->path);
    }
    if(req->post_data!=NULL)
    {
        os_free(req->post_data);
    }
    if(req!=NULL)
    {
        os_free(req);
    }
    
}

static void ICACHE_FLASH_ATTR 
http_raw_request(const char * hostname, int port, const char * path, const char * post_data)
{
	PRINTF("DNS request\n");

	request_args * req = (request_args *)os_malloc(sizeof(request_args));
	req->hostname = my_strdup(hostname);
	req->path = my_strdup(path);
	req->port = port;
	req->post_data = my_strdup(post_data);
    
	ip_addr_t addr;
	err_t error = espconn_gethostbyname((struct espconn *)req, // It seems we don't need a real espconn pointer here.
										hostname, &addr, dns_callback);

	if (error == ESPCONN_INPROGRESS) 
    {
		PRINTF("DNS pending\n");
	}
	else if (error == ESPCONN_OK) 
    {
		// Already in the local names table (or hostname was an IP address), execute the callback ourselves.
		dns_callback(hostname, &addr, req);
	}
	else if (error == ESPCONN_ARG) 
    {
		PRINTF("DNS error %s\n", hostname);
	}
	else 
    {
		PRINTF("DNS error code %d\n", error);
	}
}

/*
 * Parse an URL of the form http://host:port/path
 * <host> can be a hostname or an IP address
 * <port> is optional
 */
static void ICACHE_FLASH_ATTR 
http_post(char* url, const char * post_data)
{
	// FIXME: handle HTTP auth with http://user:pass@host/
	// FIXME: make https work.
	// FIXME: get rid of the #anchor part if present.

	char hostname[128] = "";
	int port = 80;

	if (os_strncmp(url, "http://", os_strlen("http://")) != 0) 
    {
		PRINTF("URL is not HTTP %s\n", url);
		return;
	}
	url += os_strlen("http://"); // Get rid of the protocol.

	char * path = os_strchr(url, '/');
	if (path == NULL) 
    {
		path = os_strchr(url, '\0'); // Pointer to end of string.
	}

	char * colon = os_strchr(url, ':');
	if (colon > path) 
    {
		colon = NULL; // Limit the search to characters before the path.
	}

	if (colon == NULL) 
    { 
        // The port is not present.
		os_memcpy(hostname, url, path - url);
		hostname[path - url] = '\0';
	}
	else 
    {
		port = atoi(colon + 1);
		if (port == 0) 
        {
			PRINTF("Port error %s\n", url);
			return;
		}

		os_memcpy(hostname, url, colon - url);
		hostname[colon - url] = '\0';
	}
    
	if (path[0] == '\0') 
    {
        // Empty path is not allowed.
		path = "/";
	}

	PRINTF("hostname=%s\n", hostname);
	PRINTF("port=%d\n", port);
	PRINTF("path=%s\n", path);
	http_raw_request(hostname, port, path, post_data);
}

void ICACHE_FLASH_ATTR gateway_upgrade_begin(char* ota_url, char* md5, upgrade_callback callback)
{
	if(callback != NULL)
	{
		user_upgrade_callback = callback;
	}

	//md5 format: {user1::user2};
	char md5_str[33] = {0};
	hex2bin(md5_str, md5);
	if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
	{
	    os_memcpy(md5_string, md5_str + 17, 16);
	}

	else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
	{
	    os_memcpy(md5_string, md5_str, 16);
	}

	int i = 0;
	for (i = 0; i < 16; i++)
	PRINTF("%02x", md5_string[i]);
	PRINTF("\n");

	http_post(ota_url, NULL);
}
