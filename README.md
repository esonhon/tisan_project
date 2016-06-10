# tisan_project
project for customer
####OTA升级接口说明

命令ID： 65529 

参数1(升级文件url地址)： 数据类型：TLV_TYPE_URI 

说明：esp8266的boot将flash分为两个区，升级是将固件下到另一个分区，然后修改flash中的标志位，让系统从另一个分区启动。同一套代码，配置成不同的分区，生成的固件也是不一样的，所以，需要在云端放置两份固件，一份是分区1的，一份是分区2的，tisan固件会根据自己跑在哪个分区，自动选择下载另一个分区的固件，所以ual地址只需要给到文件名的前面部分，例如：实际的OTA文件地址为：http://pandocloud.com/ota/esp8266/user1.bin,该参数只需要给http://pandocloud.com/ota/esp8266/，后面的文件名user1.bin只是给一个例子，实际需修改固件中的两处文件名，使他们与服务器上的保持一致。

```
if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
{
  PRINTF("download user2\n");
  os_memcpy(user_bin, "esp8266_512_plug_111_user2.bin", 31);
}
else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
{
   PRINTF("download user1\n");
   os_memcpy(user_bin, "esp8266_512_plug_111_user1.bin", 31); \\根据实际修改此处文件名
}
if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
{
  PRINTF("download user2\n");
  os_memcpy(user_bin, "esp8266_512_plug_111_user2.bin", 31); \\根据实际修改此处文件名
}
else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
{
  PRINTF("download user1\n");
  os_memcpy(user_bin, "esp8266_512_plug_111_user1.bin", 31);
}
```

参数2（升级文件的MD5校验值）：数据类型：TLV_TYPE_BYTES
