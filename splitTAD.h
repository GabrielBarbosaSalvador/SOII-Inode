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