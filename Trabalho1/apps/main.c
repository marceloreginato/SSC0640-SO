#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_THREADS 6

int entrada[7]; //Vai armazenar os argumentos passados na entrada
pthread_t threads[NUM_THREADS];
//Variáveis globais
int materia_prima_disponivel = 0; //Matéria prima disponível na célula de fabricação 
int canetas_no_deposito = 0; //Quantidade de canetas no depósito de canetas
int slots_canetas_disponiveis = 0; //Capacidade máxima de canetas a serem guardadas
int canetas_compradas = 0;

//Declaração dos semáforos
pthread_mutex_t mutex_materia_prima;
pthread_mutex_t mutex_canetas;
pthread_mutex_t mutex_controle_depositos_primarios; //Variável para o controle do depósito de matéria prima e da célula de fabricação
int controle_depositos_primarios = 0; //Variável de controle para os depósitos, inicia bloqueando até que o controle desbloqueie 
pthread_mutex_t mutex_comprador; 

//Variáveis de condição
pthread_cond_t dep_materiaprima_vazios;
pthread_cond_t dep_canetas_cheio, dep_canetas_com_espaco;
pthread_cond_t controle_destrava;


//Declaração das funções
void *criador(void *arg);
void *deposito_materia_prima(void *arg);
void *celula_fabricacao(void *arg);
void *controle(void *arg);
void *deposito_canetas(void *arg);
void *comprador(void *arg);

void *criador(void *arg) {
    printf("Thread Criador em execução.\n");
    printf("Threads criadas\n");

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


    pthread_exit(NULL);
}

void *deposito_materia_prima(void *arg) {
    //printf("Thread Depósito de Matéria Prima iniciada com %d unidades.\n");
    int quantidade_materia_prima = entrada[0]; //quantidade de matéria prima no depósito
    int quant_materia_prima_a_ser_enviada = entrada[1]; //quantidade de matéria prima que deve ser enviada a cada iteração
    int tempo_de_envio = entrada[3]; //tempo de envio de cada iteração

    while (1) {
        pthread_mutex_lock(&mutex_controle_depositos_primarios);
        if(controle_depositos_primarios != 0) { //Testando uma região crítica
            pthread_mutex_unlock(&mutex_controle_depositos_primarios);
            if(quantidade_materia_prima != 0){
                //Significa que tem materia prima disponível para enviar
                if(quantidade_materia_prima >= quant_materia_prima_a_ser_enviada && quant_materia_prima_a_ser_enviada > 0) {
                    //Significa que tem mais matéria prima do que a quantidade que tem que enviar a cada iteração
                    //e tem quantidade para ser enviada
                    quantidade_materia_prima -= quant_materia_prima_a_ser_enviada;
                    pthread_mutex_lock(&mutex_materia_prima); //Vamos mexer em uma região crítica
                    printf("Enviando materia prima...\n");
                    materia_prima_disponivel += quant_materia_prima_a_ser_enviada;
                    //Materia prima enviada
                    pthread_cond_signal(&dep_materiaprima_vazios);
                    printf("Materia prima enviada\n");
                    pthread_mutex_unlock(&mutex_materia_prima);
                }
                else if(quantidade_materia_prima < quant_materia_prima_a_ser_enviada && quant_materia_prima_a_ser_enviada > 0) {
                    //Significa que tem menos materia prima disponível do que a quantidade que teriamos que enviar, logo, enviaremos tudo
                    pthread_mutex_lock(&mutex_materia_prima);
                    printf("Entrou na quantidade de materia prima < quantidade a ser enviada\n");
                    printf("Enviando materia prima...\n");
                    materia_prima_disponivel += quant_materia_prima_a_ser_enviada;
                    quantidade_materia_prima -= quant_materia_prima_a_ser_enviada;
                    //Matéria prima enviada
                    pthread_cond_signal(&dep_materiaprima_vazios);
                    pthread_mutex_unlock(&mutex_materia_prima);
                }
                else if(quant_materia_prima_a_ser_enviada <= 0) {
                    //Não tem pedido de matéria prima
                    printf("Nao tem pedido de materia prima para a celula de fabricacao\n");
                }
            }
            else {
                //Quantidade matéria prima acabou
                printf("Acabou a materia prima...Parando o envio\n");
                pthread_exit(NULL);
            }
            sleep(tempo_de_envio);
        }
        else {
            printf("PARANDO DEPÓSITO DE MATÉRIA PRIMA\n");
            pthread_cond_wait(&controle_destrava, &mutex_controle_depositos_primarios); //O Controle mandou o sinal pelo controle_depositos (0 ou 1), o que travou a thread, a qual está esperando um sinal do controle para destravar
            printf("Voltando a enviar matéria prima...\n");
            pthread_mutex_unlock(&mutex_controle_depositos_primarios);
        }
        
    }
    
    pthread_exit(NULL);
}

