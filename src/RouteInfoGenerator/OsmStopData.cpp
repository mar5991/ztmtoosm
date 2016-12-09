#include "osmbase/osm_base.hpp"
#include "OsmStopData.hpp"
OsmStopData::OsmStopData()
{
	stop_position = 0;
	bus_stop = 0;
	platform = 0;
	platform_type = 0;
}

map<string, OsmStopData> loadOsmStopData(osm_base* baza, string ref_key)
{
	map <string, OsmStopData> wynik;
	map<long long, node>::iterator it1=baza->nodes.begin();
	while(it1!=baza->nodes.end())
	{
		map <string, string> tags=it1->second.getTags();
		if(tags["highway"]=="bus_stop" || tags["railway"]=="tram_stop" || tags["public_transport"]=="stop_position")
		{
			if(tags[ref_key]!="")
			{
				string ref = tags[ref_key];
				if(tags.find("name")!=tags.end())
					wynik[ref].name=tags["name"];
				if(tags["highway"]=="bus_stop")
					wynik[ref].bus_stop=it1->first;
				if(tags["railway"]=="tram_stop")
					wynik[ref].bus_stop=it1->first;
				if(tags["public_transport"]=="stop_position")
					wynik[ref].stop_position=it1->first;
			}
		}
		it1++;
	}
	cout<<"ŁADOWANIE2..."<<endl;
	map<long long, way>::iterator it2=baza->ways.begin();
	while(it2!=baza->ways.end())
	{
		map <string, string> tags=it2->second.getTags();
		if(tags["public_transport"]=="platform" || tags["highway"]=="platform" || tags["railway"]=="platform")
		{
			if(tags[ref_key]!="")
			{
				string ref = tags[ref_key];
				wynik[ref].platform=it2->first;
				wynik[ref].platform_type='W';
			}
		}
		it2++;
	}
	map<long long, relation>::iterator it3=baza->relations.begin();
	while(it3!=baza->relations.end())
	{
		map <string, string> tags=it3->second.getTags();
		if(tags["public_transport"]=="platform" || tags["highway"]=="platform" || tags["railway"]=="platform")
		{
			if(tags[ref_key]!="")
			{
				string ref = tags[ref_key];
				wynik[ref].platform=it3->first;
				wynik[ref].platform_type='R';
			}
		}
		it3++;
	}
	return wynik;
}
