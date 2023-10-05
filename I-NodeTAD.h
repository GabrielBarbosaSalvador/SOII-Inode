#include "Permissao.h"

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
    char dataCriacao[30];
    /*data de último acesso do arquivo*/
    char dataUltimoAcesso[30];
    /*data de última alteração do arquivo*/
    char dataUltimaAlteracao[30];

    /*Tamanho em bytes do arquivo apontado*/
    long long int tamanhoArquivo;

    /*Endereços de blocos que armazenam o conteúdo do arquivo( EH AS PARADA QUE APONTA ) */
    /*5 primeiros para alocação direta*/
    int enderecoDireto[5];
    /*6º ponteiro para alocação simples indireta*/
    int enderecoSimplesIndireto;
    /*7º para alocação dupla indireta*/
    int enderecoDuploIndireto;
    /*8º para alocação tripla-indireta*/
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

// vimos tbm, está ok
struct Arquivo
{
    int enderecoINode;
    char nome[MAX_NOME_ARQUIVO];
};
typedef struct Arquivo Arquivo;

// vimos com o siscoutto, o diretório está ok
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

struct exibicaoEndereco{
    int endereco;
    char info;
};


void initDisco(Disco &disco){
    disco.bad = 0;
    disco.diretorio.TL = 0;
    disco.inode.protecao[0] = '\0';
    disco.inodeIndireto.TL = 0;
    disco.lbl.topo = -1;
    disco.ls.caminho[0] = '\0';
}

int criarINode(Disco disco[], char tipoArquivo, char permissao[10], int tamanhoArquivo);

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

