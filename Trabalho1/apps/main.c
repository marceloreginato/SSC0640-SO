#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 6

int entrada[7]; //Vai armazenar os argumentos passados na entrada. Não precisa de semáforo relacionado a esta variável, uma vez que só será para leitura 
pthread_t threads[NUM_THREADS]; //Declarando as threads 

//Semáforos e variáveis de condição de iniciação das threads para melhor sincronização 
pthread_cond_t inicia_dep_materia_prima, inicia_cel_fabricacao, inicia_dep_canetas, inicia_comprador; 
pthread_mutex_t mutex_matprima, mutex_cel_fab, mutex_depcaneta, mutex_comprador; 
int DEP_MAPRIMA = 0; 
int CEL_FAB = 0;    //Essas quatro variáveis indicam se a sua respectiva thread foi escalonada, se sim, assume valor 1
int DEP_CAN = 0;
int COMPRADOR = 0;

//Variáveis de funções
int materia_prima_disponivel = 0; //Matéria prima disponível na célula de fabricação 
int canetas_no_deposito = 0; //Quantidade de canetas no depósito de canetas
int slots_canetas_disponiveis = 0; //Capacidade máxima de canetas a serem guardadas
int canetas_compradas = 0; //Quantidade de canetas que o comprador recebe
int canetas_solicitadas = 0; //Quantidade de canetas solicitadas
int sinal_deposito_comprador = 1; //Sinal que o depósito envia para o comprador, informando que não será possível atender a demanda devido ao estoque zerado (Assume valor -1). 
int sinal_comprador_controle = 1; //Sinal que o comprador envia para o criador. Assume valor -1 se não recebeu nem uma caneta após solicitação 

//Variáveis e semáforos de indicação de suicídio
pthread_mutex_t mutex_morte_matprima; 
int morte_dep_materia_prima = 0;


//Declaração dos semáforos
pthread_mutex_t mutex_materia_prima; //Semáforo para o controle da matéria prima
pthread_mutex_t mutex_canetas; //Semáforo para o controle das canetas ou váriaveis que se relacionam indiretamente
pthread_mutex_t mutex_controle_depositos_primarios; //Semáforo para o controle do depósito de matéria prima e da célula de fabricação
int controle_depositos_primarios = 0; //Variável de controle para os depósitos, inicia bloqueando até que o controle desbloqueie 
pthread_mutex_t mutex_sinal_dep_comprador; //Semáforo para o controle do sinal que o depósito envia para o comprador 
pthread_mutex_t mutex_sinal_comprador_criador; //Semáforo para o controle do sinal que o comprador envia para o criador

