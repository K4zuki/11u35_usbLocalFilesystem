#include "mbed.h"
#include "USBLocalFileSystem.h"
#include "BaseDAP.h"
#include "USB_HID.h"
#include "DA14580.h"
#include "W25X40BV.h"
//#include "loader.h"
#include "mystorage.h"

/** r0.4_aef7891
- UART -
TX = P0_19
RX = P0_18
- SWD -
SWDIO = P0_14
SWCLK = P0_13
NSRST = P0_2
TGT_RST = P1_19
- SPI Flash -
MOSI = P0_16
MISO = P0_23
SCK = P0_15
CS = P1_15
- 580 -
MOSI = P0_9
MISO = P0_7
SCK = P0_8
CS = P0_21
- LED -
RED = P0_5
GREEN = P0_4
BLUE = P0_20
*/

#undef      LOADER_FILE
#define     LOADER_FILE         "/local/loader.bin"

#undef      TARGET_FILE
#define     TARGET_FILE         "/local/target.bin"

/*
- SWD -
SWDIO = P0_14
SWCLK = P0_13
NSRST = P0_2
TGT_RST = P1_19
*/
SWD swd(P0_14, P0_13, P0_2); // SWDIO,SWCLK,nRESET
InterruptIn TGT_RST_IN(P1_19);
volatile bool isISP = false;
void TGT_RST_IN_int();

/*
- LED -
RED = P0_5
GREEN = P0_4
BLUE = P0_20
*/
DigitalOut connected(P0_5);
DigitalOut running(P0_4);

/*
- UART -
TX = P0_19
RX = P0_18
*/
DA14580 BLE(P0_19, P0_18, P0_2); // TX, RX, RESET

int file_size( FILE *fp );

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

/*
- SPI Flash -
MOSI = P0_16
MISO = P0_23
SCK = P0_15
CS = P1_15
*/
MyStorage LocalFS(P0_16, P0_23, P0_15, P1_15); // mosi, miso, sclk, cs
int main()
{
    USBLocalFileSystem* usb_local = new USBLocalFileSystem(&LocalFS, "local");
    //PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name

    USB_HID* _hid = usb_local->getUsb()->getHID();
    HID_REPORT recv_report;
    HID_REPORT send_report;
    myDAP* dap = new myDAP(&swd);

    running.write(1);
    TGT_RST_IN.mode(PullUp);
    char* STAT_MSG[6]={
        "SUCCESS",
        "NOT CONNECTED",
        "FILE NOT FOUND",
        "STX TIMEOUT",
        "NO ACKNOWLEDGE",
        "CRC MISMATCHED"
    };

    int result = 0;
    TGT_RST_IN.mode(PullUp);
    TGT_RST_IN.fall(&TGT_RST_IN_int);

    bool _hidresult;
    usb_local->lock(false);
    while(1) {
        usb_local->lock(true);
        usb_local->remount();
        connected.write(1);

        if(isISP) {
            running.write(0);
            result = BLE.load(TARGET_FILE);
            running.write(1);
            usb_local->puts(STAT_MSG[result]);
            usb_local->putc(0x07); //bell
            usb_local->puts("\n\r");
            isISP = false;
        }
        usb_local->lock(false);
        _hidresult = _hid->readNB(&recv_report);
        if( _hidresult ) {

            dap->Command(recv_report.data, send_report.data);
            send_report.length = 64;
            _hid->send(&send_report);
        }
        if(BLE._ble.readable()) {
            usb_local->putc(BLE._ble.getc());
        }
        wait_us(1000);
        connected = 0;
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

void TGT_RST_IN_int()
{
    isISP = true;
}
