﻿#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <sstream>
#include <cstring>
#include "ztmosmpor.hpp"
#include "osm_base.hpp"
#include "HTMLHeadersRouteinfo.hpp"
#include "PrzegladanieCzyPrawidloweStareLinie.hpp"
#include "WlasciwosciLokalne.hpp"
#include "WypisywanieWspolrzednychTras.hpp"
#include "RaportPrzystanki.hpp"
using namespace std;

vector <string> miejscowosci(vector <przystanek> xyz)
{
	vector <string> wynik;
	string akt;
	for(int i=0; i<xyz.size(); i++)
	{
		if(xyz[i].miejscowosc!=akt)
		{
			wynik.push_back(xyz[i].miejscowosc);
			akt=xyz[i].miejscowosc;
		}
	}
	return wynik;
}


string znacznikLink(double lon, double lat)
{
	stringstream foo, foo2;
	foo<<"http://www.openstreetmap.org/?mlat=";
	foo<<lon;
	foo<<"&mlon=";
	foo<<lat;
	foo<<"&zoom=18";
	foo2<<"http://localhost:8111/load_and_zoom?left=";
	foo2<<lat-0.002;
	foo2<<"&right=";
	foo2<<lat+0.002;
	foo2<<"&top=";
	foo2<<lon+0.002;
	foo2<<"&bottom=";
	foo2<<lon-0.002;
	if(lon<1 || lat<1)
		return "";
	return htmlgen::div("plinki", "", (htmlgen::link(foo.str(), "X")+"</br>"+htmlgen::link(foo2.str(), "J")));
}



set <long long> podpieteRelacje(osm_base* roo, long long root)
{
	set <long long> wynik;
	relation akt=roo->relations[root];
	map <string, string> tags = akt.getTags();
	if(tags["type"]=="route_master" || tags["type"]=="route")
	{
		wynik.insert(root);
	}
	int s1=akt.members.size();
	for(int i=0; i<s1; i++)
	{
		if(akt.members[i].member_type==RELATION)
		{
			long long teraz_id=akt.members[i].member_id;
			if(roo->relations.find(teraz_id)!=roo->relations.end())
			{
				relation teraz=roo->relations[teraz_id];
				auto nowe = podpieteRelacje(roo, teraz_id);
				wynik.insert(nowe.begin(), nowe.end());
			}
		}
	}
	return wynik;
}

set <long long> dziwneRelacje(osm_base* roo, long long root)
{
	set <long long> wynik;
	set <long long> reszta = podpieteRelacje(roo, root);
	for(auto& it1 : roo->relations)
	{
		map<string, string> tagi = it1.second.getTags();
		if(tagi["route"]=="bus" || tagi["route"]=="tram")
		{
			if(reszta.find(it1.first)==reszta.end())
			{
				wynik.insert(it1.first);
			}
		}
	}
	return wynik;
}






string zeraWiodace (string jeden)
{
	string wynik;
	for(int i=0; i<(10-jeden.length()); i++)
	{
		wynik+="0";
	}
	wynik+=jeden;
	return wynik;
}

struct SpecialSortedString
{
	string str;
	bool operator < (const SpecialSortedString& rhs) const
	{
		string l=str;
		string r=rhs.str;
		return zeraWiodace(l)<zeraWiodace(r);
	}
	bool operator == (const SpecialSortedString& rhs) const
	{
		string l=str;
		string r=rhs.str;
		return l==r;
	}
	static set <SpecialSortedString> convertSet(set<string> normal)
	{
		set <SpecialSortedString> wynik;
		for(auto it1 : normal)
		{
			SpecialSortedString foo;
			foo.str=it1;
			wynik.insert(foo);
		}
		return wynik;
	}
	static set <string> convertToNormal(set<SpecialSortedString> extra)
	{
		set <string> wynik;
		for(auto it1 : extra)
		{
			wynik.insert(it1.str);
		}
		return wynik;
	}
};