void *celula_fabricacao(void *arg) {
    //printf("Célula de Fabricação de Canetas - Tempo de fabricação por caneta: %d segundos.\n");
    int canetas_fabricadas = 0;
    int tempo_fabricacao_caneta = entrada[3];

    while (1) {
        pthread_mutex_lock(&mutex_controle_depositos_primarios);
        if(controle_depositos_primarios != 0) { //Testando uma região crítica
            pthread_mutex_unlock(&mutex_controle_depositos_primarios);
            //Armazenando localmente o estoque disponível na célula de fabricação
            pthread_mutex_lock(&mutex_materia_prima); //Entrando na região crítica
            if(materia_prima_disponivel > 0) {
                printf("Temos matéria prima disponível\n");
                materia_prima_disponivel--; //A célula de fabricação está pegando uma unidade de matéria prima
                pthread_mutex_unlock(&mutex_materia_prima); //Liberando a região crítica
            }
            else {
                //Não temos matéria prima disponível
                printf("Acabou a materia prima disponivel\n");
                pthread_cond_wait(&dep_materiaprima_vazios, &mutex_materia_prima); //Espera até que haja matéria prima disponível
                printf("Temos matéria prima disponível novamente\n");
                //Após o sinal de matéria prima chegar, então podemos retirar uma unidade
                materia_prima_disponivel--; 
                pthread_mutex_unlock(&mutex_materia_prima);
            }

            //Após pegar a matéria prima, fabricaremos uma caneta
            //Fabricando uma caneta
            printf("Fabricando uma caneta...\n");
            sleep(tempo_fabricacao_caneta);
            canetas_fabricadas++;
            printf("Caneta fabricada\n");

            //Após a fabricação, enviamos a caneta para o depósito de canetas
            canetas_fabricadas--; //Primeiro, pegamos a caneta
            pthread_mutex_lock(&mutex_canetas); //Região crítica para o depósito de canetas
            printf("Enviando uma caneta para o depósito...\n");
            canetas_no_deposito++; //Enviamos a caneta para o depósito
            slots_canetas_disponiveis--; //Atualizamos a quantidade de slots disponíveis na quantidade de caneta
            printf("Caneta enviada\n");
            pthread_mutex_unlock(&mutex_canetas);
        }
        else {
            printf("PARANDO CÉLULA DE FABRICAÇÃO\n");
            pthread_cond_wait(&controle_destrava, &mutex_controle_depositos_primarios); //O Controle mandou o sinal pelo controle_depositos (0 ou 1), o que travou a thread, a qual está esperando um sinal do controle para destravar
            printf("Voltando a fabricar canetas...\n");
            pthread_mutex_unlock(&mutex_controle_depositos_primarios);
        }
    }

    pthread_exit(NULL);
}

