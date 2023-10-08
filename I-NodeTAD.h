#include "Permissao.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

#define TIPO_ARQUIVO_DIRETORIO 'd'
#define TIPO_ARQUIVO_ARQUIVO '-'
#define TIPO_ARQUIVO_LINK 'l'

#define MAX_NOME_ARQUIVO 14
#define DIRETORIO_LIMITE_ARQUIVOS 10
#define MAX_INODEINDIRETO 8
#define ENDERECO_CABECA_LISTA 0
#define QUANTIDADE_LIMITE_ENDERECO_LISTA_BLOCO_LIVRE 10
#define PERMISSAO_PADRAO_DIRETORIO 755
#define PERMISSAO_PADRAO_ARQUIVO 644

// INODE CONTROLA COMO ESTÁ SENDO GRAVADO O ARQUIVO, EM QUAIS BLOCOS ESTÁ GRAVADO O CONTEÚDO DO ARQUIVO, AS PERMISSÕES, DATA
struct INode
{ // Inode principal
    /*
        DEFINÇÃO DO QUE O I-NODE ESTÁ APONTANDO:
        [0] 'd' = DIRETÓRIO, '-' = ARQUIVO, 'l' = LINK

        PERMISSÕES:
        [1] - [3] (rwx) -> OWNER
        [4] - [6] (rwx) -> GROUP
        [7] - [9] (rwx) -> OTHERS

        [10] - '\0'
    */
    char protecao[11];

    /*Contador do números de entradas no diretório que apontam para o i-node;*/
    int contadorLinkFisico;

    /*Código do proprietário do arquivo*/
    int proprietario;
    /*Código do grupo do arquivo*/
    int grupo;

    /*data de criação do arquivo*/
    char dataCriacao[20];
    /*data de último acesso do arquivo*/
    char dataUltimoAcesso[20];
    /*data de última alteração do arquivo*/
    char dataUltimaAlteracao[20];

    /*Tamanho em bytes do arquivo apontado*/
    long long int tamanhoArquivo;

    /*Endereços de blocos que armazenam o conteúdo do arquivo */
    /*5 primeiros para alocação direta*/
    int enderecoDireto[5];
    /*6° ponteiro para alocação simples indireta*/
    int enderecoSimplesIndireto;
    /*7° para alocação dupla indireta*/
    int enderecoDuploIndireto;
    /*8° para alocação tripla-indireta*/
    int enderecoTriploIndireto;
};
typedef struct INode INode;

/*UTILIZADO COMO INTERMÉDIO PARA APONTAR PARA MAIS ENDEREÇOS*/
struct INodeIndireto
{
    int endereco[MAX_INODEINDIRETO];
    int TL;
};
typedef INodeIndireto INodeIndireto;

struct Arquivo
{
    int enderecoINode;
    char nome[MAX_NOME_ARQUIVO];
};
typedef struct Arquivo Arquivo;

struct Diretorio
{
    Arquivo arquivo[DIRETORIO_LIMITE_ARQUIVOS];
    int TL;
};
typedef struct Diretorio Diretorio;

struct LinkSimbolico
{
    char caminho[100];
};
typedef struct LinkSimbolico LinkSimbolico;

// A PILHA DE DISCOS
struct ListaBlocoLivre
{
    int topo;
    int endereco[QUANTIDADE_LIMITE_ENDERECO_LISTA_BLOCO_LIVRE];
    // endereço do próximo bloco de pilha no disco
    int enderecoBlocoProx;
};
typedef struct ListaBlocoLivre ListaBlocoLivre;

struct Disco
{
    // identificador se o disco está como bad ou não
    int bad;

    INode inode;
    INodeIndireto inodeIndireto;
    Diretorio diretorio;
    ListaBlocoLivre lbl;
    LinkSimbolico ls;
};
typedef struct Disco Disco;

struct exibicaoEndereco
{
    int endereco;
    char info;
};

string getNomeProprietario(int proprietarioCod)
{
    string nome;

    switch (proprietarioCod)
    {
    case 1000:
        nome.assign("root");
        break;

    default:
        nome.assign("nao definido");
    }

    return nome;
}

string getNomeGrupo(int proprietarioCod)
{
    string nome;

    switch (proprietarioCod)
    {
    case 1000:
        nome.assign("root");
        break;
    default:
        nome.assign("nao definido");
    }

    return nome;
}

void initDisco(Disco &disco)
{
    disco.bad = 0;
    disco.diretorio.TL = 0;
    disco.inode.protecao[0] = '\0';
    disco.inodeIndireto.TL = 0;
    disco.lbl.topo = -1;
    disco.ls.caminho[0] = '\0';
}

int criarINode(Disco disco[], char tipoArquivo, char permissao[10], int tamanhoArquivo, int enderecoInodePai);
int addDiretorioEArquivo(Disco disco[], char tipoArquivo, int enderecoInodeDiretorioAtual, char nomeDiretorioArquivoNovo[MAX_NOME_ARQUIVO], int tamanhoArquivo);
void addArquivoNoDiretorio(Disco disco[], int enderecoDiretorio, int enderecoInodeCriado, char nomeDiretorioArquivo[MAX_NOME_ARQUIVO]);

void setDataHoraAtualSistema(char dataAtual[])
{
    time_t t;
    t = time(NULL);
    char data[30];
    strftime(data, sizeof(data), "%d/%m/%Y %H:%M:%S", localtime(&t));
    strcpy(dataAtual, data);
}

int getEnderecoNull()
{
    return -1;
}

