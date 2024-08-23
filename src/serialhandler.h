#ifndef _SERIALLOG_H
#define _SERIALLOG_H

void serial();
void toggleSerialCompat(bool);
void printPrefix(Print* _logOutput, int logLevel);
void printTimestamp(Print *_logOutput);
void serialLoop();
void debug();

// Print outputs
size_t printChar(bool, const char *);
size_t printChar(const char *);
size_t printDot();
size_t printDot(bool);
size_t printCR();
size_t printCR(bool);
void flush();
void flush(bool);

void nullDoc(const char *);

#define prefLen 22

// extern struct ThatVersion thatVersion;
// extern const size_t capacitySerial;

#endif //_SERIALLOG_H
