/*
 * main.c
 * Driver program — calls the recursive factorial function and prints the result.
 */

#include <stdio.h>

int factorial(int n);

int main(void)
{
    int n      = 5;
    int result = factorial(n);

    printf("%d! = %d\n", n, result);   /* expected: 5! = 120 */

    return 0;
}
