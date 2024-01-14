#ifndef BENOS_STRING_H
#define BENOS_STRING_H

#include <stdbool.h>

int strlen(const char* ptr);
int strnlen(const char* ptr, int max);
bool isdigit(char c);
int tonum(char c);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, int count);
int strncmp(const char* str1, const char* str2, int len);
int istrncmp(const char* str1, const char* str2, int len);
int strnlen_terminator(const char* str, int max, char terminator);
char tolower(char c);
char* strtok(char* str, const char* delimiters);

#endif