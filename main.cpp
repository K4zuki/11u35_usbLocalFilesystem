#include "mbed.h"
#include "USBLocalFileSystem.h"
#include "USBDAP.h"
#include "BaseDAP.h"
#include "USB_HID.h"
#include "DA14580.h"

#include "at45db161d.h"

#undef PAGE_SIZE
//#define PAGE_SIZE 264 // AT45DB081D (1MB)
#define PAGE_SIZE 256 // AT25XE011 (1Mbit)
//#define PAGE_SIZE 528 // AT45DB321D (4MB)

//#define PAGE_NUM 4095 // AT45DB081D (1MB)
#define PAGE_NUM 512 // AT25XE011 (1Mbit)
//#define PAGE_NUM 8192 // AT45DB321D (4MB)

#define WRITE_BUFFER 1
#define READ_BUFFER 2

#define     LOADER_FILE         "/local/loader.bin"
#define     TARGET_FILE         "/local/target.bin"

#if defined(TARGET_LPC1768)
//SWD swd(p25,p24,p23); // SWDIO,SWCLK,nRESET
SWD swd(p24, p23, p22); // SWDIO,SWCLK,nRESET
DigitalOut connected(LED1);
DigitalOut running(LED4);

SWSPI spi(p5, p7, p6); // mosi, miso, sclk

ATD45DB161D memory(spi, p8);
RawSerial ble(p5, p6);
DA14580 BLE(ble, p26);

#elif defined(TARGET_LPC11U35_501)
//SWD swd(p25,p24,p23); // SWDIO,SWCLK,nRESET
SWD swd(P0_5,P0_4,P0_21); // SWDIO,SWCLK,nRESET
DigitalOut connected(P0_20);
DigitalOut running(P0_2);

SWSPI spi(P0_9,P0_8,P0_10); // mosi, miso, sclk
ATD45DB161D memory(spi, P0_7);
RawSerial ble(P0_19,P0_18);
DA14580 BLE(ble, P0_1);
#endif

int file_size( FILE *fp );
void flash_write (int addr, char *buf, int len);
void flash_read (int addr, char *buf, int len);

class myDAP : public BaseDAP
{
public:
    myDAP(SWD* swd):BaseDAP(swd) {};
    virtual void infoLED(int select, int value) {
        switch(select) {
            case 0:
                connected = value^1;
                running = 1;
                break;
            case 1:
                running = value^1;
                connected = 1;
                break;
        }
    }
};


int main()
{
//    USBLocalFileSystem* usb_local = new USBLocalFileSystem(P0_9, P0_8, P0_10, P0_7,"local"); // RamDisk(64KB)
    USBLocalFileSystem* usb_local = new USBLocalFileSystem(p17, p15, p16, p18,"local"); // SD
//    USBLocalFileSystem* usb_local = new USBLocalFileSystem(P0_14, P0_15, P0_16, P0_32,"local"); // SD
    usb_local->lock(true);
    myDAP* dap = new myDAP(&swd);

//    uint8_t recieve;
//    uint8_t read;
//    int filesize=0;
    FILE* fp;
//    ble.baud(57600);
//    int crc=0x00;


    int result=0;
    while(1) {
        usb_local->lock(true);
        usb_local->remount();
        char filename[32];

        usb_local->puts("Try BLE.load(): ");
        result = BLE.load();
        usb_local->putc(result);
        usb_local->puts("\n\r");

        if (usb_local->find(filename, sizeof(filename), "*.TXT")) {
            fp = fopen(filename, "r");
            if (fp) {
                int c;
                while((c = fgetc(fp)) != EOF) {
                    usb_local->putc(c);
                }
                fclose(fp);
#if defined(__MICROLIB) && defined(__ARMCC_VERSION) // with microlib and ARM compiler
#warning "free(fp)"
                free(fp);
#endif
            }
        }

        USBStorage2* _usb = usb_local->getUsb();
        USB_HID* _hid = _usb->getHID();
        HID_REPORT recv_report;
        if( _usb->readNB(&recv_report) ) {
            HID_REPORT send_report;
            dap->Command(recv_report.data, send_report.data);
            send_report.length = 64;
            _usb->send(&send_report);
        }
        usb_local->lock(false);
        wait_ms(1000*5);
    }
}

int file_size( FILE *fp )
{
    int     size;

    fseek( fp, 0, SEEK_END ); // seek to end of file
    size    = ftell( fp );    // get current file pointer
    fseek( fp, 0, SEEK_SET ); // seek back to beginning of file

    return size;
}


void flash_write (int addr, char *buf, int len)
{
    int i;
    memory.BufferWrite(WRITE_BUFFER, addr % PAGE_SIZE);
    for (i = 0; i < len; i ++) {
        spi.write(buf[i]);
    }
    memory.BufferToPage(WRITE_BUFFER, addr / PAGE_SIZE, 1);
}

void flash_read (int addr, char *buf, int len)
{
    int i;
    memory.PageToBuffer(addr / PAGE_SIZE, READ_BUFFER);
    memory.BufferRead(READ_BUFFER, addr % PAGE_SIZE, 1);
    for (i = 0; i < len; i ++) {
        buf[i] = spi.write(0xff);
    }
}