int getQuantidadeBlocosLivres(Disco disco[])
{
    int enderecoProxBloco = ENDERECO_CABECA_LISTA;
    int quantidadeBlocosLivres = -1;
    
    while (enderecoProxBloco != -1)
    {
        quantidadeBlocosLivres += disco[enderecoProxBloco].lbl.topo ;
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
    int enderecoBlocoAtual = ENDERECO_CABECA_LISTA;
    int enderecoProxBloco = disco[enderecoBlocoAtual].lbl.enderecoBlocoProx;

    if (!isEmptyListaBlocoLivre(disco[enderecoBlocoAtual].lbl)) // há elementos na lista
    {
        if (isEnderecoNull(enderecoProxBloco)) // se for igual a -1, significa que não há próximo bloco da lista
        {
            return disco[ENDERECO_CABECA_LISTA].lbl.endereco[disco[ENDERECO_CABECA_LISTA].lbl.topo--];
        }
        else
        {
            int enderecoBlocoAnterior = enderecoBlocoAtual;

            while (isEnderecoValido(enderecoProxBloco)) // significa que a lista contém mais de um bloco, então percorre até o último da lista
            {
                enderecoBlocoAnterior = enderecoBlocoAtual;
                enderecoBlocoAtual = enderecoProxBloco;
                enderecoProxBloco = disco[enderecoBlocoAtual].lbl.enderecoBlocoProx;
            }

            int enderecoLivre = disco[enderecoBlocoAtual].lbl.endereco[disco[enderecoBlocoAtual].lbl.topo--];

            // caso o bloco atual não tenha mais nenhum item após a remoção, altera o apontamento do anterior para -1
            if (isEmptyListaBlocoLivre(disco[enderecoBlocoAtual].lbl))
            {
                // bloco anterior para de apontar para o bloco atual, quebrando a lista
                setEnderecoNull(disco[enderecoBlocoAnterior].lbl.enderecoBlocoProx);
            }

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

        while(!isEnderecoNull(topo))
        {
            printf(" [ %d ] ", disco[enderecoProxBloco].lbl.endereco[topo--]);
        }

        printf("\n");
        enderecoProxBloco = disco[enderecoProxBloco].lbl.enderecoBlocoProx;
    }
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

void inserirInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto, int &quantidadeBlocosNecessarios, int enderecoInodePrincipal, int enderecosAlocados[], int &tlEnderecosAlocados,int InseridoPeloTriplo=0)
{
    int quantidadeBlocosUtilizados = 0;
    //verifica se a quantidade de endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO - InseridoPeloTriplo)
    {
        int inicio = disco[enderecoInodeIndireto].inodeIndireto.TL;
        //adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO - InseridoPeloTriplo)
        {
            disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = popListaBlocoLivre(disco);
            disco[enderecoInodeIndireto].inodeIndireto.TL++;
            
            enderecosAlocados[tlEnderecosAlocados++] = disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio];

            quantidadeBlocosUtilizados++;
            inicio++;
        }

    }

    //gera a quantidade ainda faltante.
    quantidadeBlocosNecessarios = quantidadeBlocosNecessarios - quantidadeBlocosUtilizados;

    if (InseridoPeloTriplo && quantidadeBlocosNecessarios > 0)
    {
        char permissao[10];
        for(int i=0; i<10; i++) {
            disco[enderecoInodePrincipal].inode.protecao[i];
        }

        strncpy(permissao, disco[enderecoInodePrincipal].inode.protecao + 1, 10);

        disco[enderecoInodeIndireto].inodeIndireto.endereco[disco[enderecoInodeIndireto].inodeIndireto.TL] = criarINode(disco, 
            disco[enderecoInodePrincipal].inode.protecao[0], 
            permissao,
            quantidadeBlocosNecessarios * 10
        );

        //conseguiu criar o inode
        if(isEnderecoValido(disco[enderecoInodeIndireto].inodeIndireto.endereco[disco[enderecoInodeIndireto].inodeIndireto.TL])){
            enderecosAlocados[tlEnderecosAlocados++] = disco[enderecoInodeIndireto].inodeIndireto.endereco[disco[enderecoInodeIndireto].inodeIndireto.TL];
            disco[enderecoInodeIndireto].inodeIndireto.TL++;
        }
    }
}

void inserirInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto, int &quantidadeBlocosNecessarios, int enderecoInodePrincipal, int enderecosAlocados[], int &tlEnderecosAlocados, int InseridoPeloTriplo=0)
{
    //verifica se a quantidade de endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO)
    {
        int inicio = disco[enderecoInodeIndireto].inodeIndireto.TL;
        //adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO)
        {
            if(isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
            {
                disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = criarINodeIndireto(disco);
            }

            inserirInodeIndiretoSimples(disco, 
                                        disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 
                                        quantidadeBlocosNecessarios,
                                        enderecoInodePrincipal,
                                        enderecosAlocados,
                                        tlEnderecosAlocados,
                                        InseridoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            
            disco[enderecoInodeIndireto].inodeIndireto.TL++;
            inicio++;
        }
    }
}

void inserirInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto, int &quantidadeBlocosNecessarios, int enderecoInodePrincipal, int enderecosAlocados[], int &tlEnderecosAlocados)
{
    //verifica se a quantidade de endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL < MAX_INODEINDIRETO)
    {
        int inicio = disco[enderecoInodeIndireto].inodeIndireto.TL;
        //adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < quantidadeBlocosNecessarios && inicio < MAX_INODEINDIRETO)
        {
            if(isEnderecoNull(disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio]))
            {
                disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio] = criarINodeIndireto(disco);
            }

            inserirInodeIndiretoDuplo(disco, 
                                        disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 
                                        quantidadeBlocosNecessarios,
                                        enderecoInodePrincipal,
                                        enderecosAlocados,
                                        tlEnderecosAlocados,
                                        1);
            
            disco[enderecoInodeIndireto].inodeIndireto.TL++;
            inicio++;
        }
    }
}

