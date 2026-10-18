#ifndef NB_WIRE_H
#define NB_WIRE_H

/* Portable C89 helpers; MacTCP uses host-order values on the big-endian 68K. */
int ParseAddress(const char *host, const char *port,
                 unsigned long *address, unsigned short *number);
int MakeQuestion(const char *text, int length, char *request);
int DisplayText(char *text, int length);

#endif
