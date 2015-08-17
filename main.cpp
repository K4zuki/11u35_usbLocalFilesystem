#include "mbed.h"
#include "USBLocalFileSystem.h"
#include "USBDAP.h"
#include "BaseDAP.h"
#include "USB_HID.h"

SWD swd(p25,p24,p23); // SWDIO,SWCLK,nRESET
DigitalOut connected(LED1);
DigitalOut running(LED2);

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
    USBLocalFileSystem* usb_local = new USBLocalFileSystem(P0_9, P0_8, P0_10, P0_7,"local"); // RamDisk(64KB)
    myDAP* dap = new myDAP(&swd);

//    USBStorage2* _usb = usb_local->getUsb();
//    USB_HID* _hid = _usb->getHID();

    while(1) {
        usb_local->lock(true);
        usb_local->remount();
        char filename[32];
        if (usb_local->find(filename, sizeof(filename), "*.TXT")) {
            FILE* fp = fopen(filename, "r");
            if (fp) {
                int c;
                while((c = fgetc(fp)) != EOF) {
                    usb_local->putc(c);
                }
                fclose(fp);
#if defined(__MICROLIB) && defined(__ARMCC_VERSION) // with microlib and ARM compiler
                free(fp);
#endif
            }
        }

        HID_REPORT recv_report;
        if( usb_local->getUsb()->getHID()->readNB(&recv_report) ) {
            HID_REPORT send_report;
            dap->Command(recv_report.data, send_report.data);
            send_report.length = 64;
            usb_local->getUsb()->getHID()->send(&send_report);
        }

        usb_local->lock(false);
        wait_ms(100*5);
    }
}