int criarINode(Disco disco[], char tipoArquivo, char permissao[10], int tamanhoArquivo = 1)
{
    int enderecosUtilizados[1000], tlEnderecosUtilizados=0;
    int quantidadeBlocosNecessarios = (int)ceil((double)tamanhoArquivo / (double)10);

    //quantidade de blocos necessárias é maior que a quantidade de blocos livres. Se for, não posso gerar o arquivo, então retorna "-1"
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
        strncpy(disco[enderecoInode].inode.protecao+1, permissao, 9);

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
            inserirInodeIndiretoSimples(disco, disco[enderecoInode].inode.enderecoSimplesIndireto, enderecoInode, quantidadeBlocosFaltantes, enderecosUtilizados, tlEnderecosUtilizados);
        }

        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInode].inode.enderecoDuploIndireto))
            {
                disco[enderecoInode].inode.enderecoDuploIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoDuplo(disco, disco[enderecoInode].inode.enderecoDuploIndireto, enderecoInode, quantidadeBlocosFaltantes, enderecosUtilizados, tlEnderecosUtilizados);
        }

        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInode].inode.enderecoTriploIndireto))
            {
                disco[enderecoInode].inode.enderecoTriploIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoTriplo(disco, disco[enderecoInode].inode.enderecoTriploIndireto, enderecoInode, quantidadeBlocosFaltantes, enderecosUtilizados, tlEnderecosUtilizados);
        }
        
    }

    // retorna endereço do inode criado
    return enderecoInode;
}

char isDirFull(Diretorio diretorio)
{
    return diretorio.TL == DIRETORIO_LIMITE_ARQUIVOS;
}

void addArquivoNoDiretorio(Disco disco[], int enderecoDiretorio, int enderecoInodeCriado, char nomeDiretorioArquivo[MAX_NOME_ARQUIVO]){
    
    disco[enderecoDiretorio].diretorio.arquivo[
        disco[enderecoDiretorio].diretorio.TL
    ].enderecoINode = enderecoInodeCriado;

    strcpy(disco[enderecoDiretorio].diretorio.arquivo[
        disco[enderecoDiretorio].diretorio.TL++
    ].nome, nomeDiretorioArquivo);
}

