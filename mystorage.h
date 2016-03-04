/** MyStorage: a StorageInterface class to interface SPI flash memory and USBLocalFilesystem
*/
#include "mbed.h"
#include "StorageInterface.h"
#include "W25X40BV.h"
/** MyStorage: a StorageInterface class to interface SPI flash memory and USBLocalFilesystem
It uses W25X40BV library also hints from Sissors/code/S25FL216K_USBFileSystem
- USBLocalFileSystem by va009039/USBLocalFileSystem
- W25X40BV by k4zuki/code/W25X40BV forked from jyam/code/W25X40BV
*/
class MyStorage : public StorageInterface {
public:
    MyStorage(PinName mosi, PinName miso, PinName sclk, PinName cs);

    virtual int storage_read(uint8_t* data, uint32_t block);
    virtual int storage_write(const uint8_t* data, uint32_t block);
    virtual uint32_t storage_sectors();
    virtual uint32_t storage_size();

private:
    W25X40BV _flash;
    uint64_t _sectors;
};
