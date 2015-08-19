#include "mbed.h"
#include "DA14580.h"

/*

DigitalOut myled(LED1);

Serial pc(USBTX,USBRX);
Serial ble(p28,p27);

LocalFileSystem local( "local" );

#define     SOURCE_FILE         "/local/_loader"
#define     TARGET_FILE         "/local/_bin"

int file_size( FILE *fp );
enum XMODEM_CONST{
SOH = (0x01),
STX = (0x02),
EOT = (0x04),
ACK = (0x06),
DLE = (0x10),
NAK = (0x15),
CAN = (0x18),
};
*/
DA14580::DA14580( PinName TX, PinName RX, PinName RESET ) : _ble(TX,RX), _reset(RESET)
{
    init();
    ;
}

DA14580::DA14580( Serial &ble, PinName RESET ) : _ble(ble), _reset(RESET)
{
    init();
    ;
}

void DA14580::init(){

    _ble.baud(57600);
    _crc = 0x00;
    _recieve = 0;
    _read = 0;
    _filesize = 0;
    _reset = 0;
    _timeout = 100;
    _status = SUCCESS;
}

int DA14580::load(){

    _status = SUCCESS;

    _fp = fopen( LOADER_FILE, "rb" );
    if (_fp) {
        _filesize = file_size(_fp);
//        pc.printf("0x%04X\n\r",_filesize);
        while((_timeout--) >0){
            if( _ble.readable() ){
                _recieve = _ble.getc();
                if(_recieve == STX) {
                    _ble.putc(SOH);
//                    pc.putc('!');
                    break;
                }
            }
        }
        if(_timeout <= 0){
            _status = E_TIMEOUT_STX;
        }else{
            _timeout = 100;
            _ble.putc(_filesize & 0xff);
            _ble.putc( (_filesize >> 8) & 0xff);
            while((_timeout--) >0){
                if( _ble.readable() ){
                    _recieve = _ble.getc();
                    if(_recieve == ACK) {
//                        pc.printf("ok!\n\r");
                        break;
                    }
                }
            }
            if(_timeout <= 0){
                _status = E_ACK_NOT_RETURNED;
            }else{
                _timeout = 100;
                for(int i = 0; i < _filesize; i++){
                    _read = getc(_fp);
                    _ble.putc(_read);
                    _crc = _crc ^ _read;
                    if((i % 16) == 0){
//                        pc.printf("\n\r");
                    }
//                    pc.printf("%02X ",_read);
                }
//                pc.printf("\n\r0x%02X ",_crc);
                while((_timeout--) >0){
                    if( _ble.readable() ){
                        _recieve = _ble.getc();
                        if(_recieve == _crc) {
                            _ble.putc(ACK);
//                            pc.printf("-=-=DONE=-=-\n\r");
                            break;
                        }
                    }
                }
                if(_timeout <= 0){
                    _status = E_ACK_NOT_RETURNED;
                }
            }
        }
        fclose(_fp);
#if defined(__MICROLIB) && defined(__ARMCC_VERSION) // with microlib and ARM compiler
        free(_fp);
#endif
    }else{
        _status = E_FILE_NOT_FOUND;
    }
    return _status;


}

int DA14580::file_size( FILE *fp )
{
    int     size;

    fseek( fp, 0, SEEK_END ); // seek to end of file
    size    = ftell( fp );    // get current file pointer
    fseek( fp, 0, SEEK_SET ); // seek back to beginning of file

    return size;
}

/*
int main()
{
    uint8_t recieve;
    uint8_t read;
    int filesize=0;
    FILE* fp;
    ble.baud(57600);
    int crc=0x00;

    fp = fopen( SOURCE_FILE, "rb" );
    if (fp) {
        filesize=file_size(fp);
        pc.printf("0x%04X\n\r",filesize);
    }

    while(1) {
        recieve=ble.getc();
        if(recieve == STX) {
            ble.putc(SOH);
            pc.putc('!');
            break;
        }
    }
    ble.putc(filesize&0xff);
    ble.putc( (filesize>>8)&0xff);
    while(1) {
        recieve=ble.getc();
        if(recieve == ACK) {
            pc.printf("ok!\n\r");
//            ble.putc(0x01);
            break;
        }
    }
    for(int i=0;i<filesize;i++){
        read=getc(fp);
        ble.putc(read);
        crc=crc^read;
        if((i%16)==0){
            pc.printf("\n\r");
        }
        pc.printf("%02X ",read);
    }
    pc.printf("\n\r0x%02X ",crc);
    while(1) {
        recieve=ble.getc();
        if(recieve == crc) {
            ble.putc(ACK);
            pc.printf("-=-=DONE=-=-\n\r");
            break;
        }
    }
    fclose(fp);
    myled = 1;
    while(1) {
        recieve=ble.getc();
        pc.putc(recieve);
        wait_ms(20);
    }
}

*/
