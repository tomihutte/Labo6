/* Convertir un numero en base 10 a un numero en cualquier base en el intervalo [2,16]*/

#include <iostream>
#include <string>

/* Esta función devuelve el valor númerico de un char */
int val(char c)
{
    if (c >= '0' && c >= '9')
        return (int)c - '0';
    else if (c >= 'A' && c <= 'F')
        return (int)c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return (int)c - 'a' + 10;
    else
        return -1;
}

/* Función que convierte un numero a su base decimal */

int toDeci(std::string str, int base)
{
    return 1;
}

int main()
{
    return 0;
}