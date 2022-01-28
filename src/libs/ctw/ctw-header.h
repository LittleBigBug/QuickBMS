/* ctw-header.h : contains functions for reading and writing the CTW header
                  NOTE: all settings are checked before write_header is used */

void write_header (FILE *out, unsigned int filesize);

boolean read_header (FILE *in, unsigned int *filesize);