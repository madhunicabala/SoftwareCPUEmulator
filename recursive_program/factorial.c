/*
 * factorial.c
 * Recursive factorial function.
 *
 * Convention (mirrors the Emu16 calling convention):
 *   - argument passed in, result returned via return value
 *   - each call pushes a stack frame holding the saved argument
 */

int factorial(int n)
{
    if (n <= 1)
        return 1;

    return n * factorial(n - 1);
}
