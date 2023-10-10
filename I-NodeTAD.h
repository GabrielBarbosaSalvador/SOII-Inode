#include "Permissao.h"
#include "splitTAD.h"
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
#define PERMISSAO_PADRAO_LINKSIMBOLICO 777


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
    string caminho;
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

// prototipos
int criarINode(Disco disco[], char tipoArquivo, char permissao[10], int tamanhoArquivo, int enderecoInodePai, string caminhoLink);
int addDiretorioEArquivo(Disco disco[], char tipoArquivo, int enderecoInodeDiretorioAtual, char nomeDiretorioArquivoNovo[MAX_NOME_ARQUIVO], int tamanhoArquivo, string caminhoLink, int enderecoInodeCriado);
void addArquivoNoDiretorio(Disco disco[], int enderecoDiretorio, int enderecoInodeCriado, char nomeDiretorioArquivo[MAX_NOME_ARQUIVO]);
int getQuantidadeBlocosUsar(Disco disco[], int quantidadeBlocosNecessarios);
void vi(Disco disco[], int enderecoInodeAtual, string nomeArquivo, string &enderecosUtilizados);
bool rm(Disco disco[], int enderecoInodeAtual, string nomeArquivo, int primeiraVez);
bool rmdir(Disco disco[], int enderecoInodeAtual, string nomeDiretorio, int &contadorDiretorio, int primeiraVez);
int cd(Disco disco[], int enderecoInodeAtual, string nomeDiretorio, int enderecoInodeRaiz, string &caminhoAbsoluto);

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
    int quantidadeBlocosLivres = 0;

    while (enderecoProxBloco != -1 && disco[enderecoProxBloco].lbl.topo > -1)
    {
        for (int j = 0; j <= disco[enderecoProxBloco].lbl.topo; j++)
        {
            if (!disco[disco[enderecoProxBloco].lbl.endereco[j]].bad)
                quantidadeBlocosLivres++;
        }

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
                                                                                                                        enderecoInodePrincipal, "");

        // conseguiu criar o inode
        if (isEnderecoValido(disco[enderecoInodeIndireto].inodeIndireto.endereco[disco[enderecoInodeIndireto].inodeIndireto.TL]))
        {
            disco[enderecoInodeIndireto].inodeIndireto.TL++;
        }
    }
}

// utilizado para inserir ao criar o inode
void inserirInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, int enderecoInodePrincipal, int &quantidadeBlocosNecessarios, int InseridoPeloTriplo = 0)
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
                                        enderecoInodePrincipal,
                                        quantidadeBlocosNecessarios,
                                        InseridoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);

            disco[enderecoInodeIndireto].inodeIndireto.TL++;
            inicio++;
        }
    }
}

// utilizado para inserir ao criar o inode
void inserirInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, int enderecoInodePrincipal, int &quantidadeBlocosNecessarios)
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
                                      enderecoInodePrincipal,
                                      quantidadeBlocosNecessarios,
                                      1);

            disco[enderecoInodeIndireto].inodeIndireto.TL++;
            inicio++;
        }
    }
}

void preInsereInodeIndiretoSimples(Disco disco[], int &quantidadeBlocosNecessarios, int &quantidadeBlocosUsou, int InseridoPeloTriplo = 0)
{
    int quantidadeBlocosUtilizados = 0;
    // verifica se a quantidade de Endereços no inode indireto não está cheio

    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO - InseridoPeloTriplo)
    {
        quantidadeBlocosUtilizados++;
        inicio++;
    }

    // gera a quantidade ainda faltante.
    quantidadeBlocosNecessarios = quantidadeBlocosNecessarios - quantidadeBlocosUtilizados;
    quantidadeBlocosUsou += quantidadeBlocosUtilizados;
    if (InseridoPeloTriplo && quantidadeBlocosNecessarios > 0)
    {
        quantidadeBlocosUsou++;
        quantidadeBlocosUsou += getQuantidadeBlocosUsar(disco, quantidadeBlocosNecessarios) - 1;
    }
}

// utilizado para inserir ao criar o inode
void preInsereInodeIndiretoDuplo(Disco disco[], int &quantidadeBlocosNecessarios, int &quantidadeBlocosUsou, int InseridoPeloTriplo = 0)
{
    // verifica se a quantidade de Endereços no inode indireto não está cheio

    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO)
    {
        quantidadeBlocosUsou++;
        preInsereInodeIndiretoSimples(disco, quantidadeBlocosNecessarios, quantidadeBlocosUsou,
                                      InseridoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);

        inicio++;
    }
}

// utilizado para inserir ao criar o inode
void preInsereInodeIndiretoTriplo(Disco disco[], int &quantidadeBlocosNecessarios, int &quantidadeBlocosUsou)
{

    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO)
    {
        quantidadeBlocosUsou++;
        preInsereInodeIndiretoDuplo(disco, quantidadeBlocosNecessarios, quantidadeBlocosUsou, 1);

        inicio++;
    }
}

