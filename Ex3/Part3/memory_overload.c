#include <stdlib.h>

int main(void) {
    while(calloc(1024, sizeof(char)) != NULL);

    return 0;
}