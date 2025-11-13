#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include "arvore.h"

#define NOME_MAX 50
#define DATA_MAX 20

typedef struct {
    int  CodCliente;
    char Nome[NOME_MAX + 1];
    char DataNascimento[DATA_MAX + 1];
    int  eof; 
} Cliente;


static void rstrip(char *s){
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1]=='\n' || s[n-1]=='\r' || isspace((unsigned char)s[n-1])))
        s[--n] = '\0';
}
static char* trim_inplace(char *s){
    rstrip(s);
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p)+1);
    return s;
}


static int ler_proximo(FILE *f, Cliente *c){
    char linha[512];
    if (!f || !fgets(linha, sizeof(linha), f)) {
        c->eof = 1; c->CodCliente = INT_MAX; return 0;
    }
    trim_inplace(linha);
    if (linha[0]=='\0') { c->eof=1; c->CodCliente=INT_MAX; return 0; }

    char *p1 = strtok(linha, ";");
    char *p2 = strtok(NULL,  ";");
    char *p3 = strtok(NULL,  ";");
    if (!p1 || !p2 || !p3) { c->eof=1; c->CodCliente=INT_MAX; return 0; }

    c->CodCliente = atoi(p1);
    strncpy(c->Nome, p2, NOME_MAX); c->Nome[NOME_MAX]='\0';
    strncpy(c->DataNascimento, p3, DATA_MAX); c->DataNascimento[DATA_MAX]='\0';
    c->eof = 0;
    return 1;
}

static void escrever_registro(FILE *out, const Cliente *c){
    fprintf(out, "%d;%s;%s\n", c->CodCliente, c->Nome, c->DataNascimento);
}


static int ler_lista_de_arquivos(const char *path, char ***particoes_out, char **saida_out){
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Erro abrindo %s\n", path); return 0; }

    size_t cap = 16, n = 0;
    char **lista = (char**)malloc(cap*sizeof(char*));
    if (!lista){ fclose(f); return 0; }

    char linha[1024];
    char *saida = NULL;

    while (fgets(linha, sizeof(linha), f)){
        trim_inplace(linha);
        if (linha[0]=='\0') continue;
        if (!saida){
            saida = (char*)malloc(strlen(linha)+1);
            if (!saida){ fclose(f); free(lista); return 0; }
            strcpy(saida, linha);
        }else{
            if (n == cap){
                cap *= 2;
                char **tmp = (char**)realloc(lista, cap*sizeof(char*));
                if (!tmp){ fclose(f); free(saida); free(lista); return 0; }
                lista = tmp;
            }
            lista[n] = (char*)malloc(strlen(linha)+1);
            if (!lista[n]){ fclose(f); free(saida); for(size_t i=0;i<n;i++) free(lista[i]); free(lista); return 0; }
            strcpy(lista[n], linha);
            n++;
        }
    }
    fclose(f);
    if (!saida || n==0){
        fprintf(stderr, "entrada.txt inválido: precisa de 1 linha de saída + >=1 partição.\n");
        if (saida) free(saida);
        for (size_t i=0;i<n;i++) free(lista[i]);
        free(lista);
        return 0;
    }
    *particoes_out = lista;
    *saida_out = saida;
    return (int)n; 
}


int main(int argc, char **argv){
    const char *arquivo_entrada = (argc >= 2) ? argv[1] : "entrada.txt";

    char **particoes = NULL;
    char *arquivo_saida = NULL;
    int k = ler_lista_de_arquivos(arquivo_entrada, &particoes, &arquivo_saida);
    if (k <= 0) return 1;

    FILE **in = (FILE**)malloc(sizeof(FILE*) * k);
    if (!in){ fprintf(stderr, "Memória insuficiente.\n"); return 1; }
    for (int i=0;i<k;i++){
        in[i] = fopen(particoes[i], "r");
        if (!in[i]){
            fprintf(stderr, "Erro abrindo partição: %s\n", particoes[i]);
            for (int j=0;j<i;j++) fclose(in[j]);
            free(in);
            free(arquivo_saida);
            for (int t=0;t<k;t++) free(particoes[t]);
            free(particoes);
            return 1;
        }
    }

    FILE *out = fopen(arquivo_saida, "w");
    if (!out){
        fprintf(stderr, "Erro criando arquivo de saída: %s\n", arquivo_saida);
        for (int i=0;i<k;i++) fclose(in[i]);
        free(in); free(arquivo_saida);
        for (int t=0;t<k;t++) free(particoes[t]);
        free(particoes);
        return 1;
    }

    Cliente *heads = (Cliente*)malloc(sizeof(Cliente)*k);
    int *values = (int*)malloc(sizeof(int)*k);
    if (!heads || !values){
        fprintf(stderr, "Memória insuficiente.\n");
        fclose(out);
        for (int i=0;i<k;i++) fclose(in[i]);
        free(in); free(arquivo_saida);
        for (int t=0;t<k;t++) free(particoes[t]);
        free(particoes); free(heads); free(values);
        return 1;
    }

  
    for (int i=0;i<k;i++){
        if (ler_proximo(in[i], &heads[i])) values[i] = heads[i].CodCliente;
        else { values[i] = INT_MAX; heads[i].eof = 1; }
    }

    int size = 0;
    int *tree = build_winner_tree(values, k, &size);
    if (!tree){
        fprintf(stderr, "Memória insuficiente (árvore).\n");
        fclose(out);
        for (int i=0;i<k;i++) fclose(in[i]);
        free(in); free(arquivo_saida);
        for (int t=0;t<k;t++) free(particoes[t]);
        free(particoes); free(heads); free(values);
        return 1;
    }

  
    while (1){
        int w = tree[1];                 
        if (w == -1) break;              
        if (values[w] == INT_MAX) break; 

        escrever_registro(out, &heads[w]);

        
        if (ler_proximo(in[w], &heads[w])){
            update_value(values, tree, size, w, heads[w].CodCliente);
        }else{
            update_value(values, tree, size, w, INT_MAX);
        }
    }

  
    fclose(out);
    for (int i=0;i<k;i++) fclose(in[i]);
    free(in);
    free(tree);
    free(heads);
    free(values);
    free(arquivo_saida);
    for (int t=0;t<k;t++) free(particoes[t]);
    free(particoes);
    return 0;
}