struct MainClass
{
	WlasciwosciLokalne* wlasciwosci;
	osm_base* bazaOsm;
	ScheduleHandlerInternal* bazaZtm;
	string miasto;
	string pathHTML;
	string pathTemplate;
	string ztmBaseFreshTime;
	set <string> linieDoPrzerobienia;
	map <string, OsmStopData> osmStopData;
	set <long long> changeNodes;
	set <long long> changeWays;
	void readArg(char** argv)
	{
		string ztmBasePath=argv[1];
		string osmBasePath=argv[2];
		miasto=argv[3];
		pathHTML=argv[4];
		pathTemplate="/ztmtoosm/templates-www";
		if(miasto=="Warszawa")
			wlasciwosci = new WlasciwosciLokalneWarszawa();
		if(miasto=="Szczecin")
			wlasciwosci = new WlasciwosciLokalneSzczecin();
		if(miasto=="Gdańsk")
			wlasciwosci = new WlasciwosciLokalneGdansk();
		if(miasto=="Łódź")
			wlasciwosci = new WlasciwosciLokalneLodz();
		bazaOsm = new osm_base(osmBasePath);
		cerr<<"BAZA OSM - OK"<<endl;
		osmStopData = loadOsmStopData(bazaOsm, wlasciwosci->getRefKey());
		cerr<<"OSM-DATA - OK"<<endl;
		bazaZtm = new ScheduleHandlerInternal (ztmBasePath, miasto);
		cerr<<"Załadowano dane "<<bazaZtm<<endl;
	}

