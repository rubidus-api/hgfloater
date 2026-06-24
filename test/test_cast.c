#include <windows.h>
#include <stdio.h>

int main(void) {
    FARPROC fp = (FARPROC)1234;
    void(*fn)(void) = (void(*)(void))fp;
    (void)fn;
    printf("Passed test_cast\n");
    return 0;
}
