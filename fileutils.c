#include "esp/uart.h"
#include <stdio.h>
#include <string.h>
#include "fcntl.h"
#include "unistd.h"
#include "spiffs.h"
#include "esp_spiffs.h"
#include "fileutils.h"


bool 
mountVol() {
    esp_spiffs_init(0x200000, 0x10000);

    if (esp_spiffs_mount() != SPIFFS_OK) {
        #ifdef DBG_FILEUTILS
        printf("Error mount SPIFFS\n");
        #endif
        return false;
    }
    return true;
}

bool 
fileExists (char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        #ifdef DBG_FILEUTILS
        printf("Error opening file\n");
        #endif
        return false;
    }
    close(fd);
    return true;
}

int 
read_file (char *filename, char *readbuf) 
{
    const int buf_size = 0x500;

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        #ifdef DBG_FILEUTILS
        printf("Error opening file\n");
        #endif
        return 0;
    }

    int read_bytes = read(fd, readbuf, buf_size);
    #ifdef DBG_FILEUTILS
    printf("Read %d bytes\n", read_bytes);
    #endif

    readbuf[read_bytes] = '\0';    // zero terminate string
    close(fd);
    return read_bytes;
}

int 
write_file (char *filename, char *writebf, uint8_t len) 
{
    int fd = open(filename, O_WRONLY|O_CREAT, 0);
    if (fd < 0) {
        #ifdef DBG_FILEUTILS
        printf("Error opening file\n");
        #endif
        return 0;
    }

    int written = write(fd, writebf, len);
    printf("%d written to file\n", written);
    close(fd);
    return written;
}

