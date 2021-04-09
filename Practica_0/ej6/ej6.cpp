#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

// sin usat la std (tanto)
int StrStr(string s1, string s2)
{
    bool occurence = false;
    for (int i = 0; i < s1.length(); i++)
    {
        if (s1[i] == s2[0])
        {
            occurence = true;
            for (int j = 1; j < s2.length(); j++)
            {
                if (i + j < s1.length())
                {
                    if (s1[i + j] != s2[j])
                        occurence = false;
                }
                else
                    occurence = false;
                if (!occurence)
                    break;
            }
        }
        if (occurence)
            return i;
    }
    return -1;
}

//usando la std
int StrStr2(string s1, string s2)
{
    bool occurence = false;
    for (int i = 0; i < s1.length(); i++)
    {
        if (s1[i] == s2[0])
        {
            if (s1.length() - s2.length() >= i)
            {
                if (s1.substr(i, s2.length()) == s2)
                    occurence = true;
            }
        }
        if (occurence)
            return i;
    }
    return -1;
}

int main()
{
    string s1, s2;
    while (true)
    {
        cout << "Ingrese s1: ";
        getline(cin, s1);
        cout << "Ingrese s2: ";
        getline(cin, s2);
        cout << "Ocurrencia en " << StrStr2(s1, s2) << endl;
    }
}