int criaDiretorio(Disco disco[], int enderecoInodeDiretorioAtual, char nomeDiretorioNovo[MAX_NOME_ARQUIVO])
{
    int enderecosUtilizados[1000], tlEnderecosUtilizados=0;

    bool consegueInserir = false;
    int i;
    int enderecoInodeDiretorio;
    char permissao[10];
    convertPermissaoToString(PERMISSAO_PADRAO_DIRETORIO, permissao, 0);
    //caso o tipo de arquivo criado for diretório, é necessário criar dois diretórios dentro dele, sendo:
    // "."(Aponta para o inode do diretório atual) e 
    // ".."(Aponta para o inode do diretório anterior)

    // strcpy(nomeDiretorioAtual, ".");
    // strcpy(nomeDiretorioAnterior, "..");

    // //criação do diretório "." - atual
    // addArquivoNoDiretorio(disco, enderecoInode, enderecoInode, nomeDiretorioAtual);

    // //criação do diretório ".." - pai
    // addDiretorio(disco, enderecoInodeDiretorioAtual, enderecoInode, nomeDiretorioAnterior);
    int enderecoDireto = disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[0];
    
    for(i = 1; i < 5 && isEnderecoValido(enderecoDireto) ; i++)
    {
        enderecoDireto = disco[enderecoInodeDiretorioAtual].inode.enderecoDireto[i];

        if(disco[enderecoDireto].diretorio.TL < 10)
        {
            consegueInserir = true;
        }
    }
    
    //pergunta se não conseguiu inserir no diretório e finalizou o while antes mesmo de atingir todos os endereços.
    if(!consegueInserir && i < 5) // significa que há blocos livres para ser preenchidos e o diretório está cheio.
    {
        enderecoInodeDiretorio = popListaBlocoLivre(disco);
    }
    else if(!consegueInserir)  //todos os blocos estão cheio, consequentemente deve verificar se pode adicionar nos blocos indiretos
    {
        int quantidadeBlocosFaltantes = 1;

        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInodeDiretorioAtual].inode.enderecoSimplesIndireto))
            {
                disco[enderecoInodeDiretorioAtual].inode.enderecoSimplesIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoSimples(disco, disco[enderecoInodeDiretorioAtual].inode.enderecoSimplesIndireto, enderecoInodeDiretorioAtual, quantidadeBlocosFaltantes, enderecosUtilizados, tlEnderecosUtilizados);
        }

        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInodeDiretorioAtual].inode.enderecoDuploIndireto))
            {
                disco[enderecoInodeDiretorioAtual].inode.enderecoDuploIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoDuplo(disco, disco[enderecoInodeDiretorioAtual].inode.enderecoDuploIndireto, enderecoInodeDiretorioAtual, quantidadeBlocosFaltantes, enderecosUtilizados, tlEnderecosUtilizados);
        }

        if (quantidadeBlocosFaltantes > 0)
        {
            if (isEnderecoNull(disco[enderecoInodeDiretorioAtual].inode.enderecoTriploIndireto))
            {
                disco[enderecoInodeDiretorioAtual].inode.enderecoTriploIndireto = criarINodeIndireto(disco);
            }
            inserirInodeIndiretoTriplo(disco, disco[enderecoInodeDiretorioAtual].inode.enderecoTriploIndireto, enderecoInodeDiretorioAtual, quantidadeBlocosFaltantes, enderecosUtilizados, tlEnderecosUtilizados);
        }

        //não conseguiu criar nenhum endereço
        if (tlEnderecosUtilizados == 0)
            return getEnderecoNull();

        enderecoInodeDiretorio = enderecosUtilizados[tlEnderecosUtilizados-1];
    }    
    else //identificou um bloco que pode ser utilizado para inserir o diretório
    {

    }
    //TODO - DESCOBRIR QUAL O BLOCO LIVRE PARA INSERIR O DIRETÓRIO NO DIRETÓRIO ATUAL



    return enderecoInodeDiretorio;
}

//insere um diretório ou arquivo dentro do diretório atual
char addDiretorioArquivo(Disco disco[], char tipoCriado,  char nomeDiretorio[MAX_NOME_ARQUIVO], int enderecoInodeDiretorioAtual, int enderecoInodeDiretorioAnterior, int enderecoDiretorio = -1, int criacaoDiretorioRaiz = 0)
{
    char nomeDiretorioAtual[MAX_NOME_ARQUIVO];
    char nomeDiretorioAnterior[MAX_NOME_ARQUIVO];
    char permissao[10];

    //se o tipo criado for diretório, estabelece a permissão padrão de criação de diretório
    if (tipoCriado == TIPO_ARQUIVO_DIRETORIO)
    {
        convertPermissaoToString(PERMISSAO_PADRAO_DIRETORIO, permissao, 0);
    }
    else
    {
        //se o tipo criado for arquivo, estabelece a permissão padrão de criação de arquivo
        convertPermissaoToString(PERMISSAO_PADRAO_ARQUIVO, permissao, 0);
    }









    // if (tipoCriado == TIPO_ARQUIVO_DIRETORIO)
    // {
    //     //caso o tipo de arquivo criado for diretório, é necessário criar dois diretórios dentro dele, sendo:
    //     // "."(Aponta para o inode do diretório atual) e 
    //     // ".."(Aponta para o inode do diretório anterior)

    //     strcpy(nomeDiretorioAtual, ".");
    //     strcpy(nomeDiretorioAnterior, "..");

    //     //criação do diretório "." - atual
    //     addDiretorio(disco, enderecoInode, enderecoInode, nomeDiretorioAtual);

    //     //criação do diretório ".." - pai
    //     addDiretorio(disco, enderecoInodeDiretorioAtual, enderecoInode, nomeDiretorioAnterior);
    // }
}


