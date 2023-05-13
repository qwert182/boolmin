#include <assert.h>

enum gate_id_t {
    GATE_NONE,
    GATE_NOT,
    GATE_NAND2,
    GATE_NAND3,
    GATE_NAND4,
    GATE_NOR2,
    GATE_NOR3,
    GATE_NOR4,
    GATE_XOR2,
    GATE_ID_NUMBER,
};

enum {
    NOT_SYMMETRIC, SYMMETRIC
};

struct gate_t {
    int inputs_number;
    int outputs_number;
    char name[8];
    char formula[31];
    unsigned is_symmetric : 1;
};
//_Static_assert(sizeof(struct gate_t) == 48);

static const struct gate_t gates[] = {
    {0, 0, "", "", NOT_SYMMETRIC},
    {1, 1, "NOT", "o1 = !i1", SYMMETRIC},
    {2, 1, "NAND2", "o1 = !(i1 && i2)", SYMMETRIC},
    {3, 1, "NAND3", "o1 = !(i1 && i2 && i3)", SYMMETRIC},
    {4, 1, "NAND4", "o1 = !(i1 && i2 && i3 && i4)", SYMMETRIC},
    {2, 1, "NOR2", "o1 = !(i1 || i2)", SYMMETRIC},
    {3, 1, "NOR3", "o1 = !(i1 || i2 || i3)", SYMMETRIC},
    {4, 1, "NOR4", "o1 = !(i1 || i2 || i3 || i4)", SYMMETRIC},
    {2, 1, "XOR2", "o1 = !!i1 ^ !!i2", SYMMETRIC},
};

struct device_t {
    int gates_number;
    enum gate_id_t gates[6];
    const char name[8];
};
//_Static_assert(sizeof(struct device_t) == 36);

enum device_id_t {
    DEVICE_NONE,
    DEVICE_6NOT,
    DEVICE_4NAND2,
    DEVICE_3NAND3,
    DEVICE_2NAND4,
    DEVICE_4NOR2,
    DEVICE_3NOR3,
    DEVICE_2NOR4,
    DEVICE_4XOR2,
    DEVICE_ID_NUMBER,
};

static const struct device_t devices[] = {
    {0, {GATE_NONE}, ""},
    {2, {GATE_NOT, GATE_NOT, GATE_NOT, GATE_NOT, GATE_NOT, GATE_NOT}, "2NOT"}, //{6, {GATE_NOT, GATE_NOT, GATE_NOT, GATE_NOT, GATE_NOT, GATE_NOT}, "6NOT"},
    {2, {GATE_NAND2, GATE_NAND2}, "2NAND2"}, //{4, {GATE_NAND2, GATE_NAND2, GATE_NAND2, GATE_NAND2}, "4NAND2"},
    {3, {GATE_NAND3, GATE_NAND3, GATE_NAND3}, "3NAND3"},
    {2, {GATE_NAND4, GATE_NAND4}, "2NAND4"},
    {4, {GATE_NOR2, GATE_NOR2, GATE_NOR2, GATE_NOR2}, "4NOR2"},
    {3, {GATE_NOR3, GATE_NOR3, GATE_NOR3}, "3NOR3"},
    {2, {GATE_NOR4, GATE_NOR4}, "2NOR4"},
    {4, {GATE_XOR2, GATE_XOR2, GATE_XOR2, GATE_XOR2}, "4XOR2"},
};

struct devset_t {
    int size;
    int set[DEVICE_ID_NUMBER];
};

/*
long total_devsets_number;
static void recurse_device(struct devset_t *devset, int devices_number_to_add) {
    if (devset->size < devices_number_to_add) {
        for (int device_id = 0; device_id < DEVICE_ID_NUMBER; ++device_id) {
            ++devset->set[device_id];
            ++devset->size;
            recurse_device(devset, devices_number_to_add);
            --devset->set[device_id];
            --devset->size;
        }
    } else {
        printf("devset[%d] = {", devset->size);
        int first = 1;
        for (int device_id = 0; device_id < DEVICE_ID_NUMBER; ++device_id) {
            if (devset->set[device_id]) {
                printf(first ? "%d * %s" : ", %d * %s", devset->set[device_id], devices[device_id].name);
                first = 0;
            }
        }
        printf("}\n");
        ++total_devsets_number;
    }
}

int main() {
    int max_devices_number = 5;
    struct devset_t devset;
    memset(&devset, 0, sizeof devset);
    recurse_device(&devset, max_devices_number);
    printf("total_devsets_number = %ld\n", total_devsets_number);
}
*/

static long factorial(long n) {
    long result = 1;
    for (long i = 2; i <= n; ++i)
        result *= i;
    return result;
}

static long combinations_number(long n, long k) {
    long result = 1, min, max;
    assert(k <= n);
    if (n - k > k)
        max = n - k, min = k;
    else
        max = k, min = n - k;
    for (long i = max + 1; i <= n; ++i)
        result *= i;
    result /= factorial(min);
    assert(result * (factorial(k) * factorial(n - k)) == factorial(n));
    return result;
}

static long multiset_number(long n, long k) {
    long result = 1, min, max;
    if (n - 1 > k)
        max = n - 1, min = k;
    else
        max = k, min = n - 1;
    for (long i = max + 1; i < n + k; ++i)
        result *= i;
    result /= factorial(min);
    assert(result * (factorial(k) * factorial(n - 1)) == factorial(n + k - 1));
    return result;
}

static long _power(long n, int exp) {
    long result = 1;
    for (int i = 0; i < exp; ++i)
        result *= n;
    return result;
}

static long power(long n, int exp) {
    long result = 1, _n = n;
    int _exp = exp;
    if (exp < 0)
        return 0;
    while (exp) {
        if (exp & 1)
            result *= n;
        n *= n;
        exp >>= 1;
    }
    assert(result == _power(_n, _exp));
    return result;
}
