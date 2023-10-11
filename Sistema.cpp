#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <time.h>
#include <windows.h>
#include <math.h>
#include <conio.h>

#include "funcaoSecretaNaoOlha.h"
#include "I-NodeTAD.h"

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
            // printf("\n");
            if (comando.size() >= 5 && strcmp(comando.substr(3).c_str(), "-li") == 0){
                listaLinkDiretorioAtual(disco, enderecoInodeAtual);
            }
            else if (comando.size() >= 5 && strcmp(comando.substr(3, 2).c_str(), "-l") == 0)
            {
                listarDiretorioComAtributos(disco, enderecoInodeAtual, strcmp(comando.substr(3).c_str(), "-la") == 0);
            }
            else if (comando.size() >= 5 && strcmp(comando.substr(3).c_str(), "-e") == 0){
                listaDiretorioAtualIgualExplorer(disco, enderecoInodeAtual);
            }
            else
            {
                listarDiretorio(disco, enderecoInodeAtual, comando.size() >= 5 && strcmp(comando.substr(3).c_str(), "-a") == 0);
            }

            printf("\n");
        }
        else if (strcmp(comando.substr(0, 5).c_str(), "mkdir") == 0)
        {
            char nomeDiretorio[MAX_NOME_ARQUIVO];
            
            if (comando.size() >= 6)
            {
                strcpy(nomeDiretorio, comando.substr(6).c_str()); 
                
                if (isEnderecoValido(mkdir(disco, enderecoInodeAtual, enderecoInodeRaiz, nomeDiretorio)))
                {
                    printf("Diretorio criado\n");
                }
                else
                {
                    textcolor(RED);
                    printf("Nao foi possivel criar o diretorio\n");
                    textcolor(WHITE);
                }
            }
            // char nomeDiretorio1[MAX_NOME_ARQUIVO];
            // strcpy(nomeDiretorio1, "teste");
            // for(int i=0; i < 140; i++){
            //     itoa(i, nomeDiretorio, 10);
            //     strcpy(nomeDiretorio, strcat(nomeDiretorio1, nomeDiretorio));                
            //     addDiretorioEArquivo(disco, 'd', enderecoInodeAtual, nomeDiretorio);
            //     nomeDiretorio[0] = '\0';
            //     strcpy(nomeDiretorio1, "teste");
            // }
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
            char touchString[300];
            strcpy(touchString, comando.substr(6).c_str());

            if (comando.size() > 6)
                if(touch(disco, enderecoInodeAtual, enderecoInodeRaiz, touchString))
                    printf("Arquivo criado\n");
                else {
                    textcolor(RED);
                    printf("Nao foi possivel criar o arquivo\n");
                    textcolor(WHITE);
                }
                    
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
        else if (strcmp(comando.substr(0, 2).c_str(), "vi") == 0)
        {
            if (comando.size() >= 3)
            {
                string enderecosUtilizados;
                enderecosUtilizados.assign("");

                vi(disco, enderecoInodeAtual, comando.substr(3), enderecosUtilizados);
            }
        }
        else if (strcmp(comando.substr(0, 5).c_str(), "rmdir") == 0)
        {
            if (comando.size() >= 6)
            {
                int contadorDiretorio = 0;
                if(rmdir(disco, enderecoInodeAtual, comando.substr(6), contadorDiretorio))
                    printf("Diretorio removido\n");
                else {
                    textcolor(RED);
                    printf("Nao foi possivel remover o diretorio\n");
                    textcolor(WHITE);
                }
            }
        }
        else if (strcmp(comando.substr(0, 2).c_str(), "rm") == 0)
        {
            if (comando.size() >= 3)
            {
                if(rm(disco, enderecoInodeAtual, comando.substr(3)))
                    printf("Arquivo removido\n");
                else {
                    textcolor(RED);
                    printf("Nao foi possivel remover o arquivo\n");
                    textcolor(WHITE);
                }
            }
        }  
        else if (strcmp(comando.substr(0, 4).c_str(), "link") == 0)
        {
            if (comando.size() >= 6)
            {
                if(strcmp(comando.substr(5, 2).c_str(), "-s") == 0)
                    linkSimbolico(disco, enderecoInodeAtual, comando.substr(8), enderecoInodeRaiz);
                else if(strcmp(comando.substr(5, 2).c_str(), "-h") == 0)
                    linkFisico(disco, enderecoInodeAtual, comando.substr(8), enderecoInodeRaiz);
            }
        }   
        else if (strcmp(comando.substr(0, 6).c_str(), "unlink") == 0)
        {
            if (comando.size() >= 6)
            {
                if(strcmp(comando.substr(7, 2).c_str(), "-s") == 0)
                    unlinkSimbolico(disco, enderecoInodeAtual, comando.substr(10), enderecoInodeRaiz);
                else if(strcmp(comando.substr(7, 2).c_str(), "-h") == 0)
                    unlinkFisico(disco, enderecoInodeAtual, comando.substr(10), enderecoInodeRaiz);
            }
        }
        else if (strcmp(comando.substr(0, 5).c_str(), "chmod") == 0)
        {
            if (comando.size() >= 7)
            {
                chmod(disco, enderecoInodeAtual, comando.substr(6));
                // if(strcmp(comando.substr(6, 2).c_str(), "-s") == 0)
                //     unlinkSimbolico(disco, enderecoInodeAtual, comando.substr(10), enderecoInodeRaiz);
                // else if(strcmp(comando.substr(6, 2).c_str(), "-h") == 0)
                //     unlinkFisico(disco, enderecoInodeAtual, comando.substr(10), enderecoInodeRaiz);
            }
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
            if (comando.size() >= 4)
            {
                endereco = atoi(comando.substr(4).c_str());
                if (endereco >= 0 && endereco < quantidadeBlocosTotais)
                {
                    disco[endereco].bad = 1;
                }else{
                    textcolor(RED);
                    printf("endereco invalido.\n");
                    textcolor(WHITE);
                }
            }
        }
        else if (strcmp(comando.substr(0, 8).c_str(), "max file") == 0)
        {
            int quantidadeBlocosDisponiveis = getQuantidadeBlocosLivres(disco);
            int quantidadeUtilizadas = 0;
            int quantidadeRealUtilizada = 0;
            getQuantidadeBlocosMaiorArquivo(disco, quantidadeBlocosDisponiveis, quantidadeUtilizadas, quantidadeRealUtilizada);

            printf("Pode ser usado %d de %d blocos para inserir um arquivo(%.2f%% aproveitamento)\n", quantidadeRealUtilizada, quantidadeUtilizadas, (float) quantidadeRealUtilizada/ (float)quantidadeUtilizadas*100);
        }
        else if (strcmp(comando.substr(0, 10).c_str(), "lost block") == 0)
        {   
            int blocosPerdidos = quantidadeBlocosTotais / QUANTIDADE_LIMITE_ENDERECO_LISTA_BLOCO_LIVRE;
            int blocosPerdidosBytes = getQuantidadeBlocosPerdidos(quantidadeBlocosTotais);            

            printf("Foram perdidos %d (%d em Bytes) de %d blocos (%.2f%% de perca)\n", blocosPerdidos, blocosPerdidosBytes, quantidadeBlocosTotais, (float) blocosPerdidos/ (float)quantidadeBlocosTotais*100);
        }
        else if (strcmp(comando.substr(0, 11).c_str(), "check files") == 0)
        {   
            buscaBlocosIntegrosCorrompidos(disco, enderecoInodeAtual);
            printf("\n");
        }

        textcolor(GREEN);
        printf("root@localhost");
        textcolor(WHITE);
        printf(":");
        textcolor(LIGHTBLUE);
        printf("%s", caminhoAbsoluto.c_str());
        textcolor(WHITE);
        printf("$ ");

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
    
    iniciarAParada();
    getch();
    
    system("cls");
    
    quantidadeBlocosTotais = QuantidadeBlocosTotais();
    
    printf("Quantidade de blocos selecionada: %d",quantidadeBlocosTotais);
    
    Disco disco[quantidadeBlocosTotais];
    inicializaSistema(disco, quantidadeBlocosTotais);
    
    return 0;
}
