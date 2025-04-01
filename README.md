# greenhouse

## Pulling partition table off existing device

 1. make sure pio cli works (enjoy python version confusion on Ubuntu)
 2. python ${HOME}/.platformio/packages/tool-esptoolpy/esptool.py --port /dev/ttyUSB0 --baud 115200 read_flash 0x8000 0xC00 partition-table.bin
 3. python ${HOME}/.platformio/packages/framework-espidf/components/partition_table/gen_esp32part.py  partition-table.bin
 4. Setting in platfomio.ini (partition table, which file system to send data)
 5. Extra tuning: script to compress from data_src/ to data/ (because spiffs is just 128k and 
    to .gz is right anyway)
