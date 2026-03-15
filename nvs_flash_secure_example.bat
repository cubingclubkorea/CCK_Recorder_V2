@echo off
python C:\Users\{UserName}\esp\v5.4.3\esp-idf\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py generate nvs_custom.csv nvs_custom.bin 0x3000
python -m esptool --port {PORT_NUM} write_flash 0x17000 nvs_custom.bin
python C:\Users\{UserName}\esp\v5.4.3\esp-idf\components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py generate nvs_auth.csv nvs_auth.bin 0x3000
python -m esptool --port {PORT_NUM} write_flash 0x1A000 nvs_auth.bin
pause
