#ifndef HTMLGEN
#define HTMLGEN
#include "../../include/HTMLGenerator/HTMLGenerator.hpp"
string htmlgen::zlozTabele(string tableclass, string lineclass, string cellclass, vector <vector <string> > tabela)
{
	string wynik;
	int s1=tabela.size();
	for(int i=0; i<s1; i++)
	{
		string wynik0;
		for(int j=0; j<tabela[i].size(); j++)
		{
			wynik0+=div(cellclass, "", tabela[i][j]);
		}
		wynik+=div(lineclass, "", wynik0);
	}
	return div(tableclass, "", wynik);
}
void htmlgen::zlozHtml(string content, string file_out, string css_file)
{
	fstream plik(file_out.c_str(), fstream::trunc | fstream::out);
	string wynik;
	wynik+="<!DOCTYPE html>\n<html lang=\"pl\">\n<head> <meta charset=\"utf8\">";
	wynik+="<link rel=\"stylesheet\" type=\"text/css\" href=\""+css_file+"\">";
	wynik+="</head>\n<body>\n";
	wynik+=content+"\n";
	wynik+="</body></html>";
	plik<<wynik<<endl;
	plik.close();
}
string htmlgen::div(string classname, string id, string info)
{
	string wynik;
	wynik+="<div class=\"";
	wynik+=classname;
	wynik+="\" id=\"";
	wynik+=id;
	wynik+="\">";
	wynik+=info;
	wynik+="</div>\n";
	return wynik;
}
string htmlgen::link(string link, string info)
{
	string wynik;
	wynik+="<a href=\"";
	wynik+=link;
	wynik+="\">";
	wynik+=info;
	wynik+="</a>";
	return wynik;
}
#endif