int getQuantidadeBlocosUsar(Disco disco[], int quantidadeBlocosNecessarios)
{
    int quantidadeUsou = 0;
    int quantidadeBlocosUtilizados = 0;
    int i = 0;
    while (i < quantidadeBlocosNecessarios && i < 5)
    {
        quantidadeBlocosUtilizados++;
        i++;
    }

    quantidadeUsou += quantidadeBlocosUtilizados;
    int quantidadeBlocosFaltantes = quantidadeBlocosNecessarios - quantidadeBlocosUtilizados;
    if (quantidadeBlocosFaltantes > 0)
    {
        quantidadeUsou++;
        preInsereInodeIndiretoSimples(disco, quantidadeBlocosFaltantes, quantidadeUsou);
    }
    if (quantidadeBlocosFaltantes > 0)
    {
        quantidadeUsou++;
        preInsereInodeIndiretoDuplo(disco, quantidadeBlocosFaltantes, quantidadeUsou);
    }

    if (quantidadeBlocosFaltantes > 0)
    {
        quantidadeUsou++;
        preInsereInodeIndiretoTriplo(disco, quantidadeBlocosFaltantes, quantidadeUsou);
    }

    return quantidadeUsou + 1;
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
            }
            else
            {

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
int verificaExistenciaArquivoOuDiretorioInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, char tipoArquivo, string nomeArquivo, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeArquivo;
    int i;
    int inicio = 0;

    if (tipoArquivo == ' ')
    {
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {
            for (i = 0; i < disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL; i++)
            {
                if (strcmp(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                {
                    return disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].enderecoINode;
                }
            }

            inicio++;
        }
    }
    else
    {
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {
            for (i = 0; i < disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL; i++)
            {
                if (disco[disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].enderecoINode].inode.protecao[0] == tipoArquivo)
                {
                    if (strcmp(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                    {
                        return disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].enderecoINode;
                    }
                }
            }

            inicio++;
        }
    }

    return getEnderecoNull();
}

// utilizado para exibir os diretórios do disco
int verificaExistenciaArquivoOuDiretorioInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, char tipoArquivo, string nomeArquivo, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            enderecoInodeDir = verificaExistenciaArquivoOuDiretorioInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], tipoArquivo, nomeArquivo, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            if (isEnderecoValido(enderecoInodeDir))
                return enderecoInodeDir;

            inicio++;
        }
    }

    return getEnderecoNull();
}

// utilizado para exibir os diretórios do disco
int verificaExistenciaArquivoOuDiretorioInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, char tipoArquivo, string nomeArquivo, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            enderecoInodeDir = verificaExistenciaArquivoOuDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], tipoArquivo, nomeArquivo, 1);
            if (isEnderecoValido(enderecoInodeDir))
                return enderecoInodeDir;

            inicio++;
        }
    }

    return getEnderecoNull();
}

int buscaEnderecoEntradaDiretorioArquivoInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, char tipoArquivo, string nomeArquivo, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeArquivo;
    int i;
    int inicio = 0;

    if (tipoArquivo == ' ')
    {
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {
            for (i = 0; i < disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL; i++)
            {
                if (strcmp(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                {
                    return disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio];
                }
            }
            inicio++;
        }
    }
    else
    {
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {
            for (i = 0; i < disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL; i++)
            {
                if (disco[disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].enderecoINode].inode.protecao[0] == tipoArquivo)
                {
                    if (strcmp(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                    {
                        return disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio];
                    }
                }
            }
            inicio++;
        }
    }

    //TODO - CHAMAR INODE AUXILIAR SE NÃO ENCONTRAR
    return getEnderecoNull();
}

// utilizado para exibir os diretórios do disco
int buscaEnderecoEntradaDiretorioArquivoInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, char tipoArquivo, string nomeArquivo, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            enderecoInodeDir = buscaEnderecoEntradaDiretorioArquivoInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], tipoArquivo, nomeArquivo, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            if (isEnderecoValido(enderecoInodeDir))
                return enderecoInodeDir;

            inicio++;
        }
    }

    return getEnderecoNull();
}
// utilizado para exibir os diretórios do disco
int buscaEnderecoEntradaDiretorioArquivoInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, char tipoArquivo, string nomeArquivo, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            enderecoInodeDir = buscaEnderecoEntradaDiretorioArquivoInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], tipoArquivo, nomeArquivo, 1);
            if (isEnderecoValido(enderecoInodeDir))
                return enderecoInodeDir;

            inicio++;
        }
    }

    return getEnderecoNull();
}

// utilizado para reservar novo bloco do diretório após criação do inode
int buscaNovoBlocoDiretorioLivreEmIndiretoSimples(Disco disco[], int enderecoInodeIndireto, int enderecoInodeDiretorioAtual, int InseridoPeloTriplo = 0)
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
int buscaNovoBlocoDiretorioLivreEmIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, int enderecoInodeDiretorioAtual)
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

