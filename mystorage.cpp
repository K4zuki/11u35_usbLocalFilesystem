#include "mbed.h"
#include "mystorage.h"
#include "W25X40BV.h"

// MyStorage(PinName mosi, PinName miso, PinName sclk, PinName cs);
MyStorage::MyStorage(PinName mosi, PinName miso, PinName sclk, PinName cs) :
    _flash(mosi, miso, sclk, cs) {

    //storage_initialize();
    }

// virtual int storage_read(uint8_t* data, uint32_t block);
int MyStorage::storage_read(uint8_t *buffer, uint32_t block_number) {
    report_read_count++;
    
    // receive the data
//    _read(buffer, 512);
//    _flash.readStream(uint32_t addr, uint8_t* buf, uint32_t count);
    _flash.readStream(block_number*256, buffer, 512);
    return 0;
}

// virtual int storage_write(const uint8_t* data, uint32_t block);
int MyStorage::storage_write(const uint8_t *buffer, uint32_t block_number) {
    report_write_count++;
    
    // send the data block
//    _write(buffer, 512);
//    writeStream(uint32_t addr, uint8_t* buf, uint32_t count);
//    void pageErase(uint8_t page);
    _flash.pageErase(block_number);
    _flash.pageErase(block_number + 1);
    _flash.writeStream(block_number * 256, (uint8_t*)buffer, 256);
    _flash.writeStream(block_number * 256 + 256, (uint8_t*)(buffer + 256), 256);
    return 0;
}

// virtual uint32_t storage_sectors();
uint32_t MyStorage::storage_sectors() { 
    report_sectors_count++;
    return 256; //256*512
}

// virtual uint32_t storage_size();
uint32_t MyStorage::storage_size()
{
    report_size_count++;
    return 128*1024; //256*512
}
