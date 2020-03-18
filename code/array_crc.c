#include<stdio.h>

long long int reverse_char(long long int a, int bit_field){
    long long int c = 0;
    int count = 0;
    printf("starting rev\n");
    while(count < bit_field){
        c <<= 1;
        c = (c | (a&1));
        a >>= 1;
        count ++;
        printf("after %d shift number is %llu\n", count, c);
    }
    printf("done rev\n");
    return c;
}

char crc(char* buffer, int size, char polynomial){
    unsigned long long int holder = 0;
    char result = 0;
    int bits_rem = 0;
    unsigned long long int pos;
    unsigned long long int poly = reverse_char(polynomial, 8);
    printf("poly nomial is %llu", poly);
    int i = 0;
    while(i < size){
        holder = (holder | ((long long int)reverse_char(buffer[i++], 8)<<bits_rem));
        bits_rem += 8;
        printf("%llu %i\n", holder, bits_rem);
        while(bits_rem >= 8){
            printf("dividing %llu %d\n", holder&1, bits_rem);
            while((0 == (1 & holder)) && bits_rem > 0){
                holder >>= 1;
                bits_rem--;
                // printf("shifting\n");
            }
            pos = 1;
            if(bits_rem < 8) break;
            // printf("entering divider\n");
            // printf("holder before: %llu\n", holder);
            while(pos < (1<<8)){
                
                holder = ((holder & (~pos)) | ((pos & holder) ^ (pos & poly)));
                
                pos <<= 1;
                // printf("pos: %llu\n", pos);
            } 
            // holder = ((holder & (~(1<<8 - 1))) | ((holder & (1<<8 - 1)) ^ (poly & (1<<8 - 1))));
            printf("holder after: %llu\n", holder);
            // printf("exiting divider\n");
        }
    }
    // return (unsigned char ) holder;
    return reverse_char(holder, bits_rem);
}

int main(){
    char buff[3];
    buff[0] = 0b11010111;
    buff[1] = 0b11011111;
    buff[2] = 0b01110000;
    printf("%u", crc(buff, 3, 0b11010000));
    // printf("%llu", 0 | (reverse_char(buff[0], 8)<<0));
}