void setEnderecoNull(int &endereco)
{
    endereco = getEnderecoNull();
}
char isEnderecoNull(int endereco)
{
    return endereco == getEnderecoNull();
}
char isEnderecoValido(int endereco)
{
    return !isEnderecoNull(endereco);
}
void initListaBlocosLivre(ListaBlocoLivre &lbl)
{
    setEnderecoNull(lbl.topo);
    setEnderecoNull(lbl.enderecoBlocoProx);
}
char isFullListaBlocosLivre(ListaBlocoLivre lbl)
{
    return lbl.topo == QUANTIDADE_LIMITE_ENDERECO_LISTA_BLOCO_LIVRE - 1;
}
char isEmptyListaBlocoLivre(ListaBlocoLivre lbl)
{
    return lbl.topo == getEnderecoNull();
}
char isEnderecoBad(Disco disco[], int endereco)
{
    return disco[endereco].bad;
}
int getQuantidadeBlocosLivres(Disco disco[])
{
    int enderecoProxBloco = ENDERECO_CABECA_LISTA;
    int quantidadeBlocosLivres = -1;

    while (enderecoProxBloco != -1)
    {
        quantidadeBlocosLivres += disco[enderecoProxBloco].lbl.topo;
        enderecoProxBloco = disco[enderecoProxBloco].lbl.enderecoBlocoProx;
    }

    return quantidadeBlocosLivres;
}

void pushListaBlocoLivre(Disco disco[], int endereco)
{
    int enderecoBlocoAtual = ENDERECO_CABECA_LISTA;
    int enderecoProxBloco = disco[ENDERECO_CABECA_LISTA].lbl.enderecoBlocoProx;

    while (isEnderecoValido(enderecoProxBloco)) // significa que a lista contém mais de um bloco, então percorre até o último da lista
    {
        enderecoBlocoAtual = enderecoProxBloco;
        enderecoProxBloco = disco[enderecoProxBloco].lbl.enderecoBlocoProx;
    }

    // o último bloco encontrado da lista não está cheio?
    if (!isFullListaBlocosLivre(disco[enderecoBlocoAtual].lbl))
        disco[enderecoBlocoAtual].lbl.endereco[++disco[enderecoBlocoAtual].lbl.topo] = endereco;
    else
    {
        // caso estiver cheio, adiciona mais um na lista
        disco[enderecoBlocoAtual].lbl.enderecoBlocoProx = enderecoBlocoAtual + 1;

        // adiciona o endereço
        disco[enderecoBlocoAtual + 1].lbl.endereco[++disco[enderecoBlocoAtual + 1].lbl.topo] = endereco;
    }
}

int popListaBlocoLivre(Disco disco[])
{
    int enderecoLivre;
    int enderecoBlocoAtual = ENDERECO_CABECA_LISTA;
    int enderecoProxBloco = disco[enderecoBlocoAtual].lbl.enderecoBlocoProx;

    if (!isEmptyListaBlocoLivre(disco[enderecoBlocoAtual].lbl)) // há elementos na lista
    {
        if (isEnderecoNull(enderecoProxBloco)) // se for igual a -1, significa que não há próximo bloco da lista
        {
            enderecoLivre = disco[ENDERECO_CABECA_LISTA].lbl.endereco[disco[ENDERECO_CABECA_LISTA].lbl.topo--];

            if (isEnderecoBad(disco, enderecoLivre))
                return popListaBlocoLivre(disco);

            return enderecoLivre;
        }
        else
        {
            int enderecoBlocoAnterior = enderecoBlocoAtual;

            while (isEnderecoValido(enderecoProxBloco)) // significa que a lista cont�m mais de um bloco, então percorre at� o último da lista
            {
                enderecoBlocoAnterior = enderecoBlocoAtual;
                enderecoBlocoAtual = enderecoProxBloco;
                enderecoProxBloco = disco[enderecoBlocoAtual].lbl.enderecoBlocoProx;
            }

            enderecoLivre = disco[enderecoBlocoAtual].lbl.endereco[disco[enderecoBlocoAtual].lbl.topo--];

            // caso o bloco atual não tenha mais nenhum item após a remo��o, altera o apontamento do anterior para -1
            if (isEmptyListaBlocoLivre(disco[enderecoBlocoAtual].lbl))
            {
                // bloco anterior para de apontar para o bloco atual, quebrando a lista
                setEnderecoNull(disco[enderecoBlocoAnterior].lbl.enderecoBlocoProx);
            }

            if (isEnderecoBad(disco, enderecoLivre))
                return popListaBlocoLivre(disco);

            return enderecoLivre;
        }
    }

    return getEnderecoNull();
}

void exibeListaBlocoLivre(Disco disco[])
{
    int i = 0, topo;
    printf("\n");
    int enderecoProxBloco = ENDERECO_CABECA_LISTA;

    while (enderecoProxBloco != -1)
    {
        topo = disco[enderecoProxBloco].lbl.topo;
        printf("\n -- Lista de Blocos livres: %d --\n", enderecoProxBloco);

        while (!isEnderecoNull(topo))
        {
            printf(" [ %d ] ", disco[enderecoProxBloco].lbl.endereco[topo--]);
        }

        printf("\n");
        enderecoProxBloco = disco[enderecoProxBloco].lbl.enderecoBlocoProx;
    }
}

void addExibicaoEndereco(exibicaoEndereco enderecos[], int &TL, int endereco, char info)
{
    enderecos[TL].endereco = endereco;
    enderecos[TL++].info = info;
}

int criarINodeIndireto(Disco disco[])
{
    int blocoLivreInodeIndireto = popListaBlocoLivre(disco);
    for (int i = 0; i < MAX_INODEINDIRETO; i++)
    {
        setEnderecoNull(disco[blocoLivreInodeIndireto].inodeIndireto.endereco[i]);
    }

    return blocoLivreInodeIndireto;
}

