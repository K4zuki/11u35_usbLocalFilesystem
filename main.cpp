#include "mbed.h"
#include "USBLocalFileSystem.h"
#include "BaseDAP.h"
#include "USB_HID.h"
#include "DA14580.h"
#include "W25X40BV.h"
#include "loader.h"
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
- 580 -
MOSI = P0_9
MISO = P0_7
SCK = P0_8
CS = P0_21
*/
W25X40BV memory(P0_9, P0_7, P0_8, P0_21); // mosi, miso, sclk, cs
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
//MyStorage LocalFS(P0_15, P0_13, P0_14, P0_16);
int main()
{
    USBLocalFileSystem* usb_local = new USBLocalFileSystem(&LocalFS, "local"); //PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name

    USB_HID* _hid = usb_local->getUsb()->getHID();
    HID_REPORT recv_report;
    HID_REPORT send_report;
    myDAP* dap = new myDAP(&swd);

//    USBLocalFileSystem* usb_local = new USBLocalFileSystem(P0_8, P0_10, P0_9, P0_7, "local"); //PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name
//    USBLocalFileSystem* usb_local = new USBLocalFileSystem(&LocalFS, "local"); //PinName mosi, PinName miso, PinName sclk, PinName cs, const char* name
    running.write(1);
    TGT_RST_IN.mode(PullUp);
    memory.frequency(SPI_FREQ);
    char hex[]="0123456789ABCDEF"; //DEBUG

    int appleCount = 10;
    char buf[ 1024 ];

    int read = 0;
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
            sprintf( buf, "I have %d apples.\r\n", appleCount );
            char *ptr = buf;
            while( *ptr != (char)0 ) {
                usb_local->putc(*ptr++);
            }
            sprintf( &buf[0], "\r\nWriting "TARGET_FILE" into SPI flash" );
            ptr = &buf[0];
            while( *ptr != (char)0 ) {
                usb_local->putc(*ptr++);
            }
//            usb_local->puts("\n\r");
//            usb_local->puts("Writing "TARGET_FILE" into SPI flash");
//            BLE.copy_to_flash(&memory);

            int _filesize;
            uint8_t Headerbuffer[8] = {0x70,0x50,0x00,0x00,0x00,0x00,0x00,0x00};
            char _data[256];
            FILE* _fp = fopen(TARGET_FILE, "r" );
            if (_fp) {
                BLE.init();
                //erase 64KByte
                memory.block32Erase(0);
                memory.block32Erase(1);

                _filesize = file_size(_fp);
                usb_local->puts("\n\r");
                usb_local->puts("filesize: ");
                read= 0x0f& (_filesize>>12);
                usb_local->putc(hex[read]);
                read= 0x0f& (_filesize>>8);
                usb_local->putc(hex[read]);
                read= 0x0f& (_filesize>>4);
                usb_local->putc(hex[read]);
                read= 0x0f& (_filesize);
                usb_local->putc(hex[read]);
                usb_local->puts("\n\r");
                Headerbuffer[6]= (uint8_t)( (_filesize >> 8) & 0xff);
                Headerbuffer[7]= (uint8_t)(_filesize & 0xff);

                for(int i=0; i<8; i++) {
                    read= 0x0f& (Headerbuffer[i]>>4);
                    usb_local->putc(hex[read]);
                    read= 0x0f& (Headerbuffer[i]);
                    usb_local->putc(hex[read]);
                    usb_local->puts("\n\r");
                }

                memory.writeStream(0, Headerbuffer, 8);
                wait_ms(1);
                memory.readStream(0, (uint8_t*)_data, 8);

                for(int i=0; i<8; i++) {
                    read= 0x0f& (_data[i]>>4);
                    usb_local->putc(hex[read]);
                    read= 0x0f& (_data[i]);
                    usb_local->putc(hex[read]);
                    usb_local->puts("\n\r");
                }

                if(_filesize >= 248) {
                    fgets(_data, 248,_fp);
                    memory.writeStream(8, (uint8_t*)_data, (248));
                    _filesize -= (256-8);
                } else {
                    fgets(_data, _filesize ,_fp);
                    memory.writeStream(8, (uint8_t*)_data, (_filesize));
                    _filesize = 0;
                }

                int i=1;
                while(_filesize >= 256) {
                    fgets(_data, (256), _fp);
                    memory.writeStream(256*i, (uint8_t*)_data, (256));
                    usb_local->putc(hex[i%10]);
                    i++;
                    _filesize -= (256);
                }
                if(_filesize > 0) {
                    fgets(_data, _filesize, _fp);
                    memory.writeStream(256*i, (uint8_t*)_data, (_filesize));
                }
            }
            fclose(_fp);
#if defined(__MICROLIB) && defined(__ARMCC_VERSION) // with microlib and ARM compiler
#warning "free(_fp)"
            free(_fp);
#endif

            usb_local->puts("\n\r");
            usb_local->puts("Try BLE.load(): ");
            running.write(0);
            result = BLE.load();
            running.write(1);
            usb_local->putc(result);
            usb_local->putc(0x07); //bell
            usb_local->puts("\n\r");
            isISP = false;
            /*
            while(BLE._ble.readable()) {
                usb_local->putc(BLE._ble.getc());
            }
            */
        } else {
            ;
//            usb_local->putc('.');
        }

        usb_local->lock(false);

//        int i=10;
//        while(i>0) {
//            i--;
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
//        }
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
