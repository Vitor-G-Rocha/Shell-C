/*Vitor Gomes Rocha
SHELL */
//Ultima mudança: 06.07.2017
//Bibliotecas/////////
#include <stdio.h>    
#include <stdlib.h>   
#include <unistd.h>   
#include <string.h>   
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>//<--redirecionamento 
/////////////////////
//Prototipo das funcoes
void chama_pipe();
void chama_terminal();
void le_comando();
void trata_sinal();

//Trata Sinal
void trata_sinal(){
  waitpid(-1,NULL,WNOHANG);
}

void le_comando() {
    //Variaveis
    int count = 0, i = 0, j = 0, counter2 = 0;
    int status = 0, background = 0, pipe_ver = 0; 
    char *args[200], *par[200], *pch, line[1024];	     
    pid_t pid;    
    
    //Estrutura para tratamento de Sinal          
    struct sigaction sinal_tratamento;
    sinal_tratamento.sa_handler = trata_sinal;
    sigemptyset(&sinal_tratamento.sa_mask);
    sinal_tratamento.sa_flags = 0;
         
    //Variavies redirecionamento   
    int fdIn, fdOut;  
    //Limpando vetores, evitar problemas com lixo
    memset(&args[0], 0, sizeof(args));
    memset(&line[0], 0, sizeof(line));
    //Leitura
	for ( ;; ) {        
		int c = fgetc(stdin); //Le caracter por caracter
		line[count++] = (char) c; //Armazena em line os caracteres
		if(c == '\n') {//Quando 'Enter' é pressionado, break;			
            break;	
        }
        //printf("Debug: 1\n");
        //printf("\nLine[%d]: %s", count, line);
	}	
	//Parse das linhas
	pch = strtok(line, " \n"); //"Jogando" os caracteres de Line para pch, \n é o delimitador
    //printf("Debug: 2\n");
	//printf("\nPCH: %s\n", pch);
	while (pch != NULL) {//Duplicando pch para o vetor args, e colocando NULL em pch
		args[i++] = strdup(pch);
		pch = strtok(NULL, " \n");
	}		
	//Vetor par recebe os parametros
	for ( j = 1; j < i; j++ ) {
		par[j] = args[j];
        //printf("Debug: 3\n");
        //printf("\nPar[%c]: %s\n", j, par);
	}	
	par[i] = NULL; //Nulo para indicar o fim da lista de parametros	
	//Verificando se irá rodar em background
	if(strcmp(args[j-1], "&") == 0) {
		background = 1;
		args[j-1] = NULL;
		j -= 1;
	}    
	//printf("Debug: 4\n");
	//printf("\nBackground: %i\n", background)
	if((pid = fork()) == -1) {//Processo menor que zero == erro
		perror("\nErro FORK!!!\n");
    } else if (pid == 0) {//Processo filho == 0            
        //Verificando Pipes
        int pcount = 0;        
        while (args[pcount] != NULL){
            if (strcmp(args[pcount],"|") == 0){            
                pipe_ver = 1;
                chama_pipe(args);                
            }
            pcount++;
        }     
        //printf("Debug: 5\n");
        //printf("\nPipe_ver: %i\n", pipe_ver);
        //Verificando redirecionamento...
        while(args[counter2] != NULL) {
            if(strcmp(args[counter2], ">") == 0) {
				if(args[counter2+1] != NULL) {//Faz o redirecionamento do shell para um arquivo
					fdOut = open(args[counter2+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);//Guarda o descritor de arquivo em fdOut -- 0666 = Dap ermisão para todos de "Read" "Write", mas nao podem executar
					//printf("Debug: 6.2 - Redir\n");
					//printf("\nFdOut: %i", fdOut);
					dup2(fdOut, STDOUT_FILENO);//Duplica a saida, usando o descritor fdOut
			} else {//Imprime se nenhum arquivo for informado //ESTA BUGADO TENTAR ARRUMAR
                    printf("Nenhum arquivo de saida foi informado!");
              }
                args[counter2] = 0;
              }
       counter2++;
       }//Fim redirecionamento 
       //printf("Debug: 7 - Pipe\n");
       //printf("\nPipe_ver: %i\n", pipe_ver);       
       if(pipe_ver == 0) {//Se nao achou um Pipe, executa
			execvp(*args,args);	
            printf("Comando desconhecido: %s\n", args[0]);//Printa se o comando informado nao existir         
       }     		
    } else { //Processo pai esperando pelo filho      
		if (background == 1 && pipe_ver != 1) { //Se background == True, roda em background
                printf("Rodando {%s} em background PID:[%d]\n",args[0], pid);
                sigaction(SIGCHLD, &sinal_tratamento, NULL);
		} else {     
                waitpid(pid, &status, 0);//Espera resposta do processo filho pid
		}        
    }	
}

void chama_pipe(char * args[]){
	// Descritores de arquivo fd, e contador de comandos num_cmds
	int fd[2], fd2[2], num_cmds = 0, fim = 0;// 1 para entrada do pipe, 0 para saida do pipe	
	char *comando[256];	
	pid_t pid; //PID	
    //Contadores
	int i = 0, j = 0, k = 0, l = 0;	
    
	while (args[l] != NULL){//Contando numero de comandos
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
        //printf("Debug-Chama_pipe: 1\n");
        //printf("\nArgs: %s\n", args[l]); 
	}	
	num_cmds++;//Incrementa mais 1, numero de comandos == quantidade de pipes + 1
	// Cada comando entre os pipes sera configurado, e suas saidas/entras padroes vao ser substituidas, e depois elas serao executadas
	while (args[j] != NULL && fim != 1){//Enquanto args for diferente de NULL e fim diferente de 1, loop
		k = 0;		
		while (strcmp(args[j],"|") != 0){//Passando os argumentos para um vetor auxiliar
			comando[k] = args[j];
			j++;	
			if (args[j] == NULL){//Verifica se a posicao e nula, se for, fim recebe 1 para indicar fim do loop
				fim = 1;
				k++;
				break;
			}
			k++;
		}		
		comando[k] = NULL;//Ultima posicao recebe NULL
		j++;		
		// Sera setado descritores diferentes para os pipes, um pipe para as iteracoes impares e um pipe para as interacoes pares
		if (i % 2 != 0){// se i for impar
			pipe(fd); //Cria pipe em fd
		}else{// se i for par
			pipe(fd2); //Cria pipe em fd2
		}	
		//fd2 pra as iteracoes PARES
		//fd pra as iteracoes IMPARERS		
		if((pid = fork()) == 0){ //cria o processo e verifica se e filho
			if (i == 0){ //O primeiro comando esta na primeira iteracao de i
				dup2(fd2[1], STDOUT_FILENO);//escreve em fd2
			}
			//Se for o ultimo comando, sua entrada padrao sera substituida dependendo da iteracao, par ou impar, por ser o ultimo comando a sua saida padrao sera no terminal 
			else if (i == num_cmds - 1){//Verifica se e o ultimo comando
				if (num_cmds % 2 != 0){ // se numero de comandos for impar
					dup2(fd[0],STDIN_FILENO);//dup na leitura(entrada) de fd, para receber a escrita do comando anterior
				}else{ //se numero de comandos for par
					dup2(fd2[0],STDIN_FILENO);//dup na leitura(entrada) de fd2, para receber a escrita do comando anterior
				}
			//Se for o comando do meio, sera usado 2 pipes, 1 para a entrada e outro para a saida			
			}else{ //Comando do meio
				if (i % 2 != 0){ //se i for impar
					dup2(fd2[0],STDIN_FILENO);//le de fd2
					dup2(fd[1],STDOUT_FILENO);//escreve em fd
				}else{ //se i for par
					dup2(fd[0],STDIN_FILENO); //dup na leitura de fd
					dup2(fd2[1],STDOUT_FILENO);	//dup na escrita de fd2				
				} 
			} 
			//printf("Debug-Chama_pipe: Comando\n");
			//printf("\nComando: %s\n", comando[k]);
			execvp(*comando,comando);
		}//fim if pid
				
		//Fechando os descritores na ordem em quer foram abertos
		if (i == 0){
			close(fd2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){					
				close(fd[0]);
			}else{					
				close(fd2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(fd2[0]);
				close(fd[1]);
			}else{					
				close(fd[0]);
				close(fd2[1]);
			}
		}				
		waitpid(pid,NULL,0);				
		i++;	
	}
	kill(getpid(),SIGTERM);//Matar o processo
}

void chama_terminal() {
	static int primeira_vez = 1;
	if ( primeira_vez ) { //Limpa tela na primeira execução do shell
		system("clear");        
		primeira_vez = 0;		
	} 
	printf("#Vitor#>"); 
}

int main () {	   
	while ( 1 ) {		//Repete pra sempre
		chama_terminal();	//Chama a tela do terminal
		le_comando(); //Chama a funcao que vai fazer o parse e executar os comandos      
    }    
	return 0;
}