#include <math.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <new>

using namespace std;

class Persona
{
public:
    string apellido;
    string calle;
    int fecha[3];
    int nro;
    int codPostal;
    int dni;

    Persona() = default;

    Persona(string ap, string cal, int fech[3], int n, int cp, int d)
    {
        apellido = ap;
        calle = cal;
        for (int i = 0; i < 3; i++)
        {
            fecha[i] = fech[i];
        }
        nro = n;
        codPostal = cp;
        dni = d;
    }
};

ostream &operator<<(ostream &out, Persona const &persona)
{
    out << "Apellido: " << persona.apellido << endl
        << "Domicilio: " << persona.calle << " " << persona.nro << ", CP: " << persona.codPostal << endl
        << "DNI " << persona.dni << endl
        << "Fecha de nacimiento: ";

    for (int i = 0; i < 3; i++)
    {
        out << persona.fecha[i];
        if (i < 2)
        {
            out << "/";
        }
    }
    return out;
}

typedef int(FunCmp)(const Persona *, const Persona *);

int compare_lastname(const Persona *persona1, const Persona *persona2)
{
    string a = persona1->apellido;
    string b = persona2->apellido;
    if (a > b)
        return 1;
    else if (a == b)
        return 0;
    else
        return -1;
}

int compare_dni(const Persona *persona1, const Persona *persona2)
{
    int b = persona2->dni;
    int a = persona1->dni;
    if (a > b)
        return 1;
    else if (a == b)
        return 0;
    else
        return -1;
}

int compare_fecha(const Persona *persona1, const Persona *persona2)
{
    double a = (persona1->fecha[2] + (persona1->fecha[1] - 1) / 12.0 + persona1->fecha[0] / 372.0);
    double b = (persona2->fecha[2] + (persona2->fecha[1] - 1) / 12.0 + persona2->fecha[0] / 372.0);

    if (a > b)
        return 1;
    else if (a == b)
        return 0;
    else
        return -1;
}

int compare_CP_lastname(const Persona *persona1, const Persona *persona2)
{
    int a = persona1->codPostal;
    int b = persona2->codPostal;
    string a_a = persona1->apellido;
    string b_a = persona2->apellido;

    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
    {
        if (a_a > b_a)
            return 1;
        else if (a_a < b_a)
            return -1;
        else
            return 0;
    }
}

int compare_CP_dni(const Persona *persona1, const Persona *persona2)
{
    int a = persona1->codPostal;
    int b = persona2->codPostal;
    int a_d = persona1->dni;
    int b_d = persona2->dni;

    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
    {
        if (a_d > b_d)
            return 1;
        else if (a_d < b_d)
            return -1;
        else
            return 0;
    }
}

void ordenaPersonas(Persona *pPersonas, int cnt, FunCmp fc)
{
    for (int i = 0; i < cnt; i++)
    {
        for (int j = i + 1; j < cnt; j++)
        {
            if (fc(&pPersonas[i], &pPersonas[j]) == 1)
            {
                Persona aux = pPersonas[i];
                pPersonas[i] = pPersonas[j];
                pPersonas[j] = aux;
            }
        }
    }
}

int main()
{
    Persona *pPersonas = new Persona[3]; //(Persona *)malloc(sizeof(Persona) * 3);//
    string apellidos[3] = {"a", "b", "c"};
    string calle = "calle a";
    int nro = 10;
    int cpostals[3] = {6, 6, 6};
    int dni[3] = {3, 5, 4};
    int fechas[3][3] = {{1, 2, 1920}, {2, 1, 1920}, {1, 1, 1921}};

    for (int i = 0; i < 3; i++)
    {
        pPersonas[i] = Persona(apellidos[i], calle, fechas[i], nro, cpostals[i], dni[i]);
    }

    ordenaPersonas(pPersonas, 3, compare_CP_lastname);

    for (int i = 0; i < 3; i++)
    {
        cout << pPersonas[i] << endl
             << endl;
    }
}