int criaDiretorioRaiz(Disco disco[])
{
    char permissao[10];
    char nomeArquivo[MAX_NOME_ARQUIVO];

    convertPermissaoUGOToString(PERMISSAO_PADRAO_DIRETORIO, permissao, 0);
    int enderecoInodeDiretorioRaiz = criarINode(disco, TIPO_ARQUIVO_DIRETORIO, permissao);
    //FIXME - AO ATIVAR O CRIAR DIRETÓRIO RAIZ, UM ENDEREÇO AUTOMATICAMENTE PARA DE SER EXIBIDO TRACE DISK
    strcpy(nomeArquivo, "");
    
    addDiretorioArquivo(disco, TIPO_ARQUIVO_DIRETORIO, nomeArquivo,  enderecoInodeDiretorioRaiz, disco[enderecoInodeDiretorioRaiz].inode.enderecoDireto[0], 1);

    return enderecoInodeDiretorioRaiz;
}

void addExibicaoEndereco(exibicaoEndereco enderecos[], int &TL, int endereco, char info){
    enderecos[TL].endereco = endereco;
    enderecos[TL++].info = info;
}

void quickSort(int ini, int fim, exibicaoEndereco vet[]) {
	int i = ini;
	int j = fim;
    int troca = true;
	
	// exibicaoEndereco exibEnd;
    int end;
    char info;
    
	while(i<j) {
        if (troca)
            while(i<j && vet[i].endereco <= vet[j].endereco)
                i++;
        else
            while(i<j && vet[j].endereco >= vet[i].endereco)
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

	if(ini < i-1)
		quickSort(ini, i-1, vet);
	
	if(j+1 < fim)
		quickSort(j+1, fim, vet);
}

void percorrerInodeIndiretoSimples(Disco disco[], int enderecoInodeIndireto,  exibicaoEndereco enderecos[], int &TL, int ChamadoPeloTriplo=0)
{
    
    //verifica se a quantidade de endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        //adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO - ChamadoPeloTriplo)
        {            
            addExibicaoEndereco(enderecos, TL, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 'A'); 
            inicio++;
        }
        
    }
}

void percorrerInodeIndiretoDuplo(Disco disco[], int enderecoInodeIndireto,  exibicaoEndereco enderecos[], int &TL, int ChamadoPeloTriplo=0)
{
    
    //verifica se a quantidade de endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        //adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {            
            addExibicaoEndereco(enderecos, TL, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 'I'); 
            percorrerInodeIndiretoSimples(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecos, TL, ChamadoPeloTriplo && inicio + 1 == MAX_INODEINDIRETO);
            inicio++;
        }
        
    }
}

void percorrerInodeIndiretoTriplo(Disco disco[], int enderecoInodeIndireto,  exibicaoEndereco enderecos[], int &TL)
{
    
    //verifica se a quantidade de endereços no inode indireto não está cheio
    if (disco[enderecoInodeIndireto].inodeIndireto.TL > 0)
    {
        int inicio = 0;
        //adiciona novos blocos enquanto a quantidade for menor que o necessário e que não esteja cheio
        while (inicio < disco[enderecoInodeIndireto].inodeIndireto.TL && inicio < MAX_INODEINDIRETO)
        {            
            addExibicaoEndereco(enderecos, TL, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], 'I'); 
            percorrerInodeIndiretoDuplo(disco, disco[enderecoInodeIndireto].inodeIndireto.endereco[inicio], enderecos, TL, 1);
            inicio++;
        }
        
    }
}