// utilizado para inserir ao criar o inode
void inserirInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, int enderecoInodePrincipal, int &quantidadeBlocosNecessarios, int InseridoPeloTriplo = 0)
{
    int quantidadeBlocosUtilizados = 0;
    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO - InseridoPeloTriplo)
    {
        int inicio = disco[enderecoInodeIndireto].inodeIndireto.TL;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO - InseridoPeloTriplo)
        {
            disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = popListaBlocoLivre(disco);
            disco[enderecoInodeIndireto].inodeIndireto.TL++;            
            quantidadeBlocosUtilizados++;
            inicio++;
        }
    }

    // gera a quantidade ainda faltante.
    quantidadeBlocosNecessarios = quantidadeBlocosNecessarios - quantidadeBlocosUtilizados;

    if (InseridoPeloTriplo && quantidadeBlocosNecessarios > 0)
    {
        char permissao[10];
        for (int i = 0; i < 10; i++)
        {
            disco[enderecoInodePrincipal].inode.protecao[i];
        }

        strncpy(permissao, disco[enderecoInodePrincipal].inode.protecao + 1, 10);

        disco[enderecoInodeIndireto].inodeIndireto.endereco[disco[enderecoInodeIndireto].inodeIndireto.TL] = criarINode(disco,
                                                                                                                        disco[enderecoInodePrincipal].inode.protecao[0],
                                                                                                                        permissao,
                                                                                                                        quantidadeBlocosNecessarios * 10,
                                                                                                                        enderecoInodePrincipal);

        // conseguiu criar o inode
        if (isEnderecoValido(disco[enderecoInodeIndireto].inodeIndireto.endereco[disco[enderecoInodeIndireto].inodeIndireto.TL]))
        {
            disco[enderecoInodeIndireto].inodeIndireto.TL++;
        }
    }
}

// utilizado para inserir ao criar o inode
void inserirInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto,  int enderecoInodePrincipal, int &quantidadeBlocosNecessarios, int InseridoPeloTriplo = 0)
{
    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO)
    {
        int inicio = disco[enderecoInodeIndireto].inodeIndireto.TL;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO)
        {
            if (isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
            {
                disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = criarINodeIndireto(disco);               
            }

            inserirInodeIndiretoSimples(disco,
                                        disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio],
                                        quantidadeBlocosNecessarios,
                                        enderecoInodePrincipal,
                                        InseridoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);

            disco[enderecoInodeIndireto].inodeIndireto.TL++;
            inicio++;
        }
    }
}

// utilizado para inserir ao criar o inode
void inserirInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto,  int enderecoInodePrincipal, int &quantidadeBlocosNecessarios)
{
    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO)
    {
        int inicio = disco[enderecoInodeIndireto].inodeIndireto.TL;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO)
        {
            if (isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
            {
                disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = criarINodeIndireto(disco);                
            }

            inserirInodeIndiretoDuplo(disco,
                                      disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio],
                                      quantidadeBlocosNecessarios,
                                      enderecoInodePrincipal,
                                      1);

            disco[enderecoInodeIndireto].inodeIndireto.TL++;
            inicio++;
        }
    }
}

// utilizado para exibir o disco
void percorrerInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, exibicaoEndereco enderecos[], int &TL, int ChamadoPeloTriplo = 0)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {
            addExibicaoEndereco(enderecos, TL, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 'A');
            inicio++;
        }
    }
}

// utilizado para exibir o disco
void percorrerInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, exibicaoEndereco enderecos[], int &TL, int ChamadoPeloTriplo = 0)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            addExibicaoEndereco(enderecos, TL, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 'I');
            percorrerInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecos, TL, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            inicio++;
        }
    }
}

// utilizado para exibir o disco
void percorrerInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, exibicaoEndereco enderecos[], int &TL)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            addExibicaoEndereco(enderecos, TL, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 'I');
            percorrerInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecos, TL, 1);
            inicio++;
        }
    }
}



void buscarLivresOcupadosInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, int &qtdBlocosOcupados, int ChamadoPeloTriplo = 0)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {
            qtdBlocosOcupados++;
            inicio++;
        }
    }
}

// utilizado para exibir o disco
void buscarLivresOcupadosInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, int &qtdBlocosOcupados, int ChamadoPeloTriplo = 0)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            qtdBlocosOcupados++;
            buscarLivresOcupadosInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], qtdBlocosOcupados, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            inicio++;
        }
    }
}

// utilizado para exibir o disco
void buscarLivresOcupadosInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, int &qtdBlocosOcupados)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            qtdBlocosOcupados++;
            buscarLivresOcupadosInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], qtdBlocosOcupados, 1);
            inicio++;
        }
    }
}

// utilizado para exibir os diretórios do disco
void listarDiretorioInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, int &linha, char tipoListagem, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeArquivo;
    int i;
    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {

            if (!tipoListagem)
            {
                for (i = 0; i < disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL; i++)
                {
                    printf("%s\t", disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].nome);
                }

                if (linha == 10)
                {
                    printf("\n");
                    linha = 0;
                }
            }else{

                for (i = 0; i < disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL; i++)
                {
                    enderecoInodeArquivo = disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].enderecoINode;
                    printf("%s ", disco[enderecoInodeArquivo].inode.protecao);
                    printf("%d ", disco[enderecoInodeArquivo].inode.contadorLinkFisico);
                    printf("%s ", getNomeProprietario(disco[enderecoInodeArquivo].inode.proprietario).c_str());
                    printf("%s ", getNomeGrupo(disco[enderecoInodeArquivo].inode.grupo).c_str());
                    printf("%d ", disco[enderecoInodeArquivo].inode.tamanhoArquivo);
                    printf("%s ", disco[enderecoInodeArquivo].inode.dataUltimaAlteracao);
                    printf("%s\n", disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].nome);
                }
            
            }

            inicio++;
        }
    }
}

// utilizado para exibir os diretórios do disco
void listarDiretorioInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, int &linha, char tipoListagem, int ChamadoPeloTriplo = 0)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            listarDiretorioInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], linha, tipoListagem, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            inicio++;
        }
    }
}

