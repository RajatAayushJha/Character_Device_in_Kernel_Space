/* Create a character device that encrypts data before writing and then
 * requires a key to decrypt it everytime.
 * A new key is generated when the module is first inserted.
 */
 
// include all the necessary header files
#define MODULE
#define LINUX
#define __KERNEL__
 
#if defined(CONFIG_MODVERSIONS) && ! defined(MODVERSIONS)
   #include <linux/modversions.h>
   #define MODVERSIONS
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/errno.h>
#include <linux/uaccess.h>
#include <linux/random.h>
 
// defining the required structures
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
 
// function prototypes for the module
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
 
// function prototypes for the rsa implementation
static long long int pow(long long int, long long int, long long int);
static int is_prime(long long int);
static long long int totient(long long int);
static long long int gcdExtended(long long int, long long int, long long int*, long long int*);
static long long int modInverse(long long int, long long int);
static long long int get_random_prime(long long int);
static struct keys gen_keys(void);
static long long int encrypt(long long int, struct public_key);
static long long int decrypt(long long int, struct private_key);
static void encrypt_data(long long int*, char*, int, struct public_key);
static void decrypt_data(long long int*, char*, int, struct private_key);
 
// function prototypes for the crc implementation
static long long int reverse_char(long long int, int);
static char crc(char*, int, char);
 
 
// definition of constants
#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 1024000
#define PRIME_OFF 10000
#define POLYNOMIAL 0b11001010
 
// definition of global variables
static int Major;
static int Device_Open = 0;
 
static long long int data[BUF_LEN];                          
static char buffer[BUF_LEN];
static int idx, size;
static int initial_open;
static struct public_key Pub_Key;
static struct private_key Pvt_Key;
 
// defining the allowed file operations on the character device
static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};
 
// rsa functions
 
