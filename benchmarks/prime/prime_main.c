#include <stdio.h>
#include <stdbool.h>

#include "util.h"

bool isPrime(int num) {
    if (num <= 1) return false;
    if (num <= 3) return true;
    
    // Check if n is divisible by 2 or 3
    if (num % 2 == 0 || num % 3 == 0) return false;
    
    // Check for factors up to square root of n
    for (int i = 5; i * i <= num; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

int findNPrimes(int n) {
    if (n <= 0) {
        printf("Please enter a positive number.\n");
        return 0;
    }
    
    printf("First %d prime numbers are:\n", n);
    
    int count = 0;
    int num = 2;
    int last_prime = 0;
    
    while (count < n) {
        if (isPrime(num)) {
            // printf("%d ", num);
	    last_prime = num;
            count++;
        }
        num++;
    }
    // printf("\n");
    return last_prime;
}

int main() {
    // int n=1000000;
    int n=4000000;
    printf("Looking for %d first prime numbers:\n", n);
    int last_prime = findNPrimes(n);
    printf("Last prime is %d\n", last_prime);
    return 0;
}

