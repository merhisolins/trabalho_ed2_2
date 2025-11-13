#include <stdlib.h>

static int next_pow2(int n) {
    int p = 1;
    while (p < n) p <<= 1;
    return p;
}

static int winner_index(int a, int b, int *values) {
    if (a == -1) return b;
    if (b == -1) return a;
    return (values[a] <= values[b]) ? a : b; 
}

int *build_winner_tree(int *values, int n, int *size_out) {
    int size = next_pow2(n);
    int tree_size = 2 * size;
    int *tree = (int*)malloc(tree_size * sizeof(int));
    if (!tree) return NULL;

    
    for (int i = 0; i < size; ++i) {
        tree[size + i] = (i < n) ? i : -1;
    }
    
    for (int i = size - 1; i >= 1; --i) {
        tree[i] = winner_index(tree[2 * i], tree[2 * i + 1], values);
    }
    *size_out = size;
    return tree;
}

void update_value(int *values, int *tree, int size, int pos0, int newval) {
    values[pos0] = newval;
    int node = (size + pos0) / 2;
    while (node >= 1) {
        tree[node] = winner_index(tree[2 * node], tree[2 * node + 1], values);
        node /= 2;
    }
}