static long long int pow(long long int a, long long int n, long long int mod){
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
 
static int is_prime(long long int n){
    if(n < 2)return 0;
    if(n%2 == 0 && n != 2) return 0;
    long long int i = 3;
    while(i*i <= n){
        if(n%i == 0) return 0;
        i += 2;
    }
    return 1;
}
 
static long long int totient(long long int n){
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
 
static long long int modInverse(long long int a, long long int m)
{
    long long int x, y;
    long long int g = gcdExtended(a, m, &x, &y);
    if (g != 1)
        return 0;
    else
    {
        // m is added to handle negative x
        long long int res = (x%m + m) % m;
        return res;
    }
}
 
// C function for extended Euclidean Algorithm
static long long int gcdExtended(long long int a, long long int b, long long int *x, long long int *y)
{
    // Base Case
    if (a == 0)
    {
        *x = 0, *y = 1;
        return b;
    }
 
    long long int x1, y1; // To store results of recursive call
    long long int gcd = gcdExtended(b%a, a, &x1, &y1);
 
    // Update x and y using results of recursive
    // call
    *x = y1 - (b/a) * x1;
    *y = x1;
 
    return gcd;
}
 
static long long int get_random_prime(long long int range){
    long long int prime = 1;
    do{
        get_random_bytes(&prime, sizeof(prime));
        prime %= range;
        prime += range;
    }while(! is_prime(prime));
    return prime;
}
 
static struct keys gen_keys(){
    long long int p = get_random_prime(2 * PRIME_OFF);
    long long int q = get_random_prime(2 * PRIME_OFF);
    struct public_key pub_key;
    struct private_key pvt_key;
    pub_key.n = p*q;
    pvt_key.n = p*q;
    pub_key.e = 65537; // 65537
    long long int phi = (p-1)*(q-1);
    // long long int phi = (p-1) * (q-1);
    printk("phi: %llu\n",phi);
    pvt_key.d = modInverse(pub_key.e, phi);
    struct keys key;
    key.pub_key = pub_key;
    key.pvt_key = pvt_key;
    return key;
}
static long long int encrypt(long long int c, struct public_key pub_key){
    return pow(c, pub_key.e, pub_key.n);
}
static long long int  decrypt(long long int c, struct private_key pvt_key){
    return pow(c, pvt_key.d, pvt_key.n);
}
static void encrypt_data(long long int* data, char* buffer, int size, struct public_key pub_key){
    int i = 0;
    while(i < size){
        data[i] = encrypt((long long int)buffer[i] + i % PRIME_OFF, pub_key);
        printk("encrypting %d to %lld\n", buffer[i], data[i]);
        i++;
    }
}
static void decrypt_data(long long int* data, char* buffer, int size, struct private_key pvt_key){
    int i = 0;
    printk("decrypting using key: %lld %lld\n", pvt_key.d, pvt_key.n);
    while(i < size){
        buffer[i] = (char)(decrypt(data[i], pvt_key) - i % PRIME_OFF);
        printk("decrypting %lld to %d\n", data[i], buffer[i]);
        i++;
    }
}
 
// crc functions
static long long int reverse_char(long long int a, int bit_field){
    long long int c = 0;
    int count = 0;
    // printf("starting rev\n");
    while(count < bit_field){
        c <<= 1;
        c = (c | (a&1));
        a >>= 1;
        count ++;
        // printf("after %d shift number is %llu\n", count, c);
    }
    // printf("done rev\n");
    return c;
}
 
static char crc(char* buffer, int size, char polynomial){
    unsigned long long int holder = 0;
    char result = 0;
    int bits_rem = 0;
    unsigned long long int mask = (1<<8) - 1;
    // printf("mask %llu\n", mask);
    unsigned long long int poly = reverse_char(polynomial, 8);
    // printf("poly nomial is %llu", poly);
    int i = 0;
    while(i < size){
        holder = (holder | ((long long int)reverse_char(buffer[i++], 8)<<bits_rem));
        bits_rem += 8;
        // printf("%llu %i\n", holder, bits_rem);
        while(bits_rem >= 8){
            // printf("dividing %llu %d\n", holder&1, bits_rem);
            while((0 == (1 & holder)) && bits_rem > 0){
                holder >>= 1;
                bits_rem--;
                // printf("shifting\n");
            }
            if(bits_rem < 8) break;
            // printf("entering divider\n");
            // printf("holder before: %llu\n", holder);
            // while(pos < (1<<8)){
               
            //     holder = ((holder & (~pos)) | ((pos & holder) ^ (pos & poly)));
               
            //     pos <<= 1;
            //     // printf("pos: %llu\n", pos);
            // }
            holder = (holder & (~mask)) | ((holder & mask) ^ (poly & mask));
            // printf("holder after: %llu\n", holder);
            // printf("exiting divider\n");
        }
    }
    // return (unsigned char ) holder;
    return reverse_char(holder, bits_rem);
}
 
// kernel module functions
 
int init_module(void)
{  
  // registering the character device
   printk("chardev: init module");
   Major = register_chrdev(0, DEVICE_NAME, &fops);
 
   if (Major < 0) {
     printk ("Registering the character device failed with %d\n", Major);
     return Major;
   }
   initial_open = 1;
   // write information on how to access the device
   printk("<1>I was assigned major number %d.  To talk to\n", Major);
   printk("<1>the driver, create a dev file with\n");
   printk("'mknod /dev/hello c %d 0'.\n", Major);
   printk("<1>Try various minor numbers.  Try to cat and echo to\n");
   printk("the device file.\n");
   printk("<1>Remove the device file and module when done.\n");
   printk("done init module");
 
   return 0;
}
 
 
void cleanup_module(void)
{  
   /* Unregister the device */
   unregister_chrdev(Major, DEVICE_NAME);
}
 
 
/* Methods */
 
/* Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
   idx = 0;
   if (Device_Open) return -EBUSY;
 
   Device_Open++;
   // sprintf(msg,"I already told you %d times Hello world!\n", counter++);
   // msg_Ptr = msg;
 
   return SUCCESS;
}
 
 
/* Called when a process closes the device file */
static int device_release(struct inode *inode, struct file *file)
{
   Device_Open --;     /* We're now ready for our next caller */
   int i = 0;
   /* Decrement the usage count, or else once you opened the file, you'll
                    never get get rid of the module. */
 
   return 0;
}
 
/* Called when a process, which already opened the dev file, attempts to
   read from it.
*/
static ssize_t device_read(struct file *filp,
   char *buff,    /* The buffer to fill with data */
   size_t len,   /* The length of the buffer     */
   loff_t *off)  /* Our offset in the file       */
{
   /* Number of bytes actually written to the buffer */
   // sprintf(msg, "device_read: %ld %lld\n", length, *offset);
  // printk ("read: len: %ld, off: %lld \n", length, *offset);
   // printk(msg);
  printk("reading\n");
   int bytes_read = 0;
   if(initial_open == 1){
   
   // initialising the encrytion parameters.
   struct keys key = gen_keys();
   Pub_Key = key.pub_key;
//    Pvt_Key.n = Pub_Key.n;
//    Pvt_Key.d is not written for security puroses
   Pvt_Key = key.pvt_key; // remove this for security purposes
//    sprintf(buffer, "N is n: %lld\nThe private key is d: %lld\nThe public key is e: %lld\n", key.pvt_key.n, key.pvt_key.d, key.pub_key.e);
    sprintf(buffer, "%lld", Pvt_Key.d);
    // Pvt_Key.d = 0; // actual key removed for security reasons
   printk("%s", buffer);
    initial_open = 0;
    // write the initial data to the device.
    int i = 0;
    while(buffer[i] != 0){
      put_user(buffer[i++], buff++);
    }
    return i;
   }
   /* If we're at the end of the message, return 0 signifying end of file */
   if (idx >= size){
    //    Pvt_Key.d = 0;
    int j=0;
   while(j < BUF_LEN) buffer[j++] = 0;
   Pvt_Key.d = 0;
       return 0;
   }
 
   /* reading data from buffer */
   if(idx == 0){
    decrypt_data(data, buffer, size + 1, Pvt_Key);
    char crc_res = crc(buffer, size + 1, POLYNOMIAL);
    printk("chk crc %d\n", crc_res);
    if(crc_res == 0)put_user('y', buff++);
    else put_user('n', buff++);
    bytes_read++;
   }

   

   
   // remove the key for security reasons after decrypting
   while (len && idx < size)  {
 
        /* The buffer is in the user data segment, not the kernel segment;
         * assignment won't work.  We have to use put_user which copies data from
         * the kernel data segment to the user data segment. */
 
    put_user(buffer[idx], buff++);
    idx++;
    len--;
    bytes_read++;
   }
 
   /* Most read functions return the number of bytes put into the buffer */
   return bytes_read;
}
 
 
/*  Called when a process writes to dev file: echo "hi" > /dev/hello */
static ssize_t device_write(struct file *filp,
   const char *buff,
   size_t len,
   loff_t *off)
{
   int bytes_written = 0;
   copy_from_user(buffer, buff, len);
   if(buffer[0] == '~'){
      //private key incoming
      Pvt_Key.d = 0;
      int i = 1;
      while(i < len && buffer[i] >= '0' && buffer[i] <= '9'){
          Pvt_Key.d = Pvt_Key.d * 10 + (buffer[i] - '0');
          i++;
      }
      printk("pvt key incoming %lld\n", Pvt_Key.d);
   }
   else if(buffer[0] == ':'){
    buffer[len] = 0;
    buffer[len] = crc(buffer+1, len, POLYNOMIAL);
    printk("do crc %d\n", buffer[size]);
    encrypt_data(data, buffer + 1, len, Pub_Key);
    size = len-1;
   }
   return len;
}
 
MODULE_LICENSE("GPL");