//Variáveis de condição
pthread_cond_t dep_materiaprima_vazios;
pthread_cond_t dep_canetas_cheio, dep_canetas_com_espaco;
pthread_cond_t controle_destrava;
pthread_cond_t pedido_solicitado; 


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
    // Criando threads e verificando se não houve erro em suas criações
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
    //Indicando o escalonamento para o controle 
    pthread_mutex_lock(&mutex_matprima);
    DEP_MAPRIMA = 1; //Sinal para o controle que esta thread foi escalonada
    pthread_cond_wait(&inicia_dep_materia_prima, &mutex_matprima); //Esperando o controle liberar esta thread
    pthread_mutex_unlock(&mutex_matprima);

    //Sincronizando a thread célula de fabricação
    while(1) {
        pthread_mutex_lock(&mutex_cel_fab);
        if(CEL_FAB == 1) { //Testando se a thread célula de fabricação foi escalonada
            pthread_mutex_unlock(&mutex_cel_fab);
            pthread_cond_signal(&inicia_cel_fabricacao); //Liberando a célula de fabricação
            break; //Saindo do while
        }
        else {
            pthread_mutex_unlock(&mutex_cel_fab);
        }
    }

    int quantidade_materia_prima = entrada[0]; //quantidade de matéria prima no depósito
    int quant_materia_prima_a_ser_enviada = entrada[1]; //quantidade de matéria prima que deve ser enviada a cada iteração
    int tempo_de_envio = entrada[2]; //tempo de envio de cada iteração

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
                pthread_mutex_lock(&mutex_morte_matprima);
                morte_dep_materia_prima = 1; //Indicando para o criador que essa thread pode ser morta 
                pthread_mutex_unlock(&mutex_morte_matprima);
                
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
    //Indicando o escalonamento para o depósito de matéria prima
    pthread_mutex_lock(&mutex_cel_fab);
    CEL_FAB = 1; //Atribuindo o sinal de escalonamento
    pthread_cond_wait(&inicia_cel_fabricacao, &mutex_cel_fab); //Esperando o sinal do depósito de matéria prima para a liberação desta thread
    pthread_mutex_unlock(&mutex_cel_fab);

    //Sincronizando a thread depósito de canetas
    while(1) {
        pthread_mutex_lock(&mutex_depcaneta);
        if(DEP_CAN == 1) { //Testando se a thread depósito de canetas foi escalonada
            pthread_mutex_unlock(&mutex_depcaneta);
            pthread_cond_signal(&inicia_dep_canetas); //Liberando o depósito de canetas
            break; //Saindo do while
        }
        else {
            pthread_mutex_unlock(&mutex_depcaneta);
        }
    }

    int canetas_fabricadas = 0;
    int tempo_fabricacao_caneta = entrada[3];

    while (1) {
        pthread_mutex_lock(&mutex_controle_depositos_primarios);
        if(controle_depositos_primarios != 0) { //Testando uma região crítica
            pthread_mutex_unlock(&mutex_controle_depositos_primarios);
            
            pthread_mutex_lock(&mutex_materia_prima); //Entrando na região crítica
            if(materia_prima_disponivel > 0) {
                printf("Temos matéria prima disponível na célula de fabricação\n");
                materia_prima_disponivel--; //A célula de fabricação está pegando uma unidade de matéria prima
                pthread_mutex_unlock(&mutex_materia_prima); //Liberando a região crítica
            }
            else {
                //Não temos matéria prima disponível
                printf("Acabou a materia prima disponivel na célula de fabricação\n");
                pthread_cond_wait(&dep_materiaprima_vazios, &mutex_materia_prima); //Espera até que haja matéria prima disponível
                printf("Temos matéria prima disponível novamente na célula de fabricação\n");
                //Após o sinal de matéria prima chegar, então podemos retirar uma unidade
                materia_prima_disponivel--; 
                pthread_mutex_unlock(&mutex_materia_prima);
            }

            //Após pegar a matéria prima, fabricaremos uma caneta
            //Fabricando uma caneta
            printf("Fabricando uma caneta...\n");
            sleep(tempo_fabricacao_caneta);
            

            //Após a fabricação, enviamos a caneta para o depósito de canetas
            pthread_mutex_lock(&mutex_canetas); //Região crítica para o depósito de canetas
            printf("Caneta fabricada e sendo enviada para o depósito\n");
            canetas_no_deposito++; //Enviamos a caneta para o depósito
            slots_canetas_disponiveis--; //Atualizamos a quantidade de slots disponíveis na quantidade de caneta

            pthread_mutex_lock(&mutex_sinal_dep_comprador);
            sinal_deposito_comprador = 1; //Atribuindo o sinal de demanda não será atendida por falta de estoque (estoque zerado)
            pthread_mutex_unlock(&mutex_sinal_dep_comprador);

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
    //Sincronizando a iniciação da thread departamento de matéria prima
    while(1){
        pthread_mutex_lock(&mutex_matprima);
        if(DEP_MAPRIMA == 1) { //significa que a thread depósito de matéria prima foi criada
            pthread_mutex_unlock(&mutex_matprima); //Saindo da região crítica
            pthread_cond_signal(&inicia_dep_materia_prima); //Liberando a thread departamento de matéria prima
            break; //Saindo do while
        } 
        else {
            //A thread departamento de matéria prima ainda não foi escalonada
            pthread_mutex_unlock(&mutex_matprima);
        }
    }
    
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
    //Indicando o escalonamento para a célula de fabricação
    pthread_mutex_lock(&mutex_depcaneta);
    DEP_CAN = 1; //Atribuindo o sinal de escalonamento
    pthread_cond_wait(&inicia_dep_canetas, &mutex_depcaneta); //Esperando o sinal da célula de fabricação para a liberação desta thread
    pthread_mutex_unlock(&mutex_depcaneta);

    //Sincronizando a thread comprador
    while(1) {
        pthread_mutex_lock(&mutex_comprador);
        if(COMPRADOR == 1) { //Testando se a thread comprador foi escalonada
            pthread_mutex_unlock(&mutex_comprador);
            pthread_cond_signal(&inicia_comprador); //Liberando o comprador
            break; //Saindo do while
        }
        else {
            pthread_mutex_unlock(&mutex_comprador);
        }
    }

    while (1) {
        pthread_mutex_lock(&mutex_canetas);
        if(slots_canetas_disponiveis == 0) { //Depósito de canetas cheio
            pthread_mutex_unlock(&mutex_canetas);
            pthread_cond_signal(&dep_canetas_cheio); //Mandando um sinal para o controle informando que o depósito de canetas lotou
        }
        else { //Depósito de canetas ainda possui espaços livres
            pthread_mutex_unlock(&mutex_canetas);
            pthread_cond_signal(&dep_canetas_com_espaco); //Mandando um sinal para o controle informando que o depósito de canetas possui espaços livres
        }

        //Após as verificações para o controle, vamos enviar as compras de canetas realizadas pelo comprador
        pthread_mutex_lock(&mutex_canetas);
        pthread_cond_wait(&pedido_solicitado, &mutex_canetas);
        if(canetas_no_deposito >= canetas_solicitadas) { //Então tem caneta suficiente no depósito para enviar para o comprador
            //Vamos enviar o que o comprador solicitou
            canetas_compradas += canetas_solicitadas; //Canetas compradas é o que tinha antes mais o que foi solicitado
            canetas_no_deposito -= canetas_solicitadas; //A quantidade de canetas no depósito diminui na quantidade que foi entre, isto é, o que foi solicitado
            slots_canetas_disponiveis += canetas_solicitadas; //A quantidade de slots disponíveis no depósito aumenta na quantidade que foi enviada
            canetas_solicitadas = 0; //Atualizamos a quantidade de canetas solicitadas para 0 até que haja uma nova iteração
            pthread_mutex_unlock(&mutex_canetas); //Liberamos a região crítica
            printf("Compra feita com sucesso!\n");
        }
        else if(canetas_no_deposito < canetas_solicitadas && canetas_no_deposito > 0) { //A quantidade solicitada é maior do que existe em estoque, mas o estoque ainda existe
            //Vamos enviar tudo o que temos
            canetas_compradas += canetas_no_deposito; //Enviando tudo o que existe no depósito
            slots_canetas_disponiveis += canetas_no_deposito; //Atualizando os slots disponíveis no depósito, o qual é aumentado na quantidade de canetas que tinham no depósito, haja vista que enviamos tudo
            canetas_solicitadas -= canetas_no_deposito; //Como não enviamos o pedido completo, as canetas solicitadas é o que faltou enviar 
            canetas_no_deposito = 0; //Como foi enviado tudo o que o depósito tinha em estoque, o estoque foi zerado
            pthread_mutex_unlock(&mutex_canetas); //Liberamos a região crítica
            printf("Compra feita malo meno!\n");
        }
        else if(canetas_no_deposito == 0) { //A quantidade solicitada não será atendida nem em uma unidade
            pthread_mutex_unlock(&mutex_canetas);
            pthread_mutex_lock(&mutex_sinal_dep_comprador);
            sinal_deposito_comprador = -1; //Atribuindo o sinal de demanda não será atendida por falta de estoque (estoque zerado)
            pthread_mutex_unlock(&mutex_sinal_dep_comprador);
            //COMO APENAS O CRIADOR POSSUI PODER DE PRINTAR, VAMOS COLOCAR VARIÁVEIS DE NÚMEROS INDICANDO AS RESPOSTAS
            printf("Infelizmente seu pedido não poderá ser atendido, haja a falta de estoque\n"); 
        }
        

    }

    pthread_exit(NULL);
}

void *comprador(void *arg) {
    //Indicando o escalonamento para o depósito de canetas
    pthread_mutex_lock(&mutex_comprador);
    COMPRADOR = 1; //Atribuindo o sinal de escalonamento
    pthread_cond_wait(&inicia_comprador, &mutex_comprador); //Esperando o sinal do depósito de canetas para a liberação desta thread
    pthread_mutex_unlock(&mutex_comprador);

    while (1) { 
        int tempo_solicitacao_canetas = entrada[6];

        //Primeiro solicitamos as canetas
        pthread_mutex_lock(&mutex_canetas);
        canetas_solicitadas = entrada[5]; //Solicitando um valor de canetas
        pthread_cond_signal(&pedido_solicitado);
        printf("Canetas solicitadas...\n");
        pthread_mutex_unlock(&mutex_canetas);

        //Depois testamos se o depósito está enviando unidades de canetas
        pthread_mutex_lock(&mutex_sinal_dep_comprador);
        if(sinal_deposito_comprador == -1) { //Significa que não foi atendido o pedido em nem uma única unidade de caneta
            pthread_mutex_unlock(&mutex_sinal_dep_comprador);
            printf("COMPRADOR: O DEPÓSITO NAO TINHA ESTOQUE\n");
            pthread_mutex_lock(&mutex_sinal_comprador_criador);
            sinal_comprador_controle = -1; //Passando o sinal para o controle informando que não recebeu, em nem uma unidade, a solicitação de caneta
            pthread_mutex_unlock(&mutex_sinal_comprador_criador);
        }
        else {
            pthread_mutex_unlock(&mutex_sinal_dep_comprador);
        }
        sleep(tempo_solicitacao_canetas);

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