	string jsonEnc (string k)
	{
		string wynik;
		for(int i=0; i<k.length(); i++)
		{
			if(wynik[i]=='"')
			{
				wynik+="\\";
			}
			wynik+=k[i];
		}
		return wynik;
	}
	void addTag(int i, string key, string value, fstream& plik)
	{
		if(i>0)
			plik<<",";
		plik<<"{\"key\":\""<<jsonEnc(key)<<"\",\"value\":\""<<jsonEnc(value)<<"\"}";
	}
	void addMember(int i, string category, long long id, string role, fstream& plik)
	{
		if(i>0)
			plik<<",";
		plik<<"{\"category\":\""<<category<<"\",\"id\":\""<<id<<"\",\"role\":\""<<role<<"\"}";
	}
	void addTags(map<string, string> k, fstream& plik)
	{
		plik<<"\"tags\":[";
		auto it1 = k.begin();
		while(it1!=k.end())
		{
			int id = 1;
			if(it1 == k.begin())
				id = 0;
			addTag(id, it1->first, it1->second, plik);
			it1++;
		}
		plik<<"],"<<endl;
	}
	string otoczeniePrzystanku(string idPrzystanek, string idLinia, int idWariantu, int idKol)
	{
		string poprzedni = "POCZĄTKOWY";
		string kolejny = "KOŃCOWY";
		if(idKol>0)
		{
			string poprzedniId = bazaZtm->dane_linia[idLinia][idWariantu][idKol-1];
			poprzedni = bazaZtm->przystanki[poprzedniId].name;
		}
		if(bazaZtm->dane_linia[idLinia][idWariantu].size()>idKol+1)
		{
			string kolejnyId = bazaZtm->dane_linia[idLinia][idWariantu][idKol+1];
			kolejny = bazaZtm->przystanki[kolejnyId].name;
		}
		string ostatniId = bazaZtm->dane_linia[idLinia][idWariantu][bazaZtm->dane_linia[idLinia][idWariantu].size()-1];
		string ostatni = bazaZtm->przystanki[ostatniId].name;
		string info = idLinia + " -> "+ostatni+" poprzedni: "+poprzedni+" kolejny: "+kolejny;
		return htmlgen::div("otoczenieliniaprzystanek", "", info);
	}
	string wypiszBlednyPrzystanek(string it1)
	{
		string info = it1 + " " + bazaZtm->przystanki[it1].name+" ";
		info += " "+htmlgen::div("stopinfo", "", bazaZtm->przystanki[it1].stopinfo);
		info += " "+htmlgen::div("znaczniklink", "", znacznikLink(bazaZtm->przystanki[it1].lat, bazaZtm->przystanki[it1].lon));
		for(auto& it2 : bazaZtm->dane_linia)
		{
			for(int i=0; i<it2.second.size(); i++)
			{
				for(int j=0; j<it2.second[i].size(); j++)
				{
					if(it2.second[i][j]==it1)
						info+=otoczeniePrzystanku(it1, it2.first, i, j);
				}
			}
		}
		return info;
	}
	string wypiszPrzystanekDoUsuniecia(string id, string name, double lon, double lat)
	{
		string info = id + " " + name+" ";
		info += " "+htmlgen::div("znaczniklink", "", znacznikLink(lat, lon));
		return info;
	}
	void generujLinie(string nazwa, bool newVersion = false)
	{
		cout<<"GENEROWANIE "<<nazwa<<endl;
		vector <long long> stareRelacje=relacje_linia(bazaOsm, wlasciwosci->getRootRelation(), nazwa).second;
		long long stareId=relacje_linia(bazaOsm, wlasciwosci->getRootRelation(), nazwa).first;
		if(stareId==0)
			stareId=-1;
		string nazwaGEN;
		nazwaGEN=pathHTML+"/js"+wlasciwosci->getNazwaMiasta()+nazwa+".json";
		if(newVersion)
		{
			nazwaGEN=pathHTML+"/jsNew"+wlasciwosci->getNazwaMiasta()+nazwa+".json";
		}
		fstream plik(nazwaGEN.c_str(), ios::out | ios::trunc);
		vector <long long> noweRelacje;
		int s1 = bazaZtm -> dane_linia[nazwa].size();
		plik<<"[";
		for(int i=0; i<s1; i++)
		{
			cout<<"s1 "<<nazwa<<endl;
			vector <string> wariant = bazaZtm -> dane_linia[nazwa][i];
			cout<<"s1.5 "<<nazwa<<" "<<wariant.size()<<endl;
			cout<<"s1.5 "<<osmStopData[wariant[0]].name<<endl;
			string from = wlasciwosci->substituteWhiteCharsBySpace(osmStopData[wariant[0]].name);
			cout<<"s1.8 "<<nazwa<<endl;
			string to = wlasciwosci->substituteWhiteCharsBySpace(osmStopData[wariant[wariant.size()-1]].name);
			cout<<"s2"<<endl;
			int wariantOsmRelId = i-100;
			if(i<stareRelacje.size())
				wariantOsmRelId = stareRelacje[i];
			map <string, string> tags;
			if(i>0)
				plik<<",";
			plik<<"{ \"id\":"<<wariantOsmRelId<<","<<endl;
			plik<<"\"track_type\":\""<<wlasciwosci->nazwaMala(nazwa)<<"\",\"parentrel\":[],"<<endl;
			cout<<"s3"<<endl;
			tags["ref"]=nazwa;
			tags["type"]="route";
			tags["network"]=wlasciwosci->getNetworkName();
			tags["route"]=wlasciwosci->nazwaMala(nazwa);
			tags["from"]=from;
			tags["to"]=to;
			tags["name"]=wlasciwosci->nazwaDuza(nazwa)+" "+nazwa+": "+from+" => "+to;
			tags["public_transport:version"] = "2";
			tags["source"]="Rozkład jazdy "+wlasciwosci->getNetworkName()+", trasa wygenerowana przez bot";
			/*vector <string> miasta = miejscowosci(bazaZtm->dane_linia[nazwa][i];
			string via;
			for(int i=1; i<(miasta.size()-1); i++)
			{
				via+=miasta[i];
				if(i<(miasta.size()-2))
					via+=", ";
			}*/
			/*if(via!="")
			{
				tags["via"]=via;
			}*/
			addTags(tags, plik);
			cout<<"s4"<<endl;
			plik<<"\"track\":[";
			for(int j=0; j<wariant.size(); j++)
			{
				if(j>0)
					plik<<",";
				cout<<wariant[j]<<endl;
				plik<<osmStopData[wariant[j]].stop_position;
			}
			plik<<"]";
			plik<<",\"members\":[";
			for(int j=0; j<wariant.size(); j++)
			{
			cout<<"XI1"<<endl;
				string dopisek="";
				if(j==0)
					dopisek="_entry_only";
				if(j==wariant.size()-1)
					dopisek="_exit_only";
				if(osmStopData[wariant[j]].bus_stop!=0)
					addMember(j, "N", osmStopData[wariant[j]].bus_stop, "stop"+dopisek, plik);
				else if(osmStopData[wariant[j]].stop_position!=0)
					addMember(j, "N", osmStopData[wariant[j]].stop_position, "stop"+dopisek, plik);
				if(osmStopData[wariant[j]].platform!=0)
				{
					string type="";
					type+=osmStopData[wariant[j]].platform_type;
					addMember(1, type, osmStopData[wariant[j]].platform, "platform"+dopisek, plik);
				}
				cout<<"XI2"<<endl;
			}
			plik<<"]}";
			plik.close();
			noweRelacje.push_back(wariantOsmRelId);
		}
		cout<<"pppp"<<endl;
		if(s1<stareRelacje.size())
		{
			for(int i=s1; i<stareRelacje.size(); i++)
			{
				plik<<",{\"track\":[],\"members\":[], \"id\":"<<stareRelacje[i]<<",\"tags\":[], \"todelete\":true}";
			}
		}
		cout<<"qqqqqqq"<<endl;
		map <string, string> tags;
		tags["type"] = "route_master";
		tags["ref"] = nazwa;
		tags["source"]="Rozkład jazdy "+wlasciwosci->getNetworkName()+", trasa wygenerowana przez bot";
		tags["name"] = wlasciwosci->nazwaDuza(nazwa)+" "+nazwa;
		tags["type"] = "route_master";
		tags["url"] = wlasciwosci->getUrlLine(nazwa);
		tags["route_master"] = wlasciwosci->nazwaMala(nazwa);
		tags["network"] = wlasciwosci->getNetworkName();
		tags["public_transport:version"] = "2";
		plik<<",{\"id\":"<<stareId<<",\"parentrel\":["<<wlasciwosci->getParentRelation(nazwa)<<"],"<<endl;
		addTags(tags, plik);
		plik<<"\"track\":[],\"members\":[";
		for(int i=0; i<noweRelacje.size(); i++)
		{
			addMember(i, "R", noweRelacje[i], "", plik);
		}
		plik<<"]}]";
	}

