#ifndef _STROPS_H
#define _STROPS_H

/**
 * Convert integer to a null terminated string
 * 
 * @param x integer
 * @param str buffer to store null terminated string
 * 
*/
void int_toString(int x, char str[]);

/**
 * Count the length of a null terminated string
 * @warning     Do not use on non-null-terminated strings
 * 
 * @param str   null terminated string
 * 
 * @return      length in integer
*/
int strlen(char str[]);

/**
 * Compare null terminated strings
 * @warning     Do not use on non-null-terminated strings
 * 
 * @param str1  null terminated string
 * @param str2  null terminated string
 * 
 * @return comparison result, 1 means str1 > str2, -1 means str1 < str2, 0 means str1 == str2
*/
int strcmp(char str1[], char str2[]);

/**
 * Copy null terminated strings
 * @warning     Do not use on non-null-terminated strings
 * 
 * @param dest  Buffer to store copied string
 * @param src   Null terminated string
 * 
*/
void strcpy(char dest[], char src[]);

int str_to_int(const char *str, int *result);

#endif