// utilizado para exibir os diretórios do disco
void listarDiretorioInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, int &linha, char tipoListagem, int ChamadoPeloTriplo = 0)
{

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            listarDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], linha, tipoListagem, 1);
            inicio++;
        }
    }
}

// utilizado para exibir os diretórios do disco
int verificaExistenciaDirInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, string nomeDiretorio, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeArquivo;
    int i;
    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {
            for (i = 0; i < disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL; i++)
            {
                if (strcmp(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].nome, nomeDiretorio.c_str()) == 0)
                {
                    return disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].enderecoINode;
                }
            }
            
            inicio++;
        }
    }

    return getEnderecoNull();
}

// utilizado para exibir os diretórios do disco
int verificaExistenciaDirInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, string nomeDiretorio, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            enderecoInodeDir = verificaExistenciaDirInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], nomeDiretorio, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            if (isEnderecoValido(enderecoInodeDir))
                return enderecoInodeDir;
            
            inicio++;
        }
    }

    return getEnderecoNull();
}

// utilizado para exibir os diretórios do disco
int verificaExistenciaDirInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, string nomeDiretorio, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            enderecoInodeDir = verificaExistenciaDirInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], nomeDiretorio, 1);
            if (isEnderecoValido(enderecoInodeDir))
                return enderecoInodeDir;
            
            inicio++;
        }
    }

    return getEnderecoNull();
}

// utilizado para reservar novo bloco do diretório após criação do inode
int buscaNovoBlocoDiretorioLivreEmIndiretoSimples(Disco disco[], int enderecoInodeIndireto,int enderecoInodeDiretorioAtual, int InseridoPeloTriplo = 0)
{
    bool consegueInserir = false;
    int i;

    int endereco = disco[enderecoInodeIndireto].inodeIndireto.endereco[0];

    for (i = 0; i < MAX_INODEINDIRETO && isEnderecoValido(endereco) && !consegueInserir; i++)
    {
        endereco = disco[enderecoInodeIndireto].inodeIndireto.endereco[i];

        if (!isEnderecoNull(endereco) && disco[endereco].diretorio.TL < 10)
        {
            consegueInserir = true;
        }
    }

    // signfica que não conseguiu inserir e não percorreu todas as funções, senso assim, a próxima posição é -1, então deve ser criado um novo diretório
    if (!consegueInserir && i <= MAX_INODEINDIRETO && isEnderecoNull(endereco))
    {
        endereco = popListaBlocoLivre(disco);
        disco[enderecoInodeIndireto].inodeIndireto.endereco[i - (i == 0 ? 0 : 1)] = endereco;
        disco[enderecoInodeIndireto].inodeIndireto.TL++;
        disco[disco[disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[0]].diretorio.arquivo[0].enderecoINode].inode.tamanhoArquivo++;
    }
    else if (!consegueInserir) // chegou ao último endereço e mesmo assim não conseguiu inserir
    {
        return getEnderecoNull();
    }

    // TODO - VERIFICAR SE está SENDO PERCORRIDO PELO TRIPLO, E SE O último endereço INDIRETO É OQ SOBROU, SENDO ASSIM, DEVE SE CRIAR UM INODE EXTRA
    return endereco;
}

