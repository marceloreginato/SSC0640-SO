#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_THREADS 6

typedef struct {
    int id;
    sem_t *semaphores;
    int *args;
} thread_args;

void *criador(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Thread Criador em execução.\n");

    // Inicialização: Pode configurar ou verificar o estado inicial da fábrica.
    printf("Verificando a disponibilidade inicial de recursos...\n");
    sem_wait(&args->semaphores[1]); // Espera pelo semáforo do depósito de matéria prima.

    if (args->args[0] > 0) { // Checa se há matéria prima disponível
        printf("Matéria prima disponível. Iniciando a fabricação de canetas...\n");
        sem_post(&args->semaphores[2]); // Sinaliza para a Célula de Fabricação iniciar a produção.

        // Monitoramento contínuo da produção
        while (1) {
            sleep(1); // Simulação de tempo de monitoramento
            sem_wait(&args->semaphores[4]); // Espera pelo semáforo do depósito de canetas.
            printf("Monitorando a produção de canetas. Canetas disponíveis para venda: %d\n", args->args[4]);
            sem_post(&args->semaphores[4]);

            // Condição de parada (pode ser ajustada conforme a necessidade)
            if (args->args[4] >= 100) { // Supõe-se que 100 canetas é a capacidade máxima para o exemplo
                printf("Capacidade máxima alcançada. Parando a produção...\n");
                break;
            }
        }
    } else {
        printf("Matéria prima insuficiente para iniciar a produção.\n");
    }

    sem_post(&args->semaphores[1]); // Libera o semáforo do depósito de matéria prima.
    printf("Thread Criador finalizando operações.\n");
    pthread_exit(NULL);
}


void *deposito_materia_prima(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Thread Depósito de Matéria Prima iniciada com %d unidades.\n", args->args[0]);

    while (1) {
        sem_wait(&args->semaphores[1]);  // Espera requisição para liberar matéria-prima
        
        if (args->args[0] <= 0) {
            printf("Depósito de Matéria Prima: Esgotado!\n");
            sem_post(&args->semaphores[1]);  // Libera o semáforo se não há matéria-prima
            break;  // Encerra a thread se não há mais matéria-prima
        }

        // Simula o envio de matéria-prima para a fabricação
        printf("Enviando matéria-prima para fabricação...\n");
        args->args[0]--;  // Reduz a quantidade de matéria-prima disponível
        sleep(1);  // Simula o tempo de processamento

        sem_post(&args->semaphores[2]);  // Notifica a célula de fabricação que a matéria-prima está disponível
    }

    printf("Thread Depósito de Matéria Prima finalizando...\n");
    pthread_exit(NULL);
}


void *controle(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Thread Controle em execução, coordenando operações.\n");

    while (1) {
        sleep(10);  // Intervalo para verificação das condições da fábrica

        if (args->args[0] <= 0) {
            printf("Controle detectou falta de matéria-prima.\n");
            sem_post(&args->semaphores[1]);  // Solicita matéria-prima
        }

        if (args->args[4] >= 100) {  // Checa se o depósito de canetas está cheio
            printf("Controle detectou que o depósito de canetas está cheio.\n");
            break;  // Encerra a fábrica se atingir a capacidade máxima
        }
    }

    pthread_exit(NULL);
}


void *deposito_canetas(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Depósito de Canetas com capacidade de %d canetas.\n", args->args[4]);

    while (1) {
        sem_wait(&args->semaphores[4]);  // Espera caneta ser produzida
        args->args[4]++;  // Incrementa o contador de canetas
        printf("Caneta adicionada ao depósito, totalizando %d canetas.\n", args->args[4]);

        if (args->args[4] >= 100) {  // Capacidade máxima do depósito
            printf("Depósito cheio. Não é possível armazenar mais canetas.\n");
            sem_post(&args->semaphores[3]);  // Avisa ao controle sobre a capacidade máxima
            break;
        }
    }

    pthread_exit(NULL);
}


void *comprador(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Thread Comprador - Compras a cada %d segundos.\n", args->args[6]);

    while (args->args[4] > 0) {  // Continua comprando enquanto houver canetas no depósito
        sleep(args->args[6]);  // Espera o intervalo entre compras
        sem_wait(&args->semaphores[4]);  // Espera para acessar o depósito

        if (args->args[4] > 0) {
            args->args[4]--;  // Compra uma caneta
            printf("Comprador comprou uma caneta, restam %d canetas.\n", args->args[4]);
        } else {
            printf("Comprador não pôde comprar, depósito vazio.\n");
        }

        sem_post(&args->semaphores[4]);  // Libera acesso ao depósito
    }

    printf("Depósito vazio. Thread Comprador encerrada.\n");
    pthread_exit(NULL);
}








void *celula_fabricacao(void *arg) {
    thread_args *args = (thread_args *) arg;
    printf("Célula de Fabricação de Canetas - Tempo de fabricação por caneta: %d segundos.\n", args->args[3]);

    while (1) {
        sem_wait(&args->semaphores[2]);  // Espera sinal para começar a fabricação

        if (args->args[0] > 0) {  // Checa se há matéria-prima
            sleep(args->args[3]);  // Simula o tempo de fabricação de uma caneta
            printf("Uma caneta foi fabricada.\n");
            args->args[0]--;  // Decrementa a matéria-prima
            sem_post(&args->semaphores[4]);  // Incrementa o número de canetas no depósito
        } else {
            printf("Sem matéria-prima para continuar a fabricação.\n");
            sem_post(&args->semaphores[1]);  // Sinaliza que precisa de mais matéria-prima
            break;
        }
    }

    pthread_exit(NULL);
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