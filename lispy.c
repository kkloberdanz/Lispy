/*
 * Programmer: Kyle Kloberdanz
 * Date Created: 10 Feb 2016
 * License: GNU GPLv3 (see LICENSE.txt)
 */

#include <stdio.h>
#include <stdlib.h>

#define VERSION   0.01
#define BUFF_SIZE 2048

char* readline(char* prompt) {
    if (prompt) {
        printf("%s", prompt);
    }

    char* buff = malloc(sizeof(char) * BUFF_SIZE + 1);
    char c;
    int i = 0;
    do { 
        c = (char)fgetc(stdin);
        buff[i] = c;
        i++;
    } while ((c != '\n') && (i < BUFF_SIZE));
    buff[--i] = '\0';

    return buff;
}

int shell() {
    printf("lispy version: %.2f\n", VERSION);
    printf("CTRL-C to exit\n");

    while (1) {
        char* input = readline("[lispy]> ");
        printf("[out]> %s\n", input);
        free(input);
    }
    return 0;
}

int main(int argc, char** argv) {
    shell();
    return 0;
}
