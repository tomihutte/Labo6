#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

// podria usar los metodos de los strings, pero creo que no es la idea

void left_trim(string &phrase)
{
    int i = 0;
    while (phrase[i] == ' ')
        i++;
    phrase = phrase.substr(i, phrase.length());
}

int main()
{
    string str;
    while (true)
    {
        cout << "Ingrese string: ";
        getline(cin, str);
        left_trim(str);
        cout << "Filtrado: " << str << endl;
    }
}