#include <windows.h>
#include <stdio.h>

int main(void) {
    UINT_PTR ptr = 1234;
    FARPROC fp = (FARPROC)ptr;
    printf("Passed test_cast2\n");
    return 0;
}
