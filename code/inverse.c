static long long int modInverse(long long int a, long long int m) 
{ 
    long long int x, y; 
    long long int g = gcdExtended(a, m, &x, &y); 
    if (g != 1) 
        return 0; 
    else
    { 
        // m is added to handle negative x 
        int res = (x%m + m) % m; 
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