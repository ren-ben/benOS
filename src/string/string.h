#include <stdbool.h>
#ifndef STRING_H
#define STRING_H

int strlen(const char* ptr);
int strnlen(const char* ptr, int max);
bool isdigit(char c);
int tonum(char c);
char* strcpy(char* dest, const char* src);

#endif