void exibirDisco(Disco disco[], int tamanhoDisco, int quantidadeBlocosNecessariosListaLivre)
{
    exibicaoEndereco enderecos[tamanhoDisco];
    int TLEnderecos = 0;
    char nomeBlocos[5];
    int quantidadeCasasTamanhoDisco = (int) trunc(log10(tamanhoDisco)) + 1;
    int quantidadeBlocosLivres = getQuantidadeBlocosLivres(disco);

    int linha=0;
    for(int i=0; i < tamanhoDisco; i++)
    {
        if(disco[i].bad){
            addExibicaoEndereco(enderecos, TLEnderecos, i, 'B');
        }
        // else if(disco[i].diretorio.TL > 0){
        //     addExibicaoEndereco(enderecos, TLEnderecos, i, 'A');
        // }
        else if(strlen(disco[i].inode.protecao) > 0)
        {
            // printf("%d - inode\n", i);
            //tem um i-node
            addExibicaoEndereco(enderecos, TLEnderecos, i, 'I');
            for(int j=0; j < 5 && !isEnderecoNull(disco[i].inode.enderecoDireto[j]); j++)
            {
                // printf("%d - arquivo\n", disco[i].inode.enderecoDireto[j]);
                if(!isEnderecoNull(disco[i].inode.enderecoDireto[j]))
                    addExibicaoEndereco(enderecos, TLEnderecos, disco[i].inode.enderecoDireto[j], 'A');
            }
            
            if(!isEnderecoNull(disco[i].inode.enderecoSimplesIndireto))
            {
                int enderecoIndireto = 0;
                enderecoIndireto = disco[i].inode.enderecoSimplesIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');
    
                percorrerInodeIndiretoSimples(disco, enderecoIndireto, enderecos, TLEnderecos);
            }

            if(!isEnderecoNull(disco[i].inode.enderecoDuploIndireto))
            {
                int enderecoIndireto = disco[i].inode.enderecoDuploIndireto;
                // addExibicaoEndereco(enderecos, TLEnderecos, enderecoIndireto, 'I');
    
                percorrerInodeIndiretoDuplo(disco, enderecoIndireto, enderecos, TLEnderecos);
            }

            if(!isEnderecoNull(disco[i].inode.enderecoTriploIndireto))
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
        else if(strlen(disco[i].ls.caminho) > 0)
        {
            addExibicaoEndereco(enderecos, TLEnderecos, i, 'L');
        }
        else if(disco[i].lbl.topo > -1 || (i >= ENDERECO_CABECA_LISTA && i < ENDERECO_CABECA_LISTA + quantidadeBlocosNecessariosListaLivre))
        {

            addExibicaoEndereco(enderecos, TLEnderecos, i, 'R');
        }
        else{
            // addExibicaoEndereco(enderecos, TLEnderecos, i, 'F');
        }
    }

    for(int i=ENDERECO_CABECA_LISTA; i<ENDERECO_CABECA_LISTA+quantidadeBlocosNecessariosListaLivre && disco[i].lbl.topo > -1; i++)
    {
        for(int j=0; j<=disco[i].lbl.topo; j++)
        {
            addExibicaoEndereco(enderecos, TLEnderecos, disco[i].lbl.endereco[j], 'F');
        }
    }

    quickSort(0, TLEnderecos-1, enderecos);

    int linhaTela = 1;
    int colunaTela = 1;
    
    for(int i=0; i < TLEnderecos; i++)
    {
        
        // for(int j=0; j<quantidadeCasasTamanhoDisco; j++)
        // {
        //     // printf("%d");
        // }
        
        printf("[%d %c]\t", enderecos[i].endereco, enderecos[i].info);
        
        if(linha==10)
        {
            printf("\n");
            linha=0;
        }
        linha++;
    }
}

void listDirectory(Disco disco[], int enderecoAtual, char option1[], char option2[])
{
    int linha=0;

    printf("teste: %d", enderecoAtual);
    printf("- %d -", disco[enderecoAtual].diretorio.TL);

    for (int i=0; i<disco[enderecoAtual].diretorio.TL; i++)
    {
        printf("%s\t", disco[enderecoAtual].diretorio.arquivo[i].nome);

        if(linha==10)
        {
            printf("\n");
            linha=0;
        }
        linha++;
    }
}