// utilizado para exibir os blocos do arquivo
void retornaEnderecosInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, string &enderecosUtilizados, int ChamadoPeloTriplo = 0)
{
    char enderecoToChar[300];
    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
    {
        itoa(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecoToChar, 10);
        enderecosUtilizados.append(enderecoToChar).append("-");

        inicio++;
    }

    if (ChamadoPeloTriplo)
    {
        if (isEnderecoValido(disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1]))
        {
            string nomeArq;
            nomeArq.assign("");

            vi(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1], nomeArq, enderecosUtilizados);
        }
    }
}

// utilizado para exibir os blocos do arquivo
void retornaEnderecosInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, string &enderecosUtilizados, int ChamadoPeloTriplo = 0)
{
    char enderecoToChar[300];
    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
    {
        itoa(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecoToChar, 10);
        enderecosUtilizados.append(enderecoToChar).append("-");

        retornaEnderecosInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecosUtilizados, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
        inicio++;
    }
}

// utilizado para exibir os blocos do arquivo
void retornaEnderecosInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, string &enderecosUtilizados)
{
    char enderecoToChar[300];
    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
    {
        itoa(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecoToChar, 10);
        enderecosUtilizados.append(enderecoToChar).append("-");

        retornaEnderecosInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecosUtilizados, 1);
        inicio++;
    }
}

// utilizado para remover os blocos do arquivo
void removeEnderecosInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, int ChamadoPeloTriplo = 0)
{
    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
    {
        initDisco(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]]);
        pushListaBlocoLivre(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]);
        disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = getEnderecoNull();
        inicio++;
    }

    if (ChamadoPeloTriplo)
    {
        if (isEnderecoValido(disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1]))
        {
            string nomeArq;
            nomeArq.assign("");

            rm(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1], nomeArq, 0);

            initDisco(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1]]);
            pushListaBlocoLivre(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1]);
            disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1] = getEnderecoNull();
        }
    }
}

// utilizado para exibir os blocos do arquivo
void removeEnderecosInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, int ChamadoPeloTriplo = 0)
{
    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
    {
        removeEnderecosInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);

        initDisco(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]]);
        pushListaBlocoLivre(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]);
        disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = getEnderecoNull();
        inicio++;
    }
}

// utilizado para exibir os blocos do arquivo
void removeEnderecosInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto)
{

    int inicio = 0;
    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
    {
        removeEnderecosInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 1);

        initDisco(disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]]);
        pushListaBlocoLivre(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]);

        disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = getEnderecoNull();
        inicio++;
    }
}

// utilizado para exibir os diretórios do disco
void retornaQuantidadeDiretorioInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, int &contadorDiretorio, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeArquivo;
    int i;
    int inicio = 0;

    // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
    while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
    {
        contadorDiretorio += disco[disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]].diretorio.TL;
        inicio++;
    }

    if (ChamadoPeloTriplo)
    {
        if (isEnderecoValido(disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1]))
        {
            string nomeArq;
            nomeArq.assign("");

            rmdir(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[MAX_INODEINDIRETO - 1], nomeArq, contadorDiretorio, 0);
        }
    }
}

// utilizado para exibir os diretórios do disco
void retornaQuantidadeDiretorioInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, int &contadorDiretorio, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;

    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            retornaQuantidadeDiretorioInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], contadorDiretorio, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            inicio++;
        }
    }
}

// utilizado para exibir os diretórios do disco
void retornaQuantidadeDiretorioInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, int &contadorDiretorio, int ChamadoPeloTriplo = 0)
{
    int enderecoInodeDir;
    // verifica se a quantidade de Endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        // adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {
            retornaQuantidadeDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], contadorDiretorio, 1);
            inicio++;
        }
    }
}