// utilizado para reservar novo bloco do diretório após criação do inode
int buscaNovoBlocoDiretorioLivreEmIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, int enderecoInodeDiretorioAtual, int InseridoPeloTriplo = 0)
{
    bool conseguiuInserir = false;
    int endereco = getEnderecoNull();

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    int inicio = 0;
    while (inicio < MAX_INODEINDIRETO && !conseguiuInserir)
    {
        if (!isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
        {
            endereco = buscaNovoBlocoDiretorioLivreEmIndiretoSimples(disco,
                                                                     disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio],
                                                                     InseridoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            if (isEnderecoValido(endereco))
                conseguiuInserir = true;
        }

        if (isEnderecoNull(endereco) && isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
        {
            if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO)
            {
                disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = criarINodeIndireto(disco);
                disco[enderecoInodeIndireto].inodeIndireto.TL++;
                disco[disco[disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[0]].diretorio.arquivo[0].enderecoINode].inode.tamanhoArquivo++;
                endereco = buscaNovoBlocoDiretorioLivreEmIndiretoSimples(disco,
                                                                         disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio],
                                                                         InseridoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
                conseguiuInserir = true;
            }
            else
                break;
        }

        inicio++;
    }

    if (conseguiuInserir)
        return endereco;

    return getEnderecoNull();
}

// utilizado para reservar novo bloco após criação do inode
int buscaNovoBlocoDiretorioLivreEmIndiretoTriplo(Disco disco[], int enderecoInodeIndireto,int enderecoInodeDiretorioAtual)
{
    bool conseguiuInserir = false;
    int endereco = getEnderecoNull();

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    int inicio = 0;
    while (inicio < MAX_INODEINDIRETO && !conseguiuInserir)
    {
        if (!isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
        {
            endereco = buscaNovoBlocoDiretorioLivreEmIndiretoDuplo(disco,
                                                                   disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 1);
            if (isEnderecoValido(endereco))
                conseguiuInserir = true;
        }

        if (isEnderecoNull(endereco) && isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
        {
            if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO)
            {
                disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = criarINodeIndireto(disco);
                disco[enderecoInodeIndireto].inodeIndireto.TL++;
                disco[disco[disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[0]].diretorio.arquivo[0].enderecoINode].inode.tamanhoArquivo++;
                endereco = buscaNovoBlocoDiretorioLivreEmIndiretoDuplo(disco,
                                                                       disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 1);
                conseguiuInserir = true;
            }
            else
                break;
        }

        inicio++;
    }

    if (conseguiuInserir)
        return endereco;

    return getEnderecoNull();
}

int criarINode(Disco disco[], char tipoArquivo, char permissao[10], int tamanhoArquivo = 1, int enderecoInodePai = -1)
{
    int quantidadeBlocosNecessarios = (int)ceil((double)tamanhoArquivo / (double)10);

    // quantidade de blocos necessárias é maior que a quantidade de blocos livres. Se for, não posso gerar o arquivo, então retorna "-1"
    if (quantidadeBlocosNecessarios > getQuantidadeBlocosLivres(disco))
    {
        return getEnderecoNull();
    }

    int enderecoInode = popListaBlocoLivre(disco);
    if (isEnderecoValido(enderecoInode))
    {
        // inicializa bloco
        for (int i = 0; i < 5; i++)
        {
            setEnderecoNull(disco[enderecoInode].inode.enderecoDireto[i]);
        }
        setEnderecoNull(disco[enderecoInode].inode.enderecoSimplesIndireto);
        setEnderecoNull(disco[enderecoInode].inode.enderecoDuploIndireto);
        setEnderecoNull(disco[enderecoInode].inode.enderecoTriploIndireto);

        disco[enderecoInode].inode.protecao[0] = tipoArquivo;
        strncpy(disco[enderecoInode].inode.protecao + 1, permissao, 10);

        disco[enderecoInode].inode.contadorLinkFisico = 1;
        disco[enderecoInode].inode.tamanhoArquivo = tamanhoArquivo;

        setDataHoraAtualSistema(disco[enderecoInode].inode.dataCriacao);
        setDataHoraAtualSistema(disco[enderecoInode].inode.dataUltimaAlteracao);
        setDataHoraAtualSistema(disco[enderecoInode].inode.dataUltimoAcesso);

        // usuário root
        disco[enderecoInode].inode.proprietario = 1000;
        // grupo root
        disco[enderecoInode].inode.grupo = 1000;

        int quantidadeBlocosUtilizados = 0;

        int i = 0;
        while (i < quantidadeBlocosNecessarios && i < 5)
        {
            if (isEnderecoNull(disco[enderecoInode].inode.enderecoDireto[quantidadeBlocosUtilizados]))
            {
                disco[enderecoInode].inode.enderecoDireto[quantidadeBlocosUtilizados] = popListaBlocoLivre(disco);
                quantidadeBlocosUtilizados++;
            }
            i++;
        }

        int quantidadeBlocosFaltantes = quantidadeBlocosNecessarios - quantidadeBlocosUtilizados;
        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInode].inode.enderecoSimplesIndireto))
            {
                disco[enderecoInode].inode.enderecoSimplesIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoSimples(disco, disco[enderecoInode].inode.enderecoSimplesIndireto, enderecoInode, quantidadeBlocosFaltantes);
        }

        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInode].inode.enderecoDuploIndireto))
            {
                disco[enderecoInode].inode.enderecoDuploIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoDuplo(disco, disco[enderecoInode].inode.enderecoDuploIndireto, enderecoInode, quantidadeBlocosFaltantes);
        }

        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInode].inode.enderecoTriploIndireto))
            {
                disco[enderecoInode].inode.enderecoTriploIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoTriplo(disco, disco[enderecoInode].inode.enderecoTriploIndireto, enderecoInode, quantidadeBlocosFaltantes);
        }

        if (tipoArquivo == TIPO_ARQUIVO_DIRETORIO)
        {
            // caso o tipo de arquivo criado for diretório, é necessário criar dois diretórios dentro dele, sendo:
            //  "."(Aponta para o inode do diretório atual) e
            //  ".."(Aponta para o inode do diretório anterior)

            // strcpy(nomeDiretorioAtual, ".");
            // strcpy(nomeDiretorioAnterior, "..");

            // //criação do diretório "." - atual
            // addArquivoNoDiretorio(disco, enderecoInode, enderecoInode, nomeDiretorioAtual);

            // //criação do diretório ".." - pai
            // addDiretorio(disco, enderecoInodeDiretorioAtual, enderecoInode, nomeDiretorioAnterior);
            if (isEnderecoNull(enderecoInodePai))
                enderecoInodePai = enderecoInode;

            addArquivoNoDiretorio(disco, disco[enderecoInode].inode.enderecoDireto[0], enderecoInode, ".");
            addArquivoNoDiretorio(disco, disco[enderecoInode].inode.enderecoDireto[0], enderecoInodePai, "..");
        }
    }

    // retorna endereço do inode criado
    return enderecoInode;
}

char isDirFull(Diretorio diretorio)
{
    return diretorio.TL == DIRETORIO_LIMITE_ARQUIVOS;
}

// adiciona o arquivo dentro da entrada do diretório
void addArquivoNoDiretorio(Disco disco[], int enderecoDiretorio, int enderecoInodeCriado, char nomeDiretorioArquivo[MAX_NOME_ARQUIVO])
{

    disco[enderecoDiretorio].diretorio.arquivo[disco[enderecoDiretorio].diretorio.TL].enderecoINode = enderecoInodeCriado;

    strcpy(disco[enderecoDiretorio].diretorio.arquivo[disco[enderecoDiretorio].diretorio.TL++].nome, nomeDiretorioArquivo);
}

