#define FILEUTILS_H

bool 
mountVol();

bool 
fileExists (char *filename);

int 
read_file (char *filename, char *readbuf);

int
write_file (char *filename, char *writebf);