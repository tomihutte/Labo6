#include <math.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

class Complex
{
public:
    double real;
    double imag;

    Complex(double x, double y)
    {
        real = x;
        imag = y;
    }

    void set(double x, double y)
    {
        real = x;
        imag = y;
    }

    Complex operator+(const Complex z)
    {
        Complex z3(real + z.real, imag + z.imag);
        return z3;
    }

    void operator+=(const Complex z)
    {
        real += z.real;
        imag += z.imag;
    }

    Complex operator-(const Complex z)
    {
        Complex z3(real - z.real, imag - z.imag);
        return z3;
    }

    void operator-=(const Complex z)
    {
        real -= z.real;
        imag -= z.imag;
    }

    Complex operator*(const Complex z)
    {
        Complex z3(real * z.real - imag * z.imag, real * z.imag + imag * z.real);
        return z3;
    }

    void operator*=(const Complex z)
    {
        double im_aux = imag, re_aux = real;
        real = re_aux * z.real - im_aux * z.imag;
        imag = re_aux * z.imag + im_aux * z.real;
    }

    Complex operator/(const Complex z)
    {
        double z3_real = (real * z.real + imag * z.imag) / (pow(z.real, 2) + pow(z.imag, 2));
        double z3_imag = (imag * z.real - real * z.imag) / (pow(z.real, 2) + pow(z.imag, 2));
        Complex z3(z3_real, z3_imag);
        return z3;
    }

    void operator/=(const Complex z)
    {
        double re_aux = (real * z.real + imag * z.imag) / (pow(z.real, 2) + pow(z.imag, 2));
        double im_aux = (imag * z.real - real * z.imag) / (pow(z.real, 2) + pow(z.imag, 2));
        real = re_aux;
        imag = im_aux;
    }
};

ostream &operator<<(ostream &out, Complex const &z)
{
    if (z.imag >= 0)
    {
        return out << z.real << "+" << z.imag << "i";
    }
    else
    {
        return out << z.real << z.imag << "i";
    }
}

int main()
{
    double r1, i1, r2, i2;
    while (true)
    {
        cout << "Z1 parte real: ";
        cin >> r1;
        cout << "Z1 parte im: ";
        cin >> i1;
        cout << "Z2 parte real: ";
        cin >> r2;
        cout << "Z2 parte im: ";
        cin >> i2;
        Complex z1(r1, i1), z2(r2, i2);
        cout << "Z1+Z2 = " << z1 + z2;
        z1 += z2;
        cout << "Z1+Z2 = " << z1;
        z1.set(r1, i1);
        cout
            << "Z1-Z2 = " << z1 - z2;
        z1 -= z2;
        cout << "Z1-Z2 = " << z1;
        z1.set(r1, i1);
        cout << "Z1/Z2 = " << z1 / z2;
        z1 /= z2;
        cout << "Z1/Z2 = " << z1;
        z1.set(r1, i1);
        cout << "Z1*Z2 = " << z1 * z2 << endl;
        z1 *= z2;
        cout << "Z1*Z2 = " << z1;
    }
}