int existeArquivoOuDiretorio(Disco disco[], int enderecoInodeAtual, string nomeArquivo, char tipoArquivo = ' ', bool igualTipoFornecido=true)
{
    int direto, i, endereco = getEnderecoNull();

    if (tipoArquivo == ' ')
    {
        for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
        {

            for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
            {
                // printf("\n %s -- %s \n", disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome,nomeDiretorio.c_str());
                if (strcmp(disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                {
                    return disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode;
                }
            }
        }
    }
    else
    {
        if (igualTipoFornecido)
        {
            for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
            {

                for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
                {
                    if (disco[disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode].inode.protecao[0] == tipoArquivo)
                    {
                        // printf("\n %s -- %s \n", disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome,nomeDiretorio.c_str());
                        if (strcmp(disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                        {
                            return disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode;
                        }
                    }
                }
            }
        }
        else
        {
            for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
            {

                for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
                {
                    if (disco[disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode].inode.protecao[0] != tipoArquivo)
                    {
                        // printf("\n %s -- %s \n", disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome,nomeDiretorio.c_str());
                        if (strcmp(disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                        {
                            return disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode;
                        }
                    }
                }
            }
        }
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoSimplesIndireto))
    {
        endereco = verificaExistenciaArquivoOuDiretorioInodeIndiretoSimples(disco, disco[enderecoInodeAtual].inode.enderecoSimplesIndireto, tipoArquivo, nomeArquivo);
    }

    if (isEnderecoValido(endereco))
    {
        return endereco;
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoDuploIndireto))
    {
        endereco = verificaExistenciaArquivoOuDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeAtual].inode.enderecoDuploIndireto, tipoArquivo, nomeArquivo);
    }

    if (isEnderecoValido(endereco))
    {
        return endereco;
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoTriploIndireto))
    {
        endereco = verificaExistenciaArquivoOuDiretorioInodeIndiretoTriplo(disco, disco[enderecoInodeAtual].inode.enderecoTriploIndireto, tipoArquivo, nomeArquivo, 1);
    }

    if (isEnderecoValido(endereco))
    {
        return endereco;
    }

    return endereco;
}

int buscaEnderecoEntradaDiretorioArquivo(Disco disco[], int enderecoInodeAtual, string nomeArquivo, char tipoArquivo=' ')
{
    int direto, i, endereco = getEnderecoNull();

    if (tipoArquivo == ' ')
    {
        for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
        {
            for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
            {
                // printf("\n %s -- %s \n", disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome,nomeDiretorio.c_str());
                if (strcmp(disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                {
                    return disco[enderecoInodeAtual].inode.enderecoDireto[direto];
                }
            }
        }
    }
    else
    {
        for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
        {
            for (i = 0; i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
            {
                if (disco[disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode].inode.protecao[0] == tipoArquivo)
                {
                    // printf("\n %s -- %s \n", disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome,nomeDiretorio.c_str());
                    if (strcmp(disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome, nomeArquivo.c_str()) == 0)
                    {
                        return disco[enderecoInodeAtual].inode.enderecoDireto[direto];
                    }
                }
            }
        }
    }
    

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoSimplesIndireto))
    {
        endereco = buscaEnderecoEntradaDiretorioArquivoInodeIndiretoSimples(disco, disco[enderecoInodeAtual].inode.enderecoSimplesIndireto, tipoArquivo, nomeArquivo);
    }

    if (isEnderecoValido(endereco))
    {
        return endereco;
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoDuploIndireto))
    {
        endereco = buscaEnderecoEntradaDiretorioArquivoInodeIndiretoDuplo(disco, disco[enderecoInodeAtual].inode.enderecoDuploIndireto, tipoArquivo, nomeArquivo);
    }

    if (isEnderecoValido(endereco))
    {
        return endereco;
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoTriploIndireto))
    {
        endereco = buscaEnderecoEntradaDiretorioArquivoInodeIndiretoTriplo(disco, disco[enderecoInodeAtual].inode.enderecoTriploIndireto, tipoArquivo, nomeArquivo, 1);
    }

    if (isEnderecoValido(endereco))
    {
        return endereco;
    }

    return endereco;
}

int criarINode(Disco disco[], char tipoArquivo, char permissao[10], int tamanhoArquivo = 1, int enderecoInodePai = -1, string caminhoLink = "")
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

            char nomeDir[MAX_NOME_ARQUIVO];

            strcpy(nomeDir, ".");
            addArquivoNoDiretorio(disco, disco[enderecoInode].inode.enderecoDireto[0], enderecoInode, nomeDir);

            strcpy(nomeDir, "..");
            addArquivoNoDiretorio(disco, disco[enderecoInode].inode.enderecoDireto[0], enderecoInodePai, nomeDir);
        }
        else if (tipoArquivo == TIPO_ARQUIVO_LINK)
        {
            disco[disco[enderecoInode].inode.enderecoDireto[0]].ls.caminho=caminhoLink;
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
int addDiretorioEArquivo(Disco disco[], char tipoArquivo, int enderecoInodeDiretorioAtual, char nomeDiretorioArquivoNovo[MAX_NOME_ARQUIVO], int tamanhoArquivo = 1, string caminhoLink="", int enderecoInodeCriado=getEnderecoNull())
{

    bool consegueInserir = false;
    int i;
    int enderecoInodeDiretorio;
    int enderecoInodeArquivo;
    char permissao[10];

    enderecoInodeArquivo = existeArquivoOuDiretorio(disco, enderecoInodeDiretorioAtual, nomeDiretorioArquivoNovo);
    if (isEnderecoNull(enderecoInodeArquivo))
    {
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
            else if (tipoArquivo == TIPO_ARQUIVO_ARQUIVO)
            {
                convertPermissaoUGOToString(PERMISSAO_PADRAO_ARQUIVO, permissao, 0);
            }
            else if (tipoArquivo == TIPO_ARQUIVO_LINK)
            {
                convertPermissaoUGOToString(PERMISSAO_PADRAO_LINKSIMBOLICO, permissao, 0);
            }

            // adiciona o arquivo dentro da entrada do diretório
            if (isEnderecoNull(enderecoInodeCriado))
                enderecoInodeCriado = criarINode(disco, tipoArquivo, permissao, tamanhoArquivo, enderecoInodeDiretorioAtual, caminhoLink);

            addArquivoNoDiretorio(disco, enderecoInodeDiretorio, enderecoInodeCriado, nomeDiretorioArquivoNovo);
        }
        return enderecoInodeDiretorio;
    }

    return getEnderecoNull();
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
                addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                percorrerInodeIndiretoSimples(disco, enderecoIndireto, enderecos, TLEnderecos);
            }

            if (!isEnderecoNull(disco[i].inode.enderecoDuploIndireto))
            {
                int enderecoIndireto = disco[i].inode.enderecoDuploIndireto;
                addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                percorrerInodeIndiretoDuplo(disco, enderecoIndireto, enderecos, TLEnderecos);
            }

            if (!isEnderecoNull(disco[i].inode.enderecoTriploIndireto))
            {
                int enderecoIndireto = disco[i].inode.enderecoTriploIndireto;
                addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');

                percorrerInodeIndiretoTriplo(disco, enderecoIndireto, enderecos, TLEnderecos);
            }
        }
        // else if(disco[i].inodeIndireto.TL > 0)
        // {
        //     //ignora o indireto, será realizado no inode
        // }
        else if (strlen(disco[i].ls.caminho.c_str()) > 0)
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
        for (i = (direto == 0 ? 2 : 0); i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
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
        listarDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeAtual].inode.enderecoDuploIndireto, linha, 0);
    }

    if (!isEnderecoNull(disco[enderecoInodeAtual].inode.enderecoTriploIndireto))
    {
        listarDiretorioInodeIndiretoTriplo(disco, disco[enderecoInodeAtual].inode.enderecoTriploIndireto, linha, 0);
    }
}