// função que busca qual o diretório que será possível inserir o arquivo ou diretório
int addDiretorioEArquivo(Disco disco[], char tipoArquivo, int enderecoInodeDiretorioAtual, char nomeDiretorioArquivoNovo[MAX_NOME_ARQUIVO], int tamanhoArquivo=1)
{

    bool consegueInserir = false;
    int i;
    int enderecoInodeDiretorio;
    char permissao[10];

    int enderecoDireto = disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[0];

    for (i = 0; i < 5 && isEnderecoValido(enderecoDireto) && !consegueInserir; i++)
    {
        enderecoDireto = disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[i];

        if (isEnderecoValido(enderecoDireto) && disco[enderecoDireto].diretorio.TL < 10)
        {
            consegueInserir = true;
        }
    }

    // pergunta se não conseguiu inserir no diretório e finalizou o while antes mesmo de atingir todos os Endereços.
    if (!consegueInserir && i <= 5 && isEnderecoNull(enderecoDireto)) // significa que há blocos livres para ser preenchidos e o diretório está cheio.
    {
        enderecoInodeDiretorio = popListaBlocoLivre(disco);
        disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[i - 1] = enderecoInodeDiretorio;
        disco[disco[disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[0]].diretorio.arquivo[0].enderecoINode].inode.tamanhoArquivo++;
    }
    else if (!consegueInserir) // todos os blocos estáo cheio, consequentemente deve verificar se pode adicionar nos blocos indiretos
    {
        if (isEnderecoNull(disco[enderecoInodeDiretorioAtual].inode.enderecoSimplesIndireto))
        {
            disco[enderecoInodeDiretorioAtual].inode.enderecoSimplesIndireto = criarINodeIndireto(disco);
        }
        enderecoInodeDiretorio = buscaNovoBlocoDiretorioLivreEmIndiretoSimples(disco, disco[enderecoInodeDiretorioAtual].inode.enderecoSimplesIndireto, enderecoInodeDiretorioAtual);

        // não conseguiu buscar um bloco disponível no inode indireto simples
        if (isEnderecoNull(enderecoInodeDiretorio))
        {
            if (isEnderecoNull(disco[enderecoInodeDiretorioAtual].inode.enderecoDuploIndireto))
            {
                disco[enderecoInodeDiretorioAtual].inode.enderecoDuploIndireto = criarINodeIndireto(disco);
            }
            enderecoInodeDiretorio = buscaNovoBlocoDiretorioLivreEmIndiretoDuplo(disco, disco[enderecoInodeDiretorioAtual].inode.enderecoDuploIndireto, enderecoInodeDiretorioAtual);

            // não conseguiu buscar um bloco disponível no inode indireto duplo
            if (isEnderecoNull(enderecoInodeDiretorio))
            {
                if (isEnderecoNull(disco[enderecoInodeDiretorioAtual].inode.enderecoTriploIndireto))
                {
                    disco[enderecoInodeDiretorioAtual].inode.enderecoTriploIndireto = criarINodeIndireto(disco);
                }

                enderecoInodeDiretorio = buscaNovoBlocoDiretorioLivreEmIndiretoTriplo(disco, disco[enderecoInodeDiretorioAtual].inode.enderecoTriploIndireto, enderecoInodeDiretorioAtual);
            }
        }        
    }
    else // identificou um bloco que pode ser utilizado para inserir o diretório
    {
        enderecoInodeDiretorio = enderecoDireto;
    }
    // TODO - DESCOBRIR QUAL O BLOCO LIVRE PARA INSERIR O diretório NO diretório ATUAL

    if (!enderecoInodeDiretorio)
        return getEnderecoNull();
    else
    {
        if (tipoArquivo == TIPO_ARQUIVO_DIRETORIO)
        {
            convertPermissaoUGOToString(PERMISSAO_PADRAO_DIRETORIO, permissao, 0);
        }
        else
        {
            convertPermissaoUGOToString(PERMISSAO_PADRAO_ARQUIVO, permissao, 0);
        }

        // adiciona o arquivo dentro da entrada do diretório
        addArquivoNoDiretorio(disco, enderecoInodeDiretorio, criarINode(disco, tipoArquivo, permissao, tamanhoArquivo, enderecoInodeDiretorioAtual), nomeDiretorioArquivoNovo);
    }
    return enderecoInodeDiretorio;
}

int criaDiretorioRaiz(Disco disco[])
{
    char permissao[10];
    char nomeArquivo[MAX_NOME_ARQUIVO];

    convertPermissaoUGOToString(PERMISSAO_PADRAO_DIRETORIO, permissao, 0);
    int enderecoInodeDiretorioRaiz = criarINode(disco, TIPO_ARQUIVO_DIRETORIO, permissao);
    return enderecoInodeDiretorioRaiz;
}

void quickSort(int ini, int fim, exibicaoEndereco vet[])
{
    int i = ini;
    int j = fim;
    int troca = true;

    // exibicaoEndereco exibEnd;
    int end;
    char info;

    while (i < j)
    {
        if (troca)
            while (i < j && vet[i].endereco <= vet[j].endereco)
                i++;
        else
            while (i < j && vet[j].endereco >= vet[i].endereco)
                j--;

        end = vet[i].endereco;
        info = vet[i].info;

        vet[i].endereco = vet[j].endereco;
        vet[i].info = vet[j].info;

        vet[j].endereco = end;
        vet[j].info = info;

        // exibEnd = vet[i];
        // vet[i] = vet[j];
        // vet[j] = exibEnd;
        troca = !troca;
    }

    if (ini < i - 1)
        quickSort(ini, i - 1, vet);

    if (j + 1 < fim)
        quickSort(j + 1, fim, vet);
}

