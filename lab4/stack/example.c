
static int first(char *str) {
    return *str;
}

static int add(int x, int y) {
    return x + first("Mango Pi rules!") + y;
}

static int sum(int *arr, int n) {
    int total = 0;
    for (int i = 0; i < n; i++)
         total += arr[i];
     return total;
}

int make_array(void) {
    int array[6];
    array[2] = 9;
    array[0] = 13;
    return sum(array, 6);
}

static int combine(int x, int y) {
    return add(y, x);
}

static int follow_me(int num) {
    return make_array() + add(num, 7);
}

int main(void) {
    int a = combine(5, 7);
    return follow_me(a);
}