	map <string, string> infoHTML;
	string attention(string line)
	{
		return htmlgen::div("attention", "attention"+line, "");
	}
	string ok_route(string cos)
	{
		return htmlgen::div("ok_route", cos, "Trasa "+cos+" wygenerowana, pokaż...");
	}
	string oklink(string linia)
	{
		return "<a href=\"wyszukiwarka2.html?linia="+miasto+linia+"\" target=\"_blank\">Pokaż wygenerowany zestaw</a>"+"</br><a href=\"wyszukiwarka2.html?x=d&linia="+miasto+linia+"\" target=\"_blank\">Pokaż wygenerowany zestaw - routing testowy</a>";
	}

	void dodajLinieDoHTML(fstream& plik, int typ, string name, string content, HtmlExtraGenerator& gen)
	{
		cout<<"LOADER START"<<endl;
		if(typ==2)
			gen.loadedVariables[0]="panel-success panel-linia";
		if(typ==1)
			gen.loadedVariables[0]="panel-warning panel-linia";
		if(typ==0)
			gen.loadedVariables[0]="panel-info panel-linia";
		gen.loadedVariables[1]=name;
		gen.loadedVariables[2]=content;
		plik<<gen.loadTemplate(pathTemplate+"/lineHeader.template");
		cout<<"LOADER STOP"<<endl;
	}
	string dodajInfoNiewygenerowane(set <string> errPrzyst, string linia, HtmlExtraGenerator& gen)
	{
		stringstream foo1;
		foo1<<errPrzyst.size();
		string message = "Brak stop_position dla przystanków: ";
		int licznik = 0;
		for(auto it1 : errPrzyst)
		{
			if(licznik>0)
				message+=", ";
			message+=htmlgen::link(miasto+".html#"+it1, bazaZtm->przystanki[it1].name+" ("+it1+")", "");
			licznik++;
		}
		gen.loadedVariables[0]=linia+"niewygenerowane";
		gen.loadedVariables[1]=message;
		gen.loadedVariables[2]=foo1.str();
		return gen.loadTemplate(pathTemplate+"/errLine.template");
	}
	string dodajInfoRoznice(set <string> ein, set <string> zwei, bool brakRelacji, string linia, HtmlExtraGenerator& gen)
	{
		stringstream foo1;
		foo1<<ein.size()+zwei.size()+(int)(brakRelacji);
		string message = "Przystanki na których nie zatrzymuje się linia, ale OSM twierdzi inaczej: ";
		int licznik = 0;
		if(ein.size()==0)
			message+="brak";
		for(auto it1 : ein)
		{
			if(licznik>0)
				message+=", ";
			message+=htmlgen::link(miasto+".html#"+it1, osmStopData[it1].name+" ("+it1+")", "");
			licznik++;
		}
		message += ". Przystanki na których zatrzymuje się linia nieuwzględnione w OSM: ";
		licznik = 0;
		for(auto it1 : zwei)
		{
			if(licznik>0)
				message+=", ";
			message+=htmlgen::link(miasto+".html#"+it1, bazaZtm->przystanki[it1].name+" ("+it1+")", "");
			licznik++;
		}
		if(zwei.size()==0)
			message+="brak";
		message+=". ";
		if(brakRelacji)
			message += "Niespójność/brak relacji.";
		gen.loadedVariables[0]=linia+"roznice";
		gen.loadedVariables[1]=message;
		gen.loadedVariables[2]=foo1.str();
		return gen.loadTemplate(pathTemplate+"/diffLine.template");
	}
	string dodajInfoNormalne(pair <long long, vector <long long> > daneLinia, string linia, HtmlExtraGenerator& gen, set<long long>& niespojne)
	{
		stringstream messageStream;
		messageStream<<"<li class=\"list-group-item\">";
		messageStream<<"route_master: ";
		if(daneLinia.first!=0)
			messageStream<<htmlgen::link("http://openstreetmap.org/relation/"+toXstring(daneLinia.first), toXstring(daneLinia.first), "");
		else
			messageStream<<" <span class=\"label label-danger\">BRAK</span>";
		messageStream<<"</li>";
		for(int i=0; i<daneLinia.second.size(); i++)
		{
			messageStream<<"<li class=\"list-group-item\">";
			messageStream<<"route: ";
			messageStream<<htmlgen::link("http://openstreetmap.org/relation/"+toXstring(daneLinia.second[i]), toXstring(daneLinia.second[i]), "");
			if(niespojne.find(daneLinia.second[i])!=niespojne.end())
			{
				messageStream<<" <span class=\"label label-danger\">NIESPÓJNA</span>";
			}
			messageStream<<" <a class=\"label label-info\" href=\""<<"http://analyser.openstreetmap.fr/cgi-bin/index.py?relation="<<daneLinia.second[i]<<"\">RA</a>";
			messageStream<<"</li>";
		}
		gen.loadedVariables[0]=linia+"info";
		gen.loadedVariables[1]=messageStream.str();
		return gen.loadTemplate(pathTemplate+"/infoLine.template");
	}
	string dodajProgress(int val1, int val2, int val3, HtmlExtraGenerator& gen)
	{
		double suma = val1+val2+val3;
		double vall1 = val1;
		double vall2 = val2;
		double vall3 = val3;
		vall1/=suma/100.0;
		vall2/=suma/100.0;
		vall3/=suma/100.0;
		gen.loadedVariables[0]=toXstring(vall1);
		gen.loadedVariables[1]=toXstring(vall2);
		gen.loadedVariables[2]=toXstring(vall3);
		return gen.loadTemplate(pathTemplate+"/progress.template");
	}
	string aktTime()
	{
		char buff[20];
		time_t now = time(NULL);
		strftime(buff, 20, "%d.%m.%Y %H:%M", localtime(&now));
		string buff2=buff;
		return buff2;
	}

