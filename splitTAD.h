#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

vector<string> split(string stringPrincipal, char delimitador=' ')
{
    istringstream stringStream(stringPrincipal);
    string stringAuxiliar;

    vector<string> retorno;

    while (getline(stringStream, stringAuxiliar, delimitador))
    {
        retorno.push_back(stringAuxiliar);
    }

    return retorno;
}

string lastPosition(vector<string> vector)
{
    return vector.at(vector.size()-1);
}

int ocorrenciaString(string conteudo, char caracterBuscado)
{
    int qtd=0;
    for(int i=0; i<conteudo.size(); i++)
    {
        if (caracterBuscado == conteudo.at(i))
            qtd++;
    }

    return qtd;
}

string stringToLower(string palavra){
    for (auto& x : palavra) { 
        x = tolower(x); 
    }
    return palavra;
}