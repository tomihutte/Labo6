#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

string remove_comments(string str)
{
    int index = 0;

    while (str.length() > index + 1)
    {
        bool closed_comm = false;
        for (int i = index; i < str.length(); i++)
        {
            index = i;
            if (str[i] == '(')
            {
                string str1 = str.substr(0, i);
                for (int j = i + 1; j <= str.length(); j++)
                {
                    char c = str[j];
                    if (str[j] == ')')
                    {
                        string str2 = str.substr(j + 1, str.length());
                        str = str1 + str2;
                        closed_comm = true;
                        break;
                    }
                }
                if (closed_comm)
                    break;
                else
                    str = str1;
            }
        }
    }
    return str;
}

int main()
{
    while (true)
    {
        string str, filtered;
        cout << "Ingrese el texto que desea filtrar" << endl;
        getline(cin, str);
        filtered = remove_comments(str);
        cout << "El texto filtrado es:" << endl;
        cout << filtered << endl;
    }
}