void listarDiretorioComAtributos(Disco disco[], int enderecoInodeAtual)
{
    int direto, i, linha = 0, enderecoInodeArquivo;

    for (direto = 0; direto < 5 && isEnderecoValido(disco[enderecoInodeAtual].inode.enderecoDireto[direto]); direto++)
    {
        for (i = (direto == 0 ? 2 : 0); i < disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.TL; i++)
        {
            enderecoInodeArquivo = disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode;

            printf("%s ", disco[enderecoInodeArquivo].inode.protecao);
            printf("%d ", disco[enderecoInodeArquivo].inode.contadorLinkFisico);
            printf("%s ", getNomeProprietario(disco[enderecoInodeArquivo].inode.proprietario).c_str());
            printf("%s ", getNomeGrupo(disco[enderecoInodeArquivo].inode.grupo).c_str());
            printf("%d ", disco[enderecoInodeArquivo].inode.tamanhoArquivo);
            printf("%s ", disco[enderecoInodeArquivo].inode.dataUltimaAlteracao);

            if (disco[enderecoInodeArquivo].inode.protecao[0] == TIPO_ARQUIVO_LINK)
            {
                printf("%s",disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].nome);
                textcolor(LIGHTBLUE);
                printf(" -> %s\n", 
                                    disco[disco[disco[disco[enderecoInodeAtual].inode.enderecoDireto[direto]].diretorio.arquivo[i].enderecoINode].inode.enderecoDireto[0]].ls.caminho.c_str());
                textcolor(WHITE);
            }
            else
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
            // encontra a ultima ocorrência de uma string
            size_t pos = caminhoAbsoluto.rfind("/");

            // verifica se a posição encontrada é valida
            // se achou uma posicao da string fornecida
            if (pos != std::string::npos)
            {
                // a partir da posicao encontrada remove parte do caminho
                caminhoAbsoluto.replace(pos, caminhoAbsoluto.size(), "");
            }
        }

        return disco[disco[enderecoInodeAtual].inode.enderecoDireto[0]].diretorio.arquivo[1].enderecoINode;
    }
    else if (strcmp(nomeDiretorio.c_str(), "/") == 0)
    {
        caminhoAbsoluto.assign("~");
        return enderecoInodeRaiz;
    }
    else if (strcmp(nomeDiretorio.c_str(), "~") == 0)
    {
        caminhoAbsoluto.assign("~");
        return enderecoInodeRaiz;
    }
    else
    {
        
        endereco = existeArquivoOuDiretorio(disco, enderecoInodeAtual, nomeDiretorio, TIPO_ARQUIVO_ARQUIVO, false);
        if (isEnderecoValido(endereco))
        {
            if (disco[endereco].inode.protecao[0] == TIPO_ARQUIVO_LINK)
            {
                int enderecoInodeOrigem = enderecoInodeAtual;
                vector<string> caminhoOrigem = split(disco[disco[endereco].inode.enderecoDireto[0]].ls.caminho, '/');
                
                for(const auto& str : caminhoOrigem)
                {
                    enderecoInodeOrigem = cd(disco, enderecoInodeOrigem, str.c_str(), enderecoInodeRaiz, caminhoAbsoluto); 
                    // printf(" -- %s %d -- ", str.c_str(), enderecoInodeOrigem);
                }

                return enderecoInodeOrigem;
            }
            else
                caminhoAbsoluto.append("/" + nomeDiretorio);

            return endereco;
        }
        else
            return enderecoInodeAtual;
    }
}

