#include "printf.h"
#include "uart.h"

void main(void) {
    uart_init();

    // Basic pointer review: What's the difference between incrementing a `char
    // *` and a `int *`?
    printf("\nPointer review:\n");
    printf("===============\n");
    char *cptr = "Hello";
    int iarray[] = {0, 1, 2};
    int *iptr = iarray;

    printf("cptr   = %p\n", cptr);
    printf("cptr+1 = %p\n", cptr + 1);

    printf("iptr   = %p\n", iptr);
    printf("iptr+1 = %p\n", iptr + 1);

    // What happens if we access elements beyond end of the array?
    printf("\nAccess array element out of bounds: \n");
    for (int i = 0; i < 5; ++i) {
        printf("[%d] @%p = %x\n", i, iptr + i, iptr[i]);
    }
    printf("\n");

    printf("\nSingle-dimensional arrays and pointers\n");
    printf("======================================\n");
    int f[] = { 0, 1 };
    int g[] = { 2, 3, 4 };
    // the declaration commented out below does not work, why not?
    //int *h = {2, 3, 4};
    printf("f = %p\n", f);
    printf("g = %p\n", g);

    int *p[2] = { f, g };
    int *q = p[0];
    printf("p = %p\n", p);
    printf("q = %p\n", q);
    printf("p[0] = %p\n", p[0]);
    printf("p[1] = %p\n", p[1]);

    printf("\nPointers to pointers example\n");
    printf("==============================\n");
    const char *tokens[2] = { "Hello", "World" };
    printf("tokens   = %p \n", tokens);
    printf("tokens+1 = %p \n", tokens + 1);
    printf("tokens[0] = %p ('%s')\n", tokens[0], tokens[0]);
    printf("tokens[1] = %p ('%s')\n", tokens[1], tokens[1]);
    printf("*tokens[0] = %c\n", *tokens[0]);
    printf("*tokens[1] = %c\n", *tokens[1]);
    printf("**tokens = %c\n", **tokens);
    printf("**(tokens + 1) = %c\n", **(tokens + 1));
    printf("*(*tokens + 1) = %c\n", *(*tokens + 1));
    printf("**tokens + 1 = %c\n", **tokens + 1);

    printf("\nMulti-dimensional arrays and pointers\n");
    printf("=====================================\n");
    // how does the 2-d array `a` compare to layout of array of pointers `p` above?
    int a[2][2] = { {0, 1}, {2, 3} };
    int *b = &a[0][0];
    int (*c)[2] = a;
    int (*d)[2][2] = &a;

    printf("a = %p\n", a );
    printf("&a[0][0] = %p\n", &a[0][0] );
    printf("&a[0][1] = %p\n",  &a[0][1]);

    printf("b   = %p\n", b );
    printf("b+1 = %p\n", b+1);
    printf("b+2 = %p\n", b+2);
    printf("&a[0] = %p\n", &a[0]);
    printf("&a[1] = %p\n", &a[1]);
    printf("c   = %p\n", c );
    printf("c+1 = %p\n", c+1);
    printf("d   = %p\n", d );
    printf("d+1 = %p\n", d+1);
}