	string testBadStops()
	{
		stringstream wynik;
		for(auto& ddd : osmStopData)
		{
			OsmStopData& dataOsm = ddd.second;
			string id = ddd.first;
			if(bazaZtm->przystanki.find(id)==bazaZtm->przystanki.end())
			{
				if(id.length()==6 && (id[4]=='5' || id[4]=='6'))
				{
					double lat = 0;
					double lon = 0;
					if(bazaOsm->nodes.find(dataOsm.stop_position)!=bazaOsm->nodes.end())
					{
						lat = bazaOsm->nodes[dataOsm.stop_position].lat;
						lon = bazaOsm->nodes[dataOsm.stop_position].lon;
					}
					else if(bazaOsm->nodes.find(dataOsm.bus_stop)!=bazaOsm->nodes.end())
					{
						lat = bazaOsm->nodes[dataOsm.bus_stop].lat;
						lon = bazaOsm->nodes[dataOsm.bus_stop].lon;
					}
					wynik<<htmlgen::div("bprzyst", "", wypiszPrzystanekDoUsuniecia(id, dataOsm.name, lon, lat))<<endl;
				}
			}
		}
		return wynik.str();
	}


	string divOsmTable(vector <string> elem)
	{
		stringstream foo;
		foo<<"<table class=\"table table-striped\">";
		for(int i=0; i<elem.size(); i++)
		{
			foo<<elem[i];
		}
		foo<<"</table>";
		return foo.str();
	}

