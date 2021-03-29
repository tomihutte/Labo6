// retorna la cantidad de bits en 1 presentes en
// la representacion de ‘value’

#include "ej3_lib.h"
#include <iostream>
#include <stdio.h>
#include <string.h>

int numOnes(int value)
{
    int ret = 0;
    for (int i = 31; i >= 0; i--)
    {
        ret += ((value >> i) & 1);
    }
    return ret;
}

// devuelve el valor ‘value’, con el bit ‘bit’ en 1
int setBit(int value, int bit)
{
    int mask = 1 << bit;
    return value | mask;
}

// devuelve el valor ‘value’, con el bit ‘bit’ en 0
int resetBit(int value, int bit)
{
    int mask = 1 << bit;
    return value & ~mask;
}

// chequea si el bit ‘bit’ esta en 1
int testBit(int value, int bit)
{
    int mask = 1 << bit;
    return value & mask;
}

int main()
{
    int val;
    int bit;
    std::string aux;
    while (true)
    {

        std::cout << "Ingrese numero: ";
        std::cin >> val;
        std::cout << "Ingrese bit: ";
        std::cin >> bit;
        std::string bin = fromDeci(aux, 2, val);
        std::cout << "Numero en binario: " << bin << std::endl;
        int res = numOnes(val);
        std::cout << "Unos en binario: " << res << std::endl;
        int setVal = setBit(val, bit);
        std::string set = fromDeci(aux, 2, setVal);
        std::cout << "Con el bit en 1: " << set << " = " << setVal << std::endl;
        int resetVal = resetBit(val, bit);
        std::string reset = fromDeci(aux, 2, resetVal);
        std::cout
            << "Con el bit en 0: " << reset << " = " << resetVal << std::endl;
        bool test = testBit(val, bit);
        std::cout
            << "El bit esta en 1: " << test << std::endl;
    }
}