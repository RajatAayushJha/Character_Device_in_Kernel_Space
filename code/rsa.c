#include <stdio.h>
struct public_key{
    long long int e;
    long long int n;
};
struct private_key{
    long long int d;
    long long int n;
};

struct keys{
    struct private_key pvt_key;
    struct public_key pub_key;
};

long long int power(long long int a, long long int n, long long int mod){
    long long ans = 1;
    while(n != 0){
        if(n&1 == 1){
            ans *= a;
            if(ans >= mod) ans %= mod;
        }
        a *= a;
        if(a >= mod) a %= mod;
        n >>= 1;
    }
    return ans;
}

int is_prime(long long int n){
    if(n < 2)return 0;
    if(n%2 == 0 && n != 2) return 0;
    long long int i = 3;
    while(i*i <= n){
        if(n%i == 0) return 0;
        i += 2;
    }
    return 1;
}

long long int totient(long long int n){
    long long int factor = 2;
    long long int p, ans = 1;
    while(factor * factor <= n){
        if(!is_prime(factor)){
            factor++;
            continue;
        }
        p = 1;
        while(n%factor == 0){
            n /= factor;
            p *= factor;
        }
        ans *= (p - p/factor);
        factor++;
    }
    if(n != 1) ans *= (n - 1);
    return ans;
}

long long int inverse(long long int a, long long int n){
    if(!is_prime(n)) return -1;
    return power(a, n-2, n);
}

long long int get_random_prime(){
    long long int prime = 1;
    do{
        // get_random_bytes(&prime, sizeof(prime));
        prime %= 1000000007;
    }while(! is_prime(prime));
    return prime;
}

struct keys gen_keys(){
    printf("inside");
    long long int p = 11;
    long long int q = 17;
    printf("1");
    struct public_key pub_key;
    struct private_key pvt_key;
    pub_key.n = p*q;
    pvt_key.n = p*q;
    pub_key.e = 3;
    long long int phi = 160;
    printf("2");
    pvt_key.d = power(pub_key.e, phi - 1, phi);
    printf("3");
    struct keys key;
    key.pub_key = pub_key;
    key.pvt_key = pvt_key;
    return key;
}
long long int encrypt(long long int c, struct public_key pub_key){
    return power(c, pub_key.e, pub_key.n);
}
long long int decrypt(long long int c, struct private_key pvt_key){
    return power(c, pvt_key.d, pvt_key.n);
}
void encrypt_data(long long int* data, char* buffer, int size, struct public_key pub_key){
    int i = 0;
    while(i < size){
        data[i] = encrypt((long long int)buffer[i] + i, pub_key);
        i++;
    }
}
void decrypt_data(long long int* data, char* buffer, int size, struct private_key pvt_key){
    int i = 0;
    while(i < size){
        buffer[i] = decrypt(data[i], pvt_key) - i;
        i++;
    }
}

void main(){
    printf("begun %lld\n", totient(11*11 * 17));
}