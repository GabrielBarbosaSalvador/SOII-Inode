# Kause Linux - Trabalho prático de SO II
Este trabalho foi concebido com o propósito de aprofundar a compreensão acerca do funcionamento do I-Node e da gestão de blocos de disco em sistemas operacionais Linux. A abordagem adotada envolve a exploração prática desses conceitos, além da implementação efetiva, ou próxima à realidade, utilizando as linguagens de programação C++ e C.

Este projeto prático tem como finalidade realizar a simulação referente a implementação do Sistema de 
Arquivos utilizado Inode.
Cada Inode possui 8 ponteiros, sendo os 5 primeiros para alocação direta, o 6º ponteiro para alocação 
simples indireta; o 7º para alocação dupla indireta e o 8º para alocação tripla-indireta. Cada inode ocupa 
um bloco em disco. Cada inode principal pois um área de atributos com os seguintes campos: data, hora, 
tamanho, permissões (10 bits, onde o primeiro pode ser – ou d ou l, seguido dos do usuários (u) RWX, 
seguido do grupo (g)RWX e dos outros (o)RWX), contador de links físicos, tipo do bloco;

Por meio de um prompt de comando o usuário poderá manipular o sistema de arquivo:
- chmod (+)(-)ugo RWX alterar as permissões de acesso a arquivos e diretórios
- vi nomeArquivo visualizar um arquivo regular
- ls listar os nomes dos arquivos no diretório
- ls -l listas os nomes dos arquivos com seus atributos
- mkdir NomeDir criar diretórios
- rmdir NomeDir deletar direitórios que estejam vazios
- rm NomeArq deletar aquivos
- cd NomeDir ou . ou .. navegar nos diretórios
- link –h nomeArquivoOrigem NomeArquivoDestino criar link fisico
- link –s nomeArquivoOrigem NomeArquivoDestino criar link simbólico
- unlink –h remover link físico
- unlink –s remover link simbólico
- bad numeroBloco transformar um bloco em Bad
- touch NomeAruivo TamanhoBytes Criar um arquivo Regular

