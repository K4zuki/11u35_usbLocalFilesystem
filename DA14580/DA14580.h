/**
 * @file DA1458X.h
 * @brief DA1458X writer
 **/
#ifndef DA1458X_H
#define DA1458X_H

#include "mbed.h"

#define     LOADER_FILE         "/local/loader.bin"
#define     TARGET_FILE         "/local/target.bin"

/** \class DA14580
 * \brief mbed library for Dialog Semiconductor DA14580 Bluetooth LE chip
 *
 * Example:
 * @code
 * #include "mbed.h"
 * #include "DA14580.h"
 *
 * DA14580 BLE(P0_18, P0_19, P0_1);
 * Serial pc(USBTX, USBRX);
 *
 * int main()
 * {
 *    int result=0;
 *    pc.baud(115200);
 *
 *   wait_ms(1);
 *   fp = fopen( SOURCE_FILE, "rb" );
 *   result = BLE.load(fp);
 *   fclose(fp);
#if defined(__MICROLIB) && defined(__ARMCC_VERSION) // with microlib and ARM compiler
 *   free(fp);
#endif
 *   pc.printf("Result = %d \n\r",&result);
 * }
 * @endcode
 */

enum XMODEM_CONST {
    SOH = (0x01),
    STX = (0x02),
    EOT = (0x04),
    ACK = (0x06),
    DLE = (0x10),
    NAK = (0x15),
    CAN = (0x18),
};

enum DA14580_STATUS{
    SUCCESS,
    E_NOT_CONNECTED,
    E_FILE_NOT_FOUND,
    E_TIMEOUT_STX,
    E_ACK_NOT_RETURNED,
    E_CRC_MISMATCH
};

class DA14580
{
public:
    DA14580( PinName TX, PinName RX, PinName RESET );
    DA14580( Serial &ble, PinName RESET );
    ~DA14580();

    void init();
    int load();
    int file_size( FILE *fp ); // copied from ika_shouyu_poppoyaki
    RawSerial _ble;

private:
    uint8_t _recieve;
    uint8_t _read;
    int _filesize;
    int _timeout;
    int _status;
    FILE* _fp;
    int _crc;
    DigitalInOut _reset;
};

#endif //DA1458X_H