void *controle(void *arg) {
    printf("Controle em execução, coordenando operações.\n");

    while (1) {
        pthread_mutex_lock(&mutex_canetas); //Acessando uma região crítica relacionada ao depósito de canetas
        if(slots_canetas_disponiveis == 0) { //Significa que o depósito de canetas esta cheio
            printf("ENTROU!!!\n");
            pthread_mutex_unlock(&mutex_canetas);
            pthread_mutex_lock(&mutex_controle_depositos_primarios); //Acessando uma região crítica
            printf("Depósito de canetas CHEIO!!!\n");
            controle_depositos_primarios = 0; //Atribuindo o valor de parada para a célula de fabricação e para o depósito de matéria prima
            pthread_cond_wait(&dep_canetas_com_espaco, &mutex_controle_depositos_primarios); //Travando a thread até receber um sinal do depósito de canetas informando que há espaços livres e liberando o mutex
            //Thread recebeu o sinal esperado
            pthread_mutex_unlock(&mutex_controle_depositos_primarios); //Liberando a região crítica novamente
        }
        else if(slots_canetas_disponiveis > 0) {
            printf("ENTROU AQUI!!!\n");
            pthread_mutex_unlock(&mutex_canetas);
            pthread_mutex_lock(&mutex_controle_depositos_primarios);
            controle_depositos_primarios = 1; //Atribuindo o valor de liberação para as threads
            pthread_cond_broadcast(&controle_destrava); //Destravando a fábrica e o depósito de materia prima
            pthread_cond_wait(&dep_canetas_cheio, &mutex_controle_depositos_primarios); //Travando a thread até receber um sinal do depósito de canetas informando que não há mais espaços livres e liberando o mutex
            //Thread recebeu o sinal esperado
            pthread_mutex_unlock(&mutex_controle_depositos_primarios); //Liberando a região crítica novamente

        }
        else {
            //Um local do código que não deve entrar
            printf("ERRO!!! slots de canetas"); 
        }
       
    }

    pthread_exit(NULL);
}

void *deposito_canetas(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex_canetas);
        while (canetas_no_deposito >= entrada[4]) {  // Espera se o depósito estiver cheio
            pthread_cond_wait(&dep_canetas_com_espaco, &mutex_canetas);
        }

        pthread_mutex_unlock(&mutex_canetas); // Libera o mutex enquanto espera produção
        pthread_cond_wait(&dep_canetas_cheio, &mutex_canetas); // Espera sinal de nova caneta produzida

        pthread_mutex_lock(&mutex_canetas);
        if (canetas_no_deposito < entrada[4]) {
            canetas_no_deposito++; // Recebe a caneta produzida
            printf("Caneta adicionada ao depósito. Total no depósito: %d\n", canetas_no_deposito);
        }
        pthread_cond_signal(&dep_canetas_cheio); // Sinaliza que há espaço para mais produção se necessário
        pthread_mutex_unlock(&mutex_canetas);
    }

    pthread_exit(NULL);
}

void *comprador(void *arg) {
    int tempo_entre_compras = entrada[6]; // tempo em segundos entre compras.

    while (1) {
        pthread_mutex_lock(&mutex_canetas);
        while (canetas_no_deposito < entrada[5]) {  // Aguarda ter canetas suficientes para comprar
            printf("Aguardando canetas suficientes para comprar. Canetas disponíveis: %d\n", canetas_no_deposito);
            pthread_cond_wait(&dep_canetas_cheio, &mutex_canetas);
        }

        canetas_no_deposito -= entrada[5];  // Realiza a compra
        printf("Canetas compradas. Canetas restantes no depósito: %d\n", canetas_no_deposito);

        if (canetas_no_deposito < entrada[4]) {  // se o depósito não estiver cheio
            pthread_cond_signal(&dep_canetas_com_espaco);  // notifica que ha espaço para mais canetas
        }
        pthread_mutex_unlock(&mutex_canetas);

        sleep(tempo_entre_compras);  // Espera pelo próximo ciclo de compra baseado na variável explicitamente definida
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        printf("Uso: %s <matéria prima> <unidades enviadas> <tempo entre envios> <tempo para fabricar> <capacidade de canetas> <canetas compradas> <tempo entre compras>\n", argv[0]);
        return 1;
    }

    //Lendo a entrada passada pelo makefile e armazenando-a no vetor global, fazendo a separação de argumentos de acordo  
    for (int i = 1; i < 8; i++) {
        entrada[i-1] = atoi(argv[i]);
    }

    slots_canetas_disponiveis = entrada[4]; //Recebe a quantidade máxima de canetas que podem ser armazenadas

    // Criação de threads
    if(pthread_create(&threads[0], NULL, criador, NULL)) {
        printf("ERRO -- pthread_create\n");
        return 0;
    }


    pthread_exit(NULL);
    return 0;
}