void exibirDisco(Disco disco[], int tamanhoDisco, int quantidadeBlocosNecessariosListaLivre)
{
    exibicaoEndereco enderecos[tamanhoDisco];
    int TLEnderecos = 0;
    char nomeBlocos[5];
    int quantidadeCasasTamanhoDisco = (int)trunc(log10(tamanhoDisco)) + 1;
    int quantidadeBlocosLivres = getQuantidadeBlocosLivres(disco);

    int linha = 0;
    for (int i = 0; i < tamanhoDisco; i++)
    {
        if (disco[i].bad)
        {
            addExibicaoEndereco(enderecos, TLEnderecos, i, 'B');
        }
        // else if(disco[i].diretorio.TL > 0){
        //     addExibicaoEndereco(enderecos, TLEnderecos, i, 'A');
        // }
        else if (strlen(disco[i].inode.protecao) > 0)
        {
            // printf("%d - inode\n", i);
            // tem um i-node
            addExibicaoEndereco(enderecos, TLEnderecos, i, 'I');
            for (int j = 0; j < 5 && !isEnderecoNull(disco[i].inode.enderecoDireto[j]); j++)
            {
                // printf("%d - arquivo\n", disco[i].inode.enderecoDireto[j]);
                if (!isEnderecoNull(disco[i].inode.enderecoDireto[j]))
                    addExibicaoEndereco(enderecos, TLEnderecos, disco[i].inode.enderecoDireto[j], 'A');
            }

            if (!isEnderecoNull(disco[i].inode.enderecoSimplesIndireto))
            {
                int enderecoIndireto = 0;
                enderecoIndireto = disco[i].inode.enderecoSimplesIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                percorrerInodeIndiretoSimples(disco, enderecoIndireto, enderecos, TLEnderecos);
            }

            if (!isEnderecoNull(disco[i].inode.enderecoDuploIndireto))
            {
                int enderecoIndireto = disco[i].inode.enderecoDuploIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                percorrerInodeIndiretoDuplo(disco, enderecoIndireto, enderecos, TLEnderecos);
            }

            if (!isEnderecoNull(disco[i].inode.enderecoTriploIndireto))
            {
                int enderecoIndireto = disco[i].inode.enderecoTriploIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                percorrerInodeIndiretoTriplo(disco, enderecoIndireto, enderecos, TLEnderecos);
            }
        }
        // else if(disco[i].inodeIndireto.TL > 0)
        // {
        //     //ignora o indireto, será realizado no inode
        // }
        else if (strlen(disco[i].ls.caminho) > 0)
        {
            addExibicaoEndereco(enderecos, TLEnderecos, i, 'L');
        }
        else if (disco[i].lbl.topo > -1 || (i >= ENDERECO_CABECA_LISTA && i < ENDERECO_CABECA_LISTA + quantidadeBlocosNecessariosListaLivre))
        {

            addExibicaoEndereco(enderecos, TLEnderecos, i, 'R');
        }
        else
        {
            // addExibicaoEndereco(enderecos, TLEnderecos, i, 'F');
        }
    }

    for (int i = ENDERECO_CABECA_LISTA; i < ENDERECO_CABECA_LISTA + quantidadeBlocosNecessariosListaLivre && disco[i].lbl.topo > -1; i++)
    {
        for (int j = 0; j <= disco[i].lbl.topo; j++)
        {
            if (!disco[disco[i].lbl.endereco[j]].bad)
                addExibicaoEndereco(enderecos, TLEnderecos, disco[i].lbl.endereco[j], 'F');
        }
    }

    quickSort(0, TLEnderecos - 1, enderecos);

    int linhaTela = 1;
    int colunaTela = 1;

    for (int i = 0; i < TLEnderecos; i++)
    {

        // for(int j=0; j<quantidadeCasasTamanhoDisco; j++)
        // {
        //     // printf("%d");
        // }

        printf("[%d %c]\t", enderecos[i].endereco, enderecos[i].info);

        if (linha == 10)
        {
            printf("\n");
            linha = 0;
        }
        linha++;
    }
}

void listarDiretorio(Disco disco[], int enderecoInodeAtual)
{
    int direto, i, linha = 0;

    for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
    {
        for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
        {
            printf("%s\t", disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome);
        }

        if (linha == 10)
        {
            printf("\n");
            linha = 0;
        }
        linha++;
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoSimplesIndireto))
    {
        listarDiretorioInodeIndiretoSimples(disco, disco[enderecoInodeAtual].inode.enderecoSimplesIndireto, linha, 0);
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoDuploIndireto))
    {
        listarDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeAtual].inode.enderecoDuploIndireto, linha,0);
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoTriploIndireto))
    {
        listarDiretorioInodeIndiretoTriplo(disco, disco[enderecoInodeAtual].inode.enderecoTriploIndireto, linha,0);
    }
}

void listarDiretorioComAtributos(Disco disco[], int enderecoInodeAtual)
{
    int direto, i, linha = 0, enderecoInodeArquivo;

    for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
    {
        for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
        {
            enderecoInodeArquivo = disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode;

            printf("%s ", disco[enderecoInodeArquivo].inode.protecao);
            printf("%d ", disco[enderecoInodeArquivo].inode.contadorLinkFisico);
            printf("%s ", getNomeProprietario(disco[enderecoInodeArquivo].inode.proprietario).c_str());
            printf("%s ", getNomeGrupo(disco[enderecoInodeArquivo].inode.grupo).c_str());
            printf("%d ", disco[enderecoInodeArquivo].inode.tamanhoArquivo);
            printf("%s ", disco[enderecoInodeArquivo].inode.dataUltimaAlteracao);
            printf("%s\n", disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome);
        }
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoSimplesIndireto))
    {
        listarDiretorioInodeIndiretoSimples(disco, disco[enderecoInodeAtual].inode.enderecoSimplesIndireto, linha, 1);
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoDuploIndireto))
    {
        listarDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeAtual].inode.enderecoDuploIndireto, linha, 1);
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoTriploIndireto))
    {
        listarDiretorioInodeIndiretoTriplo(disco, disco[enderecoInodeAtual].inode.enderecoTriploIndireto, linha, 1);
    }
}

