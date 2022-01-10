#ifndef _SERIALLOG_H
#define _SERIALLOG_H

#include <Arduino.h>

#if DOTELNET == true
#include <WiFiUdp.h>
#include <esptelnet.h>
#undef CR
#define CR "\r\n"
#endif

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

// Redefine Serial.print*() functions
size_t myPrint(const __FlashStringHelper *ifsh);
size_t myPrint(const String &s);
size_t myPrint(const char str[]);
size_t myPrint(char c);
size_t myPrint(unsigned char b, int base);
size_t myPrint(int n, int base);
size_t myPrint(unsigned int n, int base);
size_t myPrint(long n, int base);
size_t myPrint(unsigned long n, int base);
size_t myPrint(double n, int digits);
size_t myPrint(const Printable &x);
size_t myPrint(struct tm *timeinfo, const char *format);
// size_t myPrintf(const char *format, ...);
size_t myPrintln(void);
size_t myPrintln(const __FlashStringHelper *ifsh);
size_t myPrintln(const String &s);
size_t myPrintln(const char c[]);
size_t myPrintln(char c);
size_t myPrintln(unsigned char b, int base);
size_t myPrintln(int num, int base);
size_t myPrintln(unsigned int num, int base);
size_t myPrintln(long num, int base);
size_t myPrintln(unsigned long num, int base);
size_t myPrintln(double num, int digits);
size_t myPrintln(const Printable &x);
size_t myPrintln(struct tm *timeinfo, const char *format);
void nullDoc(const char *);

#define prefLen 22

// extern struct ThatVersion thatVersion;
// extern const size_t capacitySerial;

#endif //_SERIALLOG_H