bool touch(Disco disco[], int enderecoInodeAtual, char comando[])
{
    string comandoString(comando);
    int endereco = getEnderecoNull();
    vector<string> vetorStringSeparado = split(comandoString, ' ');

    if (vetorStringSeparado.size() == 2 && vetorStringSeparado.at(0).size() <= MAX_NOME_ARQUIVO)
    {
        char nomeArquivo[MAX_NOME_ARQUIVO];
        strcpy(nomeArquivo, vetorStringSeparado.at(0).c_str());
        int tamanhoArquivo = atoi(vetorStringSeparado.at(1).c_str());

        endereco = existeArquivoOuDiretorio(disco, enderecoInodeAtual, nomeArquivo);
        if (isEnderecoNull(endereco)) // ainda não tem nenhum arquivo criado com esse nome
        {
            // printf(" \n -- %d -- \n", getQuantidadeBlocosUsar(disco, ceil((float)tamanhoArquivo / (float)10)));
            int quantidadeBlocosLivres = getQuantidadeBlocosLivres(disco);
            // printf("\n - %d - \n", quantidadeBlocosLivres);
            int quantidadeBlocosUsados = getQuantidadeBlocosUsar(disco, ceil((float)tamanhoArquivo / (float)10));

            if (quantidadeBlocosLivres - quantidadeBlocosUsados >= 0)
                return isEnderecoValido(addDiretorioEArquivo(disco, TIPO_ARQUIVO_ARQUIVO, enderecoInodeAtual, nomeArquivo, tamanhoArquivo));
        }
    }

    return false;
}

