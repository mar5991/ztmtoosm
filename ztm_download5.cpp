#include "src/HafasBaza.hpp"
int main(int argc, char** argv)
{
	string sciezka, sciezka2, sciezka3;
	sciezka=argv[1];
	sciezka2=argv[2];
	sciezka3=argv[3];
	HafasBazaSQL loader(sciezka, sciezka2, sciezka3);
	//HafasBazaSQL2 loader2(lodaer.kalendarz, sciezka, sciezka2);
}
