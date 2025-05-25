#include "header/stdlib/strops.h"

int strlen(char str[]){
    int counter = 0;
    while(str[counter] != 0){
        counter++;
    }
    return counter;
}

void int_toString(int x, char str[]){
    int i = 0;
    int negative = 0;

    if(x < 0){
        x = x*(-1);
        negative = 1;
    }   

    do{
        str[i] = x % 10 + '0';
        i++; 
    } while  ( (x /= 10) > 0);

    if(negative){
        str[i] = '-';
        str[i+1] = 0;
    }
    else{
        str[i] = 0;
    }

    int j, k, temp;
    for(j = 0, k = strlen(str) - 1; j < k; j++, k--){
        temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }

    return;
}


int strcmp(char str1[], char str2[]){
    int i = 0;

    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] > str2[i]) return 1;
        if (str1[i] < str2[i]) return -1;
        i++;
    }

    if (str1[i] != '\0') return 1;
    if (str2[i] != '\0') return -1;

    return 0;
}

void strcpy(char dest[], char src[]){
    int counter = 0;
    while(src[counter] != 0){
        dest[counter] = src[counter];
        counter++;
    }
    dest[counter] = 0;
    return;
}

int str_to_int(const char *str, int *result) {
    // Input validation
    if (str == 0 || result == 0 || *str == '\0') {
        return 0; // Invalid pointers or empty string
    }
    
    const char *ptr = str;
    int sign = 1;
    int value = 0;
    
    // Handle optional sign
    if (*ptr == '-') {
        sign = -1;
        ptr++;
    } else if (*ptr == '+') {
        ptr++;
    }
    
    // Must have at least one digit after sign
    if (*ptr < '0' || *ptr > '9') {
        return 0; // No digits found
    }
    
    // Skip leading zeros and validate digits
    while (*ptr == '0') {
        ptr++;
    }
    
    // Count digits and validate format
    int digit_count = 0;
    const char *digit_start = ptr;
    while (*ptr >= '0' && *ptr <= '9') {
        digit_count++;
        ptr++;
    }
    
    // Check if we reached end of string (all characters were valid)
    if (*ptr != '\0') {
        return 0; // Invalid character found
    }
    
    // Handle case where number is just "0"
    if (digit_count == 0) {
        *result = 0;
        return 1;
    }
    
    // Check for overflow - INT_MAX = 2147483647, INT_MIN = -2147483648
    if (digit_count > 10) {
        return 0; // Definitely overflow
    }
    
    if (digit_count == 10) {
        // Exactly 10 digits - need to compare with limits
        const char *limit = (sign == -1) ? "2147483648" : "2147483647";
        for (int i = 0; i < 10; i++) {
            if (digit_start[i] > limit[i]) {
                return 0; // Overflow
            } else if (digit_start[i] < limit[i]) {
                break; // No overflow, safe to continue
            }
        }
    }
    
    // Perform conversion
    ptr = digit_start;
    while (*ptr >= '0' && *ptr <= '9') {
        value = value * 10 + (*ptr - '0');
        ptr++;
    }
    
    *result = sign * value;
    return 1; // Success
}



