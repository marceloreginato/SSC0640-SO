#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_THREADS 6

int entrada[7]; //Vai armazenar os argumentos passados na entrada
pthread_t threads[NUM_THREADS];

//Variáveis globais
int materia_prima_disponivel; 
int slots_canetas_disponiveis;
int canetas_compradas;

//Declaração dos semáforos
pthread_mutex_t mutex_dep_materia_prima;
pthread_mutex_t mutex_celula_fabricacao; 
pthread_mutex_t mutex_controle;
pthread_mutex_t mutex_dep_canetas;
pthread_mutex_t mutex_comprador; 

//Variáveis de condição
pthread_cond_t dep_materiaprima_vazios;
pthread_cond_t dep_canetas_slots_vazios, dep_canetas_slots_cheio;


//Declaração das funções
void *criador(void *arg);
void *deposito_materia_prima(void *arg);
void *celula_fabricacao(void *arg);
void *controle(void *arg);
void *deposito_canetas(void *arg);
void *comprador(void *arg);

//Declaração e criação dos semáforos
sem_t available, go1, go2, full; //FALTA TERMINAR ESSE KRL

/*
void *deposito_materia_prima(void *arg) {
    //printf("Thread Depósito de Matéria Prima iniciada com %d unidades.\n");

    while (1) {
         sem_wait(&go1); // Espera liberação para proceder, espera para prosseguir

        if (entrada[0] > 0) {
            entrada[0]--; // Decrementa a matéria-prima disponível
            printf("Matéria-prima liberada para fabricação. Matéria-prima restante: %d\n", entrada[0]);
            sem_post(&go2); 
        } else {
            printf("Depósito de Matéria Prima: Esgotado!\n");
            sem_post(&go1); 
            break; 
        }
        sleep(1); // Ajustar ainda
    }

    printf("Thread Depósito de Matéria Prima finalizando...\n");
    pthread_exit(NULL);
}
*/

void *criador(void *arg) {
    printf("Thread Criador em execução.\n");

    // Criando threads
    if(pthread_create(&threads[1], NULL, deposito_materia_prima, NULL)) {
        printf("ERRO -- pthread_create - deposito de materia prima\n");
        pthread_exit((void *) -1);
    }
    if(pthread_create(&threads[2], NULL, celula_fabricacao, NULL)) {
        printf("ERRO -- pthread_create - celula de fabricacao\n");
        pthread_exit((void *) -1);
    }
    if(pthread_create(&threads[3], NULL, controle, NULL)) {
        printf("ERRO -- pthread_create - controle\n");
        pthread_exit((void *) -1);
    }
    if(pthread_create(&threads[4], NULL, deposito_canetas, NULL)) {
        printf("ERRO -- pthread_create - deposito de canetas\n");
        pthread_exit((void *) -1);
    }
    if(pthread_create(&threads[5], NULL, comprador, NULL)) {
        printf("ERRO -- pthread_create - comprador\n");
        pthread_exit((void *) -1);
    }

    printf("Threads criadas\n");

    pthread_exit(NULL);
}

void *deposito_materia_prima(void *arg) {
    //printf("Thread Depósito de Matéria Prima iniciada com %d unidades.\n");
    int quantidade_materia_prima = entrada[0]; //quantidade de matéria prima no depósito
    int quant_materia_prima_a_ser_enviada = entrada[1]; //quantidade de matéria prima que deve ser enviada a cada iteração
    int tempo_de_envio = entrada[3]; //tempo de envio de cada iteração

    while (1) {
        if(quantidade_materia_prima != 0){
            //Significa que tem materia prima disponível para enviar
            if(quantidade_materia_prima >= quant_materia_prima_a_ser_enviada && quant_materia_prima_a_ser_enviada > 0) {
                //Significa que tem mais matéria prima do que a quantidade que tem que enviar a cada iteração
                //e tem quantidade para ser enviada
                pthread_mutex_lock(&mutex_dep_materia_prima); //Vamos mexer em uma região crítica
                printf("Enviando materia prima...\n");
                printf("Entrou na quantidade de materia prima != 0\n");
                materia_prima_disponivel += quant_materia_prima_a_ser_enviada;
                quantidade_materia_prima -= quant_materia_prima_a_ser_enviada;
                //materia prima enviada
                printf("Materia prima enviada\n");
                pthread_mutex_unlock(&mutex_dep_materia_prima);
            }
            else if(quantidade_materia_prima < quant_materia_prima_a_ser_enviada && quant_materia_prima_a_ser_enviada > 0) {
                //Significa que tem menos materia prima disponível do que a quantidade que teriamos que enviar, logo, enviaremos tudo
                pthread_mutex_lock(&mutex_dep_materia_prima);
                printf("Entrou na quantidade de materia prima < quantidade a ser enviada\n");
                printf("Enviando materia prima...\n");
                materia_prima_disponivel += quant_materia_prima_a_ser_enviada;
                quantidade_materia_prima -= quant_materia_prima_a_ser_enviada;
                pthread_mutex_unlock(&mutex_dep_materia_prima);
            }
            else if(quant_materia_prima_a_ser_enviada <= 0) {
                //Não tem pedido de matéria prima
                printf("Nao tem pedido de materia prima para a celula de fabricacao\n");
            }
        }
        else {
            //Quantidade matéria prima acabou
            pthread_mutex_lock(&mutex_dep_materia_prima);
            printf("Acabou a materia prima...Parando o envio\n");
            pthread_cond_wait(&dep_materiaprima_vazios, &mutex_dep_materia_prima);
        }
        sleep(tempo_de_envio);
    }

    printf("Thread Depósito de Matéria Prima finalizando...\n");
    pthread_exit(NULL);
}

