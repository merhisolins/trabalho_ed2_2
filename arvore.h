#ifndef ARVORE_H
#define ARVORE_H

int *build_winner_tree(int *values, int n, int *size_out);

void update_value(int *values, int *tree, int size, int pos0, int newval);

#endif