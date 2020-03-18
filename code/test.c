#include <stdio.h>

int main(){
    int a[] = {1,2,3,4,5,6,7,8,9};
    int *b = a+1;
    printf("%d %d %d", b[0], b[1], b[2]);
}