#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_THREADS 6

// Estrutura para armazenar argumentos da thread
typedef struct {
    int id;
    sem_t *semaphores;
    int *args;
} thread_args;

void *criador(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Thread Criador em execução.\n");
    // Implementação do criador
}

void *deposito_materia_prima(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Depósito de Matéria Prima com %d unidades.\n", args->args[0]);
    // Implementação do depósito de matéria prima
}

void *celula_fabricacao(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Célula de Fabricação de Canetas - Tempo de fabricação por caneta: %d segundos.\n", args->args[3]);
    // Implementação da célula de fabricação
}

void *controle(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Thread Controle em execução.\n");
    // Implementação do controle
}

void *deposito_canetas(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Depósito de Canetas com capacidade de %d canetas.\n", args->args[4]);
    // Implementação do depósito de canetas
}

void *comprador(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Thread Comprador - Compras a cada %d segundos.\n", args->args[6]);
    // Implementação do comprador
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        printf("Uso: %s <matéria prima> <unidades enviadas> <tempo entre envios> <tempo para fabricar> <capacidade de canetas> <canetas compradas> <tempo entre compras>\n", argv[0]);
        return 1;
    }

    int args[7];
    for (int i = 1; i < 8; i++) {
        args[i - 1] = atoi(argv[i]);
    }

    pthread_t threads[NUM_THREADS];
    sem_t semaphores[NUM_THREADS];
    thread_args t_args[NUM_THREADS];

    // Inicialização de semáforos
    for (int i = 0; i < NUM_THREADS; i++) {
        sem_init(&semaphores[i], 0, 1);
    }

    // Criação de threads
    for (int i = 0; i < NUM_THREADS; i++) {
        t_args[i].id = i;
        t_args[i].semaphores = semaphores;
        t_args[i].args = args;
        if (i == 0) {
            pthread_create(&threads[i], NULL, criador, (void *)&t_args[i]);
        } else if (i == 1) {
            pthread_create(&threads[i], NULL, deposito_materia_prima, (void *)&t_args[i]);
        } else if (i == 2) {
            pthread_create(&threads[i], NULL, celula_fabricacao, (void *)&t_args[i]);
        } else if (i == 3) {
            pthread_create(&threads[i], NULL, controle, (void *)&t_args[i]);
        } else if (i == 4) {
            pthread_create(&threads[i], NULL, deposito_canetas, (void *)&t_args[i]);
        } else if (i == 5) {
            pthread_create(&threads[i], NULL, comprador, (void *)&t_args[i]);
        }
    }

    // Aguardar término das threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Limpeza dos recursos
    for (int i = 0; i < NUM_THREADS; i++) {
        sem_destroy(&semaphores[i]);
    }

    return 0;
}