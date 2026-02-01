#ifndef _SERIALLOG_H
#define _SERIALLOG_H

#include <cstddef>

void serial();

// Print outputs
size_t printChar(bool, const char *);
size_t printChar(const char *);
size_t printDot();
size_t printDot(bool);
size_t printCR();
size_t printCR(bool);
void flush();
void flush(bool);

#define prefLen 22

#endif //_SERIALLOG_H
