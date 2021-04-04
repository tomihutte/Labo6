#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

int mcd(int a, int b)
{
    if (a == 0 || b == 0)
    {
        return a + b;
    }
    if (a > b)
    {
        return mcd(b, a % b);
    }
    if (b >= a)
    {
        return mcd(a, b % a);
    }
}


int main()
{
    int a,b;
    while(true)
    {
        cout << "Ingrese primer numero: ";
        cin >> a;
        cout << "Ingrese el segundo numero: ";
        cin >> b;
        cout << "El maximo común divisor es: " << mcd(a,b) << endl;
    }
}