void buscaBlocosLivresOcupados(Disco disco[], int &qtdBlocosLivres, int &qtdBlocosOcupados, int quantidadeBlocosTotais, int quantidadeBlocosNecessariosListaLivre)
{

    for (int i = 0; i < quantidadeBlocosTotais; i++)
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

        else if (strlen(disco[i].ls.caminho.c_str()) > 0)
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

void vi(Disco disco[], int enderecoInodeAtual, string nomeArquivo, string &enderecosUtilizados)
{
    int enderecoInodeArquivo = getEnderecoNull(), i;
    char enderecoToChar[100];
    bool primeiraVez = enderecosUtilizados.size() == 0;

    // primeira vez que entra
    if (primeiraVez)
        enderecoInodeArquivo = existeArquivoOuDiretorio(disco, enderecoInodeAtual, nomeArquivo, TIPO_ARQUIVO_ARQUIVO);
    else // demais extenções de inode
        enderecoInodeArquivo = enderecoInodeAtual;

    if (isEnderecoValido(enderecoInodeArquivo))
    {
        if (primeiraVez)
            printf("%s: ", nomeArquivo.c_str());

        itoa(enderecoInodeArquivo, enderecoToChar, 10);
        enderecosUtilizados.append(enderecoToChar).append("-");

        for (int direto = 0; direto < 5; direto++)
        {
            if (isEnderecoValido(disco[enderecoInodeArquivo].inode.enderecoDireto[direto]))
            {
                itoa(disco[enderecoInodeArquivo].inode.enderecoDireto[direto], enderecoToChar, 10);
                enderecosUtilizados.append(enderecoToChar).append("-");
            }
        }

        if (!isEnderecoNull(disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto))
        {
            itoa(disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto, enderecoToChar, 10);
            enderecosUtilizados.append(enderecoToChar).append("-");

            retornaEnderecosInodeIndiretoSimples(disco, disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto, enderecosUtilizados);
        }

        if (!isEnderecoNull(disco[enderecoInodeArquivo].inode.enderecoDuploIndireto))
        {
            itoa(disco[enderecoInodeArquivo].inode.enderecoDuploIndireto, enderecoToChar, 10);
            enderecosUtilizados.append(enderecoToChar).append("-");

            retornaEnderecosInodeIndiretoDuplo(disco, disco[enderecoInodeArquivo].inode.enderecoDuploIndireto, enderecosUtilizados);
        }

        if (!isEnderecoNull(disco[enderecoInodeArquivo].inode.enderecoTriploIndireto))
        {
            itoa(disco[enderecoInodeArquivo].inode.enderecoTriploIndireto, enderecoToChar, 10);
            enderecosUtilizados.append(enderecoToChar).append("-");

            retornaEnderecosInodeIndiretoTriplo(disco, disco[enderecoInodeArquivo].inode.enderecoTriploIndireto, enderecosUtilizados);
        }

        if (primeiraVez)
        {
            int qtdBlocos = 0;
            for (i = 0; i < enderecosUtilizados.size(); i++)
            {
                if (enderecosUtilizados.at(i) == '-')
                {
                    qtdBlocos++;
                }
            }
            printf("%s\n", enderecosUtilizados.substr(0, enderecosUtilizados.size() - 1).c_str());
            printf("Quantidade de enderecos: %d", qtdBlocos);
        }
    }
    else
    {
        if (primeiraVez)
            printf("Arquivo nao encontrado!");
    }

    if (primeiraVez)
        printf("\n");
}

bool rm(Disco disco[], int enderecoInodeAtual, string nomeArquivo, int primeiraVez = 1)
{
    int enderecoInodeArquivo, endereco;

    if (primeiraVez)
        enderecoInodeArquivo = existeArquivoOuDiretorio(disco, enderecoInodeAtual, nomeArquivo, TIPO_ARQUIVO_ARQUIVO);
    else // demais extenções de inode
        enderecoInodeArquivo = enderecoInodeAtual;

    if (isEnderecoValido(enderecoInodeArquivo))
    {
        for (int direto = 0; direto < 5; direto++)
        {
            if (isEnderecoValido(disco[enderecoInodeArquivo].inode.enderecoDireto[direto]))
            {
                initDisco(disco[disco[enderecoInodeArquivo].inode.enderecoDireto[direto]]);
                pushListaBlocoLivre(disco, disco[enderecoInodeArquivo].inode.enderecoDireto[direto]);
                disco[enderecoInodeArquivo].inode.enderecoDireto[direto] = getEnderecoNull();
            }
        }

        if (!isEnderecoNull(disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto))
        {
            removeEnderecosInodeIndiretoSimples(disco, disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto);

            initDisco(disco[disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto]);
            pushListaBlocoLivre(disco, disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto);

            disco[enderecoInodeArquivo].inode.enderecoSimplesIndireto = getEnderecoNull();
        }

        if (!isEnderecoNull(disco[enderecoInodeArquivo].inode.enderecoDuploIndireto))
        {
            removeEnderecosInodeIndiretoDuplo(disco, disco[enderecoInodeArquivo].inode.enderecoDuploIndireto);

            initDisco(disco[disco[enderecoInodeArquivo].inode.enderecoDuploIndireto]);
            pushListaBlocoLivre(disco, disco[enderecoInodeArquivo].inode.enderecoDuploIndireto);

            disco[enderecoInodeArquivo].inode.enderecoDuploIndireto = getEnderecoNull();
        }

        if (!isEnderecoNull(disco[enderecoInodeArquivo].inode.enderecoTriploIndireto))
        {
            removeEnderecosInodeIndiretoTriplo(disco, disco[enderecoInodeArquivo].inode.enderecoTriploIndireto);

            initDisco(disco[disco[enderecoInodeArquivo].inode.enderecoTriploIndireto]);
            pushListaBlocoLivre(disco, disco[enderecoInodeArquivo].inode.enderecoTriploIndireto);

            disco[enderecoInodeArquivo].inode.enderecoTriploIndireto = getEnderecoNull();
        }

        if (primeiraVez)
        {
            // remover da entrada do diretório o nome do arquivo.
            int enderecoEntradaDiretorio = buscaEnderecoEntradaDiretorioArquivo(disco, enderecoInodeAtual, nomeArquivo, TIPO_ARQUIVO_ARQUIVO);
            int pos = 0;

            while (pos < disco[enderecoEntradaDiretorio].diretorio.TL)
            {
                if (strcmp(disco[enderecoEntradaDiretorio].diretorio.arquivo[pos].nome, nomeArquivo.c_str()) == 0)
                    break;
                pos++;
            }

            for (int i = pos; i < disco[enderecoEntradaDiretorio].diretorio.TL; i++)
            {
                disco[enderecoEntradaDiretorio].diretorio.arquivo[i] = disco[enderecoEntradaDiretorio].diretorio.arquivo[i + 1];
            }

            disco[enderecoEntradaDiretorio].diretorio.TL--;

            initDisco(disco[enderecoInodeArquivo]);
            pushListaBlocoLivre(disco, enderecoInodeArquivo);

            return true;
        }
    }

    return false;
}

bool rmdir(Disco disco[], int enderecoInodeAtual, string nomeDiretorio, int &contadorDiretorio, int primeiraVez = 1)
{    
    int enderecoEntradaDiretorio, pos = 0;

    if (primeiraVez)        
        enderecoEntradaDiretorio = buscaEnderecoEntradaDiretorioArquivo(disco, enderecoInodeAtual, nomeDiretorio, TIPO_ARQUIVO_DIRETORIO);
    else
        enderecoEntradaDiretorio = enderecoInodeAtual;

    // printf("\n [%d] \n", enderecoEntradaDiretorio);
    if (isEnderecoValido(enderecoEntradaDiretorio))
    {
        int enderecoInodeDiretorio;
        if(primeiraVez)
        {
            while (pos < disco[enderecoEntradaDiretorio].diretorio.TL)
            {
                if (strcmp(disco[enderecoEntradaDiretorio].diretorio.arquivo[pos].nome, nomeDiretorio.c_str()) == 0)
                    break;
                pos++;
            }

            enderecoInodeDiretorio = disco[enderecoEntradaDiretorio].diretorio.arquivo[pos].enderecoINode;
        }
        else
            enderecoInodeDiretorio = enderecoInodeAtual;
        
        for (int i = 0; i < 5; i++)
        {
            if (isEnderecoValido(disco[enderecoInodeDiretorio].inode.enderecoDireto[i]))
            {
                contadorDiretorio += disco[disco[enderecoInodeDiretorio].inode.enderecoDireto[i]].diretorio.TL;
            }
        }

        if (isEnderecoValido(disco[enderecoInodeDiretorio].inode.enderecoSimplesIndireto))
            retornaQuantidadeDiretorioInodeIndiretoSimples(disco, disco[enderecoInodeDiretorio].inode.enderecoSimplesIndireto, contadorDiretorio);
        if (isEnderecoValido(disco[enderecoInodeDiretorio].inode.enderecoDuploIndireto))
            retornaQuantidadeDiretorioInodeIndiretoDuplo(disco, disco[enderecoInodeDiretorio].inode.enderecoDuploIndireto, contadorDiretorio);
        if (isEnderecoValido(disco[enderecoInodeDiretorio].inode.enderecoTriploIndireto))
            retornaQuantidadeDiretorioInodeIndiretoTriplo(disco, disco[enderecoInodeDiretorio].inode.enderecoTriploIndireto, contadorDiretorio);

        if(primeiraVez)
        {
            // printf("\n-- %d --\n", contadorDiretorio);
            if (contadorDiretorio <= 2)
            {
                if (isEnderecoValido(disco[enderecoInodeDiretorio].inode.enderecoDireto[0]))
                {
                    initDisco(disco[disco[enderecoInodeDiretorio].inode.enderecoDireto[0]]);
                    pushListaBlocoLivre(disco, disco[enderecoInodeDiretorio].inode.enderecoDireto[0]);
                    disco[enderecoInodeDiretorio].inode.enderecoDireto[0] = getEnderecoNull();
                }

                for (int i = pos; i < disco[enderecoEntradaDiretorio].diretorio.TL; i++)
                {
                    disco[enderecoEntradaDiretorio].diretorio.arquivo[i] = disco[enderecoEntradaDiretorio].diretorio.arquivo[i + 1];
                }

                disco[enderecoEntradaDiretorio].diretorio.TL--;

                initDisco(disco[enderecoInodeDiretorio]);
                pushListaBlocoLivre(disco, enderecoInodeDiretorio);

                return true;
            }
        }
    }

    return false;
}

void linkSimbolico(Disco disco[], int enderecoInodeAtual, string comando, int enderecoInodeRaiz)
{
    char caminhoDestinoChar[300];
    string caminhoExemplo;
    string caminhoAux;
    vector<string> caminhosOrigemDestino, caminhoOrigem, caminhoDestino;

    caminhosOrigemDestino = split(comando, ' ');

    if (caminhosOrigemDestino.size() == 2)
    {
        caminhoOrigem = split(caminhosOrigemDestino.at(0), '/');
        caminhoDestino = split(caminhosOrigemDestino.at(1), '/');

        strcpy(caminhoDestinoChar, caminhoDestino.at(0).c_str());

        caminhoAux = caminhosOrigemDestino.at(0);
        addDiretorioEArquivo(disco, TIPO_ARQUIVO_LINK, enderecoInodeAtual, caminhoDestinoChar, 1, caminhoAux);
    }
}

void linkFisico(Disco disco[], int enderecoInodeAtual, string comando, int enderecoInodeRaiz)
{
    int pos=0;
    char caminhoDestinoChar[300];
    string caminhoExemplo, nomeDiretorioOrigem, nomeDiretorioDestino;
    string caminhoAux;
    vector<string> caminhosOrigemDestino, caminhoOrigem, caminhoDestino;
    
    caminhosOrigemDestino = split(comando, ' ');

    if (caminhosOrigemDestino.size() == 2)
    {
        int enderecoInodeOrigem = enderecoInodeAtual, enderecoEntradaDiretorio,
        enderecoInodeDestino = enderecoInodeAtual;

        caminhoOrigem = split(caminhosOrigemDestino.at(0), '/');
        caminhoDestino = split(caminhosOrigemDestino.at(1), '/');

        
        for(const auto& str : caminhoOrigem)
        {
            enderecoInodeOrigem = cd(disco, enderecoInodeOrigem, str.c_str(), enderecoInodeRaiz, caminhoAux);
        }

        if (isEnderecoValido(enderecoInodeOrigem))
        {
            nomeDiretorioOrigem.assign(caminhoOrigem.at(caminhoOrigem.size()-1));

            enderecoEntradaDiretorio = buscaEnderecoEntradaDiretorioArquivo(disco, enderecoInodeOrigem, nomeDiretorioOrigem);
            
            while (pos < disco[enderecoEntradaDiretorio].diretorio.TL)
            {
                if (strcmp(disco[enderecoEntradaDiretorio].diretorio.arquivo[pos].nome, nomeDiretorioOrigem.c_str()) == 0)
                    break;
                pos++;
            }

            disco[disco[enderecoEntradaDiretorio].diretorio.arquivo[pos].enderecoINode].inode.contadorLinkFisico++;

            for(const auto& str : caminhoDestino)
            {
                enderecoInodeDestino = cd(disco, enderecoInodeDestino, str.c_str(), enderecoInodeRaiz, caminhoAux);
            }

            addDiretorioEArquivo(disco, disco[disco[enderecoEntradaDiretorio].diretorio.arquivo[pos].enderecoINode].inode.protecao[0], 
                                enderecoInodeDestino, caminhoDestinoChar, 1, caminhoAux, 
                                disco[enderecoEntradaDiretorio].diretorio.arquivo[pos].enderecoINode);
        }
    }
}