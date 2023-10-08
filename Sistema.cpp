#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <time.h>
#include <windows.h>
#include <math.h>
#include <conio.h>

#include "I-NodeTAD.h"
#include "funcaoSecretaNaoOlha.h"

using namespace std;

void inicializaSistemaBlocos(Disco disco[], int quantidadeBlocosTotais){
    int quantidadeBlocosNecessariosListaLivre = quantidadeBlocosTotais / QUANTIDADE_LIMITE_ENDERECO_LISTA_BLOCO_LIVRE;
    
    for(int j=0; j<quantidadeBlocosNecessariosListaLivre; j++)
    {
        initDisco(disco[j]);
        initListaBlocosLivre(disco[j].lbl);
        
    }

    for(int i = quantidadeBlocosNecessariosListaLivre; i < quantidadeBlocosTotais; i++) 
    {
        initDisco(disco[i]);
        pushListaBlocoLivre(disco, i);
    }
}

void execucaoSistema(Disco disco[], int quantidadeBlocosTotais, int enderecoInodeRaiz){
    string caminhoAbsoluto;
    int endereco;
    string comando;

    comando.append("init");
    
    fflush(stdin);

    int enderecoInodeAtual = enderecoInodeRaiz;

    caminhoAbsoluto.append("~");

    printf("\n");
    do
    {		
        if (strcmp(comando.substr(0, 2).c_str(), "ls") == 0)
        {
            printf("\n");
            if (comando.size() >= 5 && strcmp(comando.substr(3).c_str(), "-l") == 0)
            {
                listarDiretorioComAtributos(disco, enderecoInodeAtual);
            }
            else
            {
                listarDiretorio(disco, enderecoInodeAtual);
            }

            printf("\n");
        }
        else if (strcmp(comando.substr(0, 5).c_str(), "mkdir") == 0)
        {
            char nomeDiretorio[MAX_NOME_ARQUIVO], nomeDiretorio1[MAX_NOME_ARQUIVO];
            strcpy(nomeDiretorio1, "teste");
            // strcpy(nomeDiretorio, comando.substr(6).c_str());            
            // addDiretorioEArquivo(disco, 'd', enderecoInodeAtual, nomeDiretorio);
            for(int i=0; i < 140; i++){
                itoa(i, nomeDiretorio, 10);
                strcpy(nomeDiretorio, strcat(nomeDiretorio1, nomeDiretorio));
                
                addDiretorioEArquivo(disco, 'd', enderecoInodeAtual, nomeDiretorio);
                nomeDiretorio[0] = '\0';
                strcpy(nomeDiretorio1, "teste");
            }
        
//        	printf("\n\n");                        
            printf("Diretorio Criado: \n\n");                        

//            for(int i=0; i < disco[enderecoInodeAtual].diretorio.TL; i++)
//            {
//                printf(" - %s\n", disco[enderecoInodeAtual].diretorio.arquivo[i]);
//            }
        }
        else if (strcmp(comando.substr(0, 2).c_str(), "cd") == 0)
        {
            if (comando.size() >= 3)
            {
                enderecoInodeAtual = cd(disco, enderecoInodeAtual, comando.substr(3), enderecoInodeRaiz, caminhoAbsoluto);
            }
        }
        else if (strcmp(comando.substr(0, 5).c_str(), "touch") == 0)
        {
            if (comando.size() > 6)
                touch(disco, enderecoInodeAtual, comando.substr(6));
        }
        else if(strcmp(comando.substr(0, 2).c_str(), "df") == 0)
        {
            int qtdBlocosLivres=0, qtdBlocosOcupados=0;
            float porcentagemBlocosUsados;
            buscaBlocosLivresOcupados(disco, qtdBlocosLivres, qtdBlocosOcupados, quantidadeBlocosTotais, quantidadeBlocosTotais / QUANTIDADE_LIMITE_ENDERECO_LISTA_BLOCO_LIVRE);

            porcentagemBlocosUsados = (float) qtdBlocosOcupados/quantidadeBlocosTotais;
            printf("Filesystem\tTamanho\tUsados\tDisponivel\tUso%\t\tMontado em\n");
            printf("/dev/sda1\t%d\t%d\t%d\t\t%.2f%%\t\t/\n", quantidadeBlocosTotais, qtdBlocosOcupados,qtdBlocosLivres, porcentagemBlocosUsados*100);
        }
        else if (strcmp(comando.c_str(), "trace disk") == 0)
        {
            printf("\n");
            exibirDisco(disco, quantidadeBlocosTotais, quantidadeBlocosTotais / QUANTIDADE_LIMITE_ENDERECO_LISTA_BLOCO_LIVRE);
            printf("\n");
        }
        else if (strcmp(comando.c_str(), "clear") == 0)
        {
            system("cls");
        }
        else if (strcmp(comando.substr(0, 3).c_str(), "bad") == 0)
        {            
            endereco = atoi(comando.substr(4).c_str());
            if (endereco >= 0 && endereco < quantidadeBlocosTotais)
            {
                disco[endereco].bad = 1;
            }else{
                printf("endereco invalido.\n");
            }
        }
        
        printf("root@localhost:%s$ ", caminhoAbsoluto.c_str());

		getline(cin, comando);
    } while(strcmp(comando.c_str(), "exit") != 0);
}

void inicializaSistema(Disco disco[], int quantidadeBlocosTotais)
{
    inicializaSistemaBlocos(disco, quantidadeBlocosTotais);
    int enderecoInodeRaiz = criaDiretorioRaiz(disco);
    execucaoSistema(disco, quantidadeBlocosTotais, enderecoInodeRaiz);
}

int QuantidadeBlocosTotais() {

    string charQuantidadeBlocosTotais;
	int quantidadeBlocosTotais;
    bool flag = 1;

    printf("Informe a Quantidade de blocos no disco desejada[ENTER p/ ignorar]:\n");
    while(flag) {

        fflush(stdin);
        getline(cin, charQuantidadeBlocosTotais);

        flag = 0;

        if(strcmp(charQuantidadeBlocosTotais.c_str(),"") == 0) {
			quantidadeBlocosTotais = 1000;
		} 
		else {
			quantidadeBlocosTotais = atoi(charQuantidadeBlocosTotais.c_str()); 
			if(quantidadeBlocosTotais <= 10) {
				printf("A quantidade deve ser maior que 10!\n");
                flag = 1;
			}
		}
    }
	return quantidadeBlocosTotais;
}

int main()
{
    int quantidadeBlocosTotais;
    /*no início do sistema, deve ser informado pelo usuário a quantidade total de discos que haverá */
    /*deve ser possível executar o comando ls -l*/
    
    // iniciarAParada();
    // getch();
    
    system("cls");
    
    quantidadeBlocosTotais = QuantidadeBlocosTotais();
    
    printf("Quantidade de blocos selecionada: %d",quantidadeBlocosTotais);
    
    Disco disco[quantidadeBlocosTotais];
    inicializaSistema(disco, quantidadeBlocosTotais);
    
    return 0;
}
