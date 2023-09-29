//https://www.infowester.com/linuxpermissoes.php
int permissaoToInt(char permissao[]){
    int chmodCod = 0;
    
    if (permissao[0] == 'r'){
        chmodCod += 4;
    }
    
    if (permissao[1] == 'w'){
        chmodCod += 2;
    }
    
    if (permissao[2] == 'x'){
        chmodCod += 1;
    }
    
    return chmodCod;
}
void convertPermissaoToString(int permissao, char retorno[], int posicaoInicial=0){
    // printf("%d %d %d\n", posicaoInicial, posicaoInicial+1, posicaoInicial+2);
    if (permissao - 4 >= 0){
        retorno[posicaoInicial] = 'r';
        permissao -= 4;
    }else{
        retorno[posicaoInicial] = '-';
    }
    
    if (permissao - 2 >= 0){
        retorno[posicaoInicial+1] = 'w';
        permissao -= 2;
    }else{
        retorno[posicaoInicial+1] = '-';
    }
    
    if (permissao - 1 >= 0){
        retorno[posicaoInicial+2] = 'x';
        permissao -= 1;
    }else{
        retorno[posicaoInicial+2] = '-';
    }

    retorno[posicaoInicial+3] = '\0';
    
}

void convertPermissaoUGOToString(int permissaoCompleta, char retorno[], int posicaoInicial=0){
    char permissaoCompletaChar[4];
    itoa(permissaoCompleta, permissaoCompletaChar, 10);

    int perm[3] = {
        permissaoCompletaChar[0] - '0',
        permissaoCompletaChar[1] - '0',
        permissaoCompletaChar[2] - '0'
    };

    // printf("%d %d %d", perm[0], perm[1], perm[2]);
    convertPermissaoToString(perm[0], retorno, posicaoInicial);
    convertPermissaoToString(perm[1], retorno, posicaoInicial+3);
    convertPermissaoToString(perm[2], retorno, posicaoInicial+6);

    retorno[posicaoInicial+9] = '\0';
}

int convertPermissaoUGOToInt(char permissaoCompleta[], int posicaoInicial=0){
    int permissao = 0;
    char permissaoIndividual[3];

    //usuário (7XX)
    strncpy(permissaoIndividual, permissaoCompleta+posicaoInicial, 3);
    permissao += permissaoToInt(permissaoIndividual) * 100;

    //grupo (X7X)
    strncpy(permissaoIndividual, permissaoCompleta+posicaoInicial+3, 3);
    permissao += permissaoToInt(permissaoIndividual) * 10;

    //outros (XX7)
    strncpy(permissaoIndividual, permissaoCompleta+posicaoInicial+6, 3);
    permissao += permissaoToInt(permissaoIndividual);

    return permissao;
}