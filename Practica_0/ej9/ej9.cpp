#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

int fill_rectangle(int n)
{
    if (n == 0)
        return 1;
    else if (n == 1)
        return 1;
    else if (n == 2)
        return 2;
    else
    {
        int m = 0;
        m += fill_rectangle(n - 1);
        m += fill_rectangle(n - 2);
        return m;
    }
}

int main()
{
    int n;
    while (true)
    {
        cout << "Ingrese N: ";
        cin >> n;
        cout << "Numero de formas: " << fill_rectangle(n) << endl;
    }
}