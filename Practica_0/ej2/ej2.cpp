/* Convertir un numero en base 10 a un numero en cualquier base en el intervalo [2,16]*/

#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

/* funcion que convierte un numero al char correspondiente */

char reVal(int num)
{
    if (num >= 0 && num <= 9)
        return (char)(num + '0');
    else if (num >= 10 && num <= 15)
        return (char)(num - 10 + 'A');
    else
        return -1;
}

void strev(string &str)
{
    int len = str.size() - 1;
    int i;
    for (i = 0; i < len / 2; i++)
    {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

/* Función que cambia la base de un string*/
void fromDeci(string &res, int base, int inputNum)
{
    int index = 0;

    while (inputNum > 0)
    {
        res += reVal(inputNum % base);
        inputNum /= base;
    }
    res += '\0';

    strev(res);
}

void getAndConvert()
{
    int base;
    int num;

    while (true)
    {
        cout << "Ingrese el numero a convertir: ";
        cin >> num;

        cout << "Ingrese a que base quiere convertirlo: ";
        cin >> base;

        if (base > 16 || base < 1)
            cout << "Base entre 1 y 16 pa" << endl;
        else
        {
            string res;

            fromDeci(res, base, num);

            cout << "El numero en base " << base << " es " << res << endl;
            cout << endl;
        }
    }
}

int main()
{
    getAndConvert();
    return 0;
}