	string divOsmLink(long long id, char t)
	{
		string type;
		if(t=='N')
			type="node";
		if(t=='W')
			type="way";
		if(t=='R')
			type="relaton";
		if(id==0)
			return htmlgen::div("komorka", "", "-");
		stringstream foo;
		foo<<"<a href=\"http://openstreetmap.org/"<<type<<"/"<<id<<"\">"<<type<<" "<<id<<"</a>";
		return htmlgen::div("komorka", "", foo.str());
	}



	void uzupelnijPelne(fstream& lineHTMLStream, HtmlExtraGenerator& htmlGenerator)
	{
		lineHTMLStream.precision(9);
		uzupelnij(lineHTMLStream, pathTemplate+"/theme.template");
		lineHTMLStream<<miasto<<" - linie";
		htmlGenerator.loadedVariables[0]=miasto+".html";
		htmlGenerator.loadedVariables[1]="Zestawienie przystanków";
		lineHTMLStream<<htmlGenerator.loadTemplate(pathTemplate+"/themeA.template");

		lineHTMLStream<<"Stan na: ";
		lineHTMLStream<<aktTime();
		uzupelnij(lineHTMLStream, pathTemplate+"/themeB.template");
	}





	MainClass(char** argv)
	{
		HtmlExtraGenerator htmlGenerator;
		readArg(argv);
		linieDoPrzerobienia=wlasciwosci->wszystkieLinie(bazaZtm);
		PrzegladanieCzyPrawidloweStareLinie przeglStare(bazaOsm, bazaZtm, linieDoPrzerobienia, wlasciwosci->getRootRelation(), wlasciwosci->getRefKey());
		set <string> nieprawidlowe=przeglStare.nieprawidlowe;
		PrzegladanieCzyPrawidloweNoweLinie przeglNowe(&osmStopData, bazaZtm, nieprawidlowe);
		linieDoPrzerobienia=przeglNowe.getPrawidlowe();


		string linieHTMLPath=pathHTML+"/Pelne"+miasto+"bis.html";
		fstream lineHTMLStream(linieHTMLPath.c_str(), ios::out | ios::trunc);
		uzupelnijPelne(lineHTMLStream, htmlGenerator);




		string przystankiHTMLPath=pathHTML+"/"+miasto+"bis.html";
		string jsonPath=pathHTML+"/List"+miasto+".json";
		string json2Path=pathHTML+"/Przystanki"+miasto+".json";
		string wspRoutePath=pathHTML+"/Trasy"+miasto+".txt";
		//string wspStopPath=pathHTML+"/Wsp"+miasto+".txt";

		//fstream wspStream(wspStopPath.c_str(), ios::out | ios::trunc);
		fstream przystankiHTMLStream(przystankiHTMLPath.c_str(), ios::out | ios::trunc);
		fstream jsonStream(jsonPath.c_str(), ios::out | ios::trunc);
		fstream json2Stream(json2Path.c_str(), ios::out | ios::trunc);

		json2Stream.precision(9);
		RaportPrzystanki rrr(json2Stream, osmStopData, bazaZtm, miasto, bazaOsm);






		przystankiHTMLStream.precision(9);
		uzupelnij(przystankiHTMLStream, pathTemplate+"/theme.template");
		przystankiHTMLStream<<miasto<<" - przystanki";

		htmlGenerator.loadedVariables[0]="Pelne"+miasto+".html";
		htmlGenerator.loadedVariables[1]="Zestawienie linii";
		przystankiHTMLStream<<htmlGenerator.loadTemplate(pathTemplate+"/themeA.template");
		przystankiHTMLStream<<"Stan na: ";
		przystankiHTMLStream<<aktTime();
		uzupelnij(przystankiHTMLStream, pathTemplate+"/themeB.template");


		/*
		stringstream p5_tmp, p6_tmp, p7_tmp;
		for(string it1 : blednePrzystanki)
		{
			p5_tmp<<htmlgen::div("bprzyst", "", wypiszBlednyPrzystanek(it1))<<endl;
		}
		plik5<<htmlgen::div("blstops", "", p5_tmp.str())<<endl;
		plik5<<htmlgen::div("partx", "", "Linie do usunięcia")<<endl;
		*/

		/*
		plik5<<htmlgen::div("partx", "", "Inne relacje komunikacji w bazie OSM")<<endl;
		auto dziwne = dziwneRelacje(bazaOsm, wlasciwosci->getRootRelation());
		for(auto& it1 : dziwne)
		{
			stringstream wyn, wyn0;
			wyn0<<it1;
			wyn<<htmlgen::div("dziwna_linia_id", "", wyn0.str());
			auto tags = bazaOsm->relations[it1].getTags();
			for(auto& it2 : tags)
			{
				wyn<<htmlgen::div("dziwna_linia_dup", "", it2.first+" = "+it2.second);
			}
			p7_tmp<<htmlgen::div("dziwna_linia", "", wyn.str())<<"</br>";
		}

		cout<<"AAA-END"<<endl;
		plik5<<htmlgen::div("dziwne", "", p7_tmp.str())<<endl;
		plik5<<htmlgen::div("partx", "", "Trasy wygenerowane...")<<endl;
		*/
		lineHTMLStream<<dodajProgress(linieDoPrzerobienia.size(), przeglNowe.getNieprawidlowe().size(), przeglStare.prawidlowe.size(), htmlGenerator)<<endl;

		auto doUsuniecia = linieDoUsuniecia(bazaZtm, bazaOsm, wlasciwosci->getRootRelation());
		if(doUsuniecia.size()>0)
		{
			stringstream p6_tmp;
			lineHTMLStream<<"<h2>Linie do usunięcia <span class=\"badge\">"<<doUsuniecia.size()<<"</span></h2>";
			for(auto& it1 : doUsuniecia)
			{
				p6_tmp<<it1<<" ";
			}
			string p6_xxx = p6_tmp.str();
			lineHTMLStream<<htmlgen::div("do_usuniecia", "", p6_xxx)<<endl;
		}

		jsonStream<<"[";
		int licznikx=0;
		auto linieDoPrzerobieniaSorted = SpecialSortedString::convertSet(linieDoPrzerobienia);
		lineHTMLStream<<"<h2>Linie wygenerowane <span class=\"badge\">"<<linieDoPrzerobieniaSorted.size()<<"</span></h2>";
		for(auto it1 : linieDoPrzerobieniaSorted)
		{
			if(licznikx>0)
				jsonStream<<",";
			jsonStream<<"\""<<it1.str<<"\"";
			generujLinie(it1.str);
			string message1 = dodajInfoNormalne(przeglStare.relacjeDlaLinii[it1.str], it1.str, htmlGenerator, przeglStare.badRelations);
			message1 += dodajInfoRoznice(przeglStare.onlyOsmStop[it1.str], przeglStare.onlyZtmStop[it1.str], (przeglStare.badLines.find(it1.str)!=przeglStare.badLines.end()), it1.str, htmlGenerator);
			dodajLinieDoHTML(lineHTMLStream, 2, it1.str, message1, htmlGenerator);
			licznikx++;
		}
		jsonStream<<"]";
		jsonStream.close();
		auto linieNiewygenerowaneSorted = SpecialSortedString::convertSet(przeglNowe.getNieprawidlowe());
		auto linieNiewygenerowaneMap = przeglNowe.getNieprawidloweMap();
		lineHTMLStream<<"<h2>Linie niewygenerowane <span class=\"badge\">"<<linieNiewygenerowaneSorted.size()<<"</span></h2>";
		for(auto it1 : linieNiewygenerowaneSorted)
		{
			set<string> errPrzyst = linieNiewygenerowaneMap[it1.str];
			string message1 = dodajInfoNormalne(przeglStare.relacjeDlaLinii[it1.str], it1.str, htmlGenerator, przeglStare.badRelations);
			message1 += dodajInfoRoznice(przeglStare.onlyOsmStop[it1.str], przeglStare.onlyZtmStop[it1.str], (przeglStare.badLines.find(it1.str)!=przeglStare.badLines.end()), it1.str, htmlGenerator);
			message1 += dodajInfoNiewygenerowane(errPrzyst, it1.str, htmlGenerator);
			dodajLinieDoHTML(lineHTMLStream,1, it1.str, message1, htmlGenerator);
		}


		auto linieNormalneSorted = SpecialSortedString::convertSet(przeglStare.prawidlowe);
		lineHTMLStream<<"<h2>Linie bez zmian <span class=\"badge\">"<<linieNormalneSorted.size()<<"</span></h2>";
		for(auto it1 : linieNormalneSorted)
		{
			string message1 = dodajInfoNormalne(przeglStare.relacjeDlaLinii[it1.str], it1.str, htmlGenerator, przeglStare.badRelations);
			dodajLinieDoHTML(lineHTMLStream, 0, it1.str, message1, htmlGenerator);
		}
		/*
		cout<<"BLUE-END"<<endl;
		plik5<<testBadStops()<<endl;
		bool rss=false;
		if(miasto=="Warszawa")
			rss=true;
		htmlTile(plik5, rss);
		plik5.close();
		cout<<"ZZZZ-END"<<endl;
		*/

		uzupelnij(lineHTMLStream, pathTemplate+"/theme2.template");
		if(miasto=="Warszawa")
			lineHTMLStream<<"<script src=\"jq.js\"></script><script src=\"jfeed.js\"></script><script src=\"rss-test.js\"></script>";
		lineHTMLStream<<"</body></html>";
		lineHTMLStream.close();
		json2Stream.close();

		htmlGenerator.loadedVariables[0]=miasto;
		//przystankiHTMLStream<<htmlGenerator.loadTemplate(pathTemplate+"/theme5.template");

		uzupelnij(przystankiHTMLStream, pathTemplate+"/theme3.template");
		przystankiHTMLStream<<"Przystanki"<<miasto<<".json";
		uzupelnij(przystankiHTMLStream, pathTemplate+"/theme4.template");
		przystankiHTMLStream.close();
		vector <long long> rels;
		for(auto& it1 : przeglStare.relacjeDlaLinii)
		{
			for(auto& it2 : it1.second.second)
			{
				rels.push_back(it2);
			}
		}
		if(miasto=="Warszawa")
		{
			WypisywanieWspolrzednychTras(rels, bazaOsm, wspRoutePath);
		}
	}
	~MainClass()
	{
		delete bazaOsm;
		delete bazaZtm;
	}
};




int main(int argc, char** argv)
{
	MainClass foo(argv);
}