int cd(Disco disco[], int enderecoInodeAtual, string nomeDiretorio, int enderecoInodeRaiz, string &caminhoAbsoluto)
{
    int direto, i, endereco = -1;

    if (strcmp(nomeDiretorio.c_str(), ".") == 0)
    {
        return disco[disco[enderecoInodeAtual].inode.enderecoDireto[0]].diretorio.arquivo[0].enderecoINode;
    }
    else if (strcmp(nomeDiretorio.c_str(), "..") == 0)
    {
        if (enderecoInodeAtual == enderecoInodeRaiz)
        {
            caminhoAbsoluto.assign("~");
        }
        else
        {
            // /teste/teste/teste/
            // int ant=0, atual=caminhoAbsoluto.size();
            // while(caminhoAbsoluto.at(atual-1))
            // {

            //    atual--;               
            // }
            
            //encontra a ultima ocorrência de uma string
            size_t pos = caminhoAbsoluto.rfind("/");

            //verifica se a posição encontrada é valida
            //se achou uma posicao da string fornecida
            if (pos != std::string::npos) {
                //a partir da posicao encontrada remove parte do caminho
                caminhoAbsoluto.replace(pos, caminhoAbsoluto.size(), "");                
            }

            
        
        }

        return disco[disco[enderecoInodeAtual].inode.enderecoDireto[0]].diretorio.arquivo[1].enderecoINode;
    }
    else
    {
        //verifica nos demais blocos qual diretório é o informado.
        for (direto = 2; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
        {
            for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
            {
                if (strcmp(disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome, nomeDiretorio.c_str()) == 0)
                {                    
                    caminhoAbsoluto.append("/"+nomeDiretorio);
                    return disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode;
                }
            }
        }

        if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoSimplesIndireto))
        {
            endereco = verificaExistenciaDirInodeIndiretoSimples(disco, disco[enderecoInodeAtual].inode.enderecoSimplesIndireto, nomeDiretorio);
        }
       
        if (isEnderecoValido(endereco))
        {
            caminhoAbsoluto.append("/"+nomeDiretorio);
            return endereco;
        }

        if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoDuploIndireto))
        {
            endereco = verificaExistenciaDirInodeIndiretoDuplo(disco, disco[enderecoInodeAtual].inode.enderecoDuploIndireto, nomeDiretorio);
        }

        
        if (isEnderecoValido(endereco))
        {
            caminhoAbsoluto.append("/"+nomeDiretorio);
            return endereco;
        }

        if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoTriploIndireto))
        {
            endereco = verificaExistenciaDirInodeIndiretoTriplo(disco, disco[enderecoInodeAtual].inode.enderecoTriploIndireto, nomeDiretorio, 1);
        }
       
        if (isEnderecoValido(endereco)) 
        {
            caminhoAbsoluto.append("/"+nomeDiretorio);
            return endereco;
        }

        return enderecoInodeAtual;
    }
}


void touch(Disco disco[], int enderecoInodeAtual, string comando)
{
    istringstream stringStream(comando);
    string stringAuxiliar;

    vector<string> vetorStringSeparado;
    while (getline(stringStream, stringAuxiliar, ' ')) 
    {
        vetorStringSeparado.push_back(stringAuxiliar);
    }

    if (vetorStringSeparado.size() == 2 && vetorStringSeparado.at(0).size() <= MAX_NOME_ARQUIVO)
    {
        char nomeArquivo[MAX_NOME_ARQUIVO];
        strcpy(nomeArquivo, vetorStringSeparado.at(0).c_str());
        int tamanhoArquivo = atoi(vetorStringSeparado.at(1).c_str());
        addDiretorioEArquivo(disco, TIPO_ARQUIVO_ARQUIVO, enderecoInodeAtual, nomeArquivo, tamanhoArquivo);
    }
}



void buscaBlocosLivresOcupados(Disco disco[], int &qtdBlocosLivres, int &qtdBlocosOcupados, int quantidadeBlocosTotais, int quantidadeBlocosNecessariosListaLivre)
{

    for(int i=0; i < quantidadeBlocosTotais; i++)
    {
        if (disco[i].bad)
        {
            qtdBlocosOcupados++;
        }
        // else if(disco[i].diretorio.TL > 0){
        //     addExibicaoEndereco(enderecos, TLEnderecos, i, 'A');
        // }
        else if (strlen(disco[i].inode.protecao) > 0)
        {
            qtdBlocosOcupados++;
            for (int j = 0; j < 5 && !isEnderecoNull(disco[i].inode.enderecoDireto[j]); j++)
            {
                // printf("%d - arquivo\n", disco[i].inode.enderecoDireto[j]);
                if (!isEnderecoNull(disco[i].inode.enderecoDireto[j]))
                    qtdBlocosOcupados++;
            }

            if (!isEnderecoNull(disco[i].inode.enderecoSimplesIndireto))
            {
                int enderecoIndireto = 0;
                enderecoIndireto = disco[i].inode.enderecoSimplesIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                buscarLivresOcupadosInodeIndiretoSimples(disco, enderecoIndireto, qtdBlocosOcupados);
            }

            if (!isEnderecoNull(disco[i].inode.enderecoDuploIndireto))
            {
                int enderecoIndireto = disco[i].inode.enderecoDuploIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                buscarLivresOcupadosInodeIndiretoDuplo(disco, enderecoIndireto, qtdBlocosOcupados);
            }

            if (!isEnderecoNull(disco[i].inode.enderecoTriploIndireto))
            {
                int enderecoIndireto = disco[i].inode.enderecoTriploIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                buscarLivresOcupadosInodeIndiretoTriplo(disco, enderecoIndireto, qtdBlocosOcupados);
            }
        }
        
       
        else if (strlen(disco[i].ls.caminho) > 0)
        {
            qtdBlocosOcupados++;
        }
        else if (disco[i].lbl.topo > -1 || (i >= ENDERECO_CABECA_LISTA && i < ENDERECO_CABECA_LISTA + quantidadeBlocosNecessariosListaLivre))
        {

            qtdBlocosOcupados++;
        }
        
    }

    for (int i = ENDERECO_CABECA_LISTA; i < ENDERECO_CABECA_LISTA + quantidadeBlocosNecessariosListaLivre && disco[i].lbl.topo > -1; i++)
    {
        for (int j = 0; j <= disco[i].lbl.topo; j++)
        {
            if (!disco[disco[i].lbl.endereco[j]].bad)
                qtdBlocosLivres++;
        }
    }

}