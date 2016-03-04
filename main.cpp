#include "mbed.h"
#include "USBLocalFileSystem.h"
#include "USBDAP.h"
#include "BaseDAP.h"
#include "USB_HID.h"
#include "DA14580.h"
#include "W25X40BV.h"
#include "loader.h"
/**
- UART -
TX = P0_19
RX = P0_18
- SWD -
SWDIO = P0_4
SWCLK = P0_5
NSRST = P0_21
TGT_RST = P1_19
- SD -
MOSI = P0_8
MISO = P0_10
SCK = P0_9
CS = P0_7
DETECT2 = P0_22
- 580 -
MOSI = P0_15
MISO = P0_13
SCK = P0_14
CS = P0_16
DETECT1 = P0_11
- LED -
GREEN = P0_20
YELLOW = P0_2
*BL = P0_14*
*/
/** r0.1
- UART -
TX = P0_19
RX = P0_18
- SWD -
SWDIO = P0_4
SWCLK = P0_5
NSRST = P0_21
- SD -
MOSI = P0_8
MISO = P0_10
SCK = P0_9
CS = P0_7
- 580/SPI Flash -
MOSI = P0_15
MISO = P0_13
SCK = P0_14
CS = P0_16
- LED -
GREEN = P0_20
YELLOW = P0_2
*/

#undef      LOADER_FILE
#define     LOADER_FILE         "/local/loader.bin"

#undef      TARGET_FILE
#define     TARGET_FILE         "/local/target.bin"

//SWD swd(P0_4, P0_5, P0_21); // SWDIO,SWCLK,nRESET

DigitalOut connected(P0_20);
DigitalOut running(P0_2);

InterruptIn BL(P1_19);
volatile bool isISP = false;
void BL_int();

W25X40BV memory(P0_15, P0_13, P0_14, P0_16); // mosi, miso, sclk, cs
uint8_t Headerbuffer[8]={0x70,0x50,0x00,0x00,0x00,0x00,0x00,0x00};
/*
header[0] | 0x70 | 'p'
header[1] | 0x50 | 'P'
header[2] | 0x00 | dummy[3]
header[3] | 0x00 | dummy[2]
header[4] | 0x00 | dummy[1]
header[5] | 0x00 | dummy[0]
header[6] | 0x00 | binary size MSB <- to be replaced to actual size
header[7] | 0x00 | binary size LSB <- to be replaced to actual size
*/
DA14580 BLE(P0_19, P0_18, P0_21); // TX, RX, RESET

int file_size( FILE *fp );

/*
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
*/

MyStorage flash(P0_15, P0_13, P0_14, P0_16);
int main()
{
//    USBLocalFileSystem* usb_local = new USBLocalFileSystem(P0_8, P0_10, P0_9, P0_7, "local"); //PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name
    USBLocalFileSystem* usb_local = new USBLocalFileSystem(&flash, "local"); //PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name
    running.write(1);
    BL.mode(PullUp);
    char hex[]="0123456789ABCDEF"; //DEBUG

//    usb_local->lock(true);

//    uint8_t recieve;
//    uint8_t read=0;
    int read = 0;
    int loadersize = sizeof(loader)/sizeof(loader[0]);
    int targetsize = 0;
    FILE* fp;
//    BLE.baud(57600);
//    int crc=0x00;

//    myDAP* dap = new myDAP(&swd);

    int result=0;
    BL.mode(PullUp);
    BL.fall(&BL_int);

/*
    fp = fopen(LOADER_FILE, "rb");
    loadersize = file_size(fp);
    fclose(fp);
#if defined(__MICROLIB) && defined(__ARMCC_VERSION) // with microlib and ARM compiler
#warning "free(fp)"
    free(fp);
#endif
*/
/*
    fp = fopen(TARGET_FILE, "rb");
    targetsize = file_size(fp);
    fclose(fp);
#if defined(__MICROLIB) && defined(__ARMCC_VERSION) // with microlib and ARM compiler
#warning "free(fp)"
    free(fp);
#endif
*/

    bool _hidresult;

    while(1) {
        usb_local->lock(true);
        usb_local->remount();
        connected.write(1);
        char filename[32];

        if(isISP) {
            usb_local->puts("\n\r");
            usb_local->puts("Writing "TARGET_FILE" into SPI flash");
            usb_local->puts("\n\r");
            usb_local->puts("Try BLE.load(): ");
            running.write(0);
            result = BLE.load();
            running.write(1);
            usb_local->putc(result);
            usb_local->puts("\n\r");
            isISP = false;
        } else {
            if(BLE._ble.readable()){
                usb_local->putc(BLE._ble.getc());
            }else{
                usb_local->putc('.');
            }
        }
        /*
        usb_local->puts("loadersize: ");
        read= 0x0f& (loadersize>>12);
        usb_local->putc(hex[read]);
        read= 0x0f& (loadersize>>8);
        usb_local->putc(hex[read]);
        read= 0x0f& (loadersize>>4);
        usb_local->putc(hex[read]);
        read= 0x0f& (loadersize);
        usb_local->putc(hex[read]);
        usb_local->puts("\n\r");
        */
        /*
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
        */

//        USBStorage2* _usb = usb_local->getUsb();
/*
        HID_REPORT recv_report;

        USB_HID* _hid = usb_local->getUsb()->getHID();
        _hidresult = _hid->readNB(&recv_report);
        if( _hidresult ) {
            HID_REPORT send_report;
            usb_local->puts("T\n\r");
            dap->Command(recv_report.data, send_report.data);
            send_report.length = 64;
            _hid->send(&send_report);
        } else {
            usb_local->puts("F\n\r");
        }
*/
        usb_local->lock(false);
        connected = 0;
        wait_ms(1000);
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

void BL_int()
{
    isISP = true;
}
