#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void itoa(int value, char *str, int base) {
    int i = 0;
    bool negative = value < 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (value == INT_MIN) {
        strcpy(str, "-2147483648");
        return;
    }

    if (negative)
        value = -value;

    while (value) {
        int digit = value % base;
        // handle hexadecimals
        str[i++] = (char)(digit < 10 ? digit + '0' : digit - 10 + 'a');
        value /= base;
    }

    if (negative)
        str[i++] = '-';
    str[i] = '\0';

    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - 1 - j];
        str[i - 1 - j] = temp;
    }
}
