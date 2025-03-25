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

    do{
        if(str1[i] > str2[i]){
            return 1;
        }
        else if(str1[i] < str2[i]){
            return -1;
        }
        i++;
    } while (str1[i] != 0 && str2[i] != 0);
    
    if(str1[i] > str2[i]){
        return 1;
    }
    else if(str1[i] < str2[i]){
        return -1;
    }

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