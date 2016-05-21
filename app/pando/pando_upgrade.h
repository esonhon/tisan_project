/*******************************************************
 * File name: pando_upgrade.h
 * Author:Chongguang Li
 * Versions:1.0
 * Description:This module is to used to update the gateway programme through ota.
 * History:
 *   1.Date:
 *     Author:
 *     Modification:    
 *********************************************************/
 
 #ifndef PANDO_UPGRADE_H_
 #define PANDO_UPGRADE_H_

typedef enum {
    UPGRADE_OK = 0,
    UPGRADE_FAIL  // http request failed.
} UPGRADDE_RESULT;
 
 typedef void (* upgrade_callback)(UPGRADDE_RESULT result);
/******************************************************************************
 * FunctionName : gateway_upgrade_begin
 * Description  : update the gateway programme through ota.
 * Parameters   : the download url.
 * Returns      : none
*******************************************************************************/
void gateway_upgrade_begin(char* ota_url, char*md5, upgrade_callback callback);

#endif