void *celula_fabricacao(void *arg) {
    //printf("Célula de Fabricação de Canetas - Tempo de fabricação por caneta: %d segundos.\n");

    while (1) {
        
        sleep(5);
        pthread_mutex_lock(&mutex_dep_materia_prima);
        printf("Travei o depósito de matéria prima\n");
        sleep(5);
        printf("Destravei o depósito\n");
        pthread_mutex_unlock(&mutex_dep_materia_prima);
    }

    pthread_exit(NULL);
}

void *controle(void *arg) {
    printf("Thread Controle em execução, coordenando operações.\n");

    while (1) {
        
    }

    pthread_exit(NULL);
}


void *deposito_canetas(void *arg) {
    //printf("Depósito de Canetas com capacidade de %d canetas.\n");

    while (1) {
        sem_wait(&full); // Espera que uma caneta seja produzida

        if (entrada[4] > 0) {
            entrada[4]--; // Decrementa a quantidade de canetas no depósito.
            printf("Caneta retirada para venda. Canetas restantes: %d\n", entrada[4]);
            sem_post(&available); 
        } else {
            printf("Depósito de Canetas vazio. Aguardando produção...\n");
            sem_post(&full); 
        }

        if (entrada[4] == 0) { // Verifica se o depósito está vazio
            printf("Depósito vazio. Thread Depósito de Canetas encerrada.\n");
            break;
        }

        sleep(1); // Ajustar ainda!!!
    }

    pthread_exit(NULL);
}


void *comprador(void *arg) {
    //printf("Thread Comprador - Compras a cada %d segundos.\n");

    while (1) { 
        
    }

    printf("Depósito vazio. Thread Comprador encerrada.\n");
    pthread_exit(NULL);
}


/*
void *criador(void *arg) {
    printf("Thread Criador em execução.\n");

    pthread_t threads[NUM_THREADS];

    //Inicialização dos semáforos
    sem_init(&available, 0, entrada[4]); //Semáforo contador para determinar o número de slots disponíveis no Rank 4
    sem_init(&go1, 0, 1); //Mutex para liberar a ação do depósito de matéria prima
    sem_init(&go2, 0, 1); //Mutex para liberar a ação da célula de fabricação de canetas
    sem_init(&full, 0, entrada[0]); //Mutex para liberar a ação do depósito de matéria prima

    // Criando threads
    pthread_create(&threads[1], NULL, deposito_materia_prima, NULL);
    pthread_create(&threads[2], NULL, celula_fabricacao, NULL);
    pthread_create(&threads[3], NULL, controle, NULL);
    pthread_create(&threads[4], NULL, deposito_canetas, NULL);
    pthread_create(&threads[5], NULL, comprador, NULL);

    // Inicialização: Pode configurar ou verificar o estado inicial da fábrica.
    printf("Verificando a disponibilidade inicial de recursos...\n");
    //sem_wait(&args->semaphores[1]); // Espera pelo semáforo do depósito de matéria prima.

    

    //sem_post(&args->semaphores[1]); // Libera o semáforo do depósito de matéria prima.
    printf("Thread Criador finalizando operações.\n");
    pthread_exit(NULL);
}
*/

/*
int main(int argc, char *argv[]) {
    if (argc != 8) {
        printf("Uso: %s <matéria prima> <unidades enviadas> <tempo entre envios> <tempo para fabricar> <capacidade de canetas> <canetas compradas> <tempo entre compras>\n", argv[0]);
        return 1;
    }

    //Lendo a entrada passada pelo makefile e armazenando-a no vetor global, fazendo a separação de argumentos de acordo  
    for (int i = 1; i < 8; i++) {
        entrada[i - 1] = atoi(argv[i]);
    }

    //Declarando o vetor de threads
    pthread_t threads[NUM_THREADS];

    // Criação de threads
    pthread_create(&threads[0], NULL, criador, NULL);

    // Aguardar término das threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
*/

int main(int argc, char *argv[]) {
    if (argc != 8) {
        printf("Uso: %s <matéria prima> <unidades enviadas> <tempo entre envios> <tempo para fabricar> <capacidade de canetas> <canetas compradas> <tempo entre compras>\n", argv[0]);
        return 1;
    }

    //Lendo a entrada passada pelo makefile e armazenando-a no vetor global, fazendo a separação de argumentos de acordo  
    for (int i = 1; i < 8; i++) {
        entrada[i-1] = atoi(argv[i]);
    }

    // Criação de threads
    if(pthread_create(&threads[0], NULL, criador, NULL)) {
        printf("ERRO -- pthread_create\n");
        return 0;
    }


    pthread_exit(NULL);
    return 0;
}