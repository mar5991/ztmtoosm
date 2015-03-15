var palette = ['#ab0d15'
,'#F7931D'
/*,'#FFF200'*/
,'#0DB14B'
,'#0071BC'
,'#00B9F1'
,'#903F98'
,'#EE609D'];

var dni = [
"niedziela",
"poniedziałek",
"wtorek",
"środa",
"czwartek",
"piątek",
"sobota",
];

var miesiace = [
"stycznia",
"lutego",
"marca",
"kwietnia",
"maja",
"czerwca",
"lipca"];

function getpalette(poz, n)
{
	return palette[Math.floor(poz%7)];
}
function getpaletteid(poz, n)
{
	return Math.floor(poz%7);
}

function loadJSON(url) 
{
	var resp;
	var xmlHttp;
	resp  = '';
	xmlHttp = new XMLHttpRequest();

	if(xmlHttp != null)
	{
		xmlHttp.open( "GET", url, false );
		xmlHttp.send( null );
		resp = xmlHttp.responseText;
	}
	return JSON.parse(resp); 
}
function podrozSciezka (from, to, time)
{
	var sciezka="/hafas";
	sciezka+='\?from\='+from;
	sciezka+='\&to\='+to;
	sciezka+='\&time\='+time;
	console.log(sciezka);
	return sciezka;
}
function polaczeniaSciezka (from, delim, time)
{
	var sciezka="/hafas";
	sciezka+='\?from\='+from;
	sciezka+='\&delim\='+delim;
	sciezka+='\&time\='+time;
	return sciezka;
}
function kursSciezka (from, line, time)
{
	var sciezka="/hafas";
	sciezka+='\?from\='+from;
	sciezka+='\&line\='+line;
	sciezka+='\&time\='+time;
	return sciezka;
}
function addLineToSource (coordinates, colorLine, name, vectorSource1, vectorSource2)
{
	var warstwa1 = new ol.Feature({geometry: new ol.geom.LineString(coordinates)});
	var warstwaStroke1 = new ol.style.Stroke ({color: colorLine, width: 4});
	var warstwaStrokeText1 = new ol.style.Stroke ({color: '#fff', width: 6});
	var warstwaFont1 = '18px helvetica,sans-serif,bold';
	var warstwaFill1 = new ol.style.Fill({color: colorLine});
	var warstwaText1 = new ol.style.Text({text: name, font: warstwaFont1, fill: warstwaFill1, stroke: warstwaStrokeText1});
	var warstwaStyle1 =new ol.style.Style({stroke : warstwaStroke1, text : warstwaText1});
	warstwa1.setStyle(warstwaStyle1);
	var warstwa2 = new ol.Feature({geometry: new ol.geom.LineString(coordinates)});
	var warstwaStroke2 = new ol.style.Stroke ({color: '#fff', width: 7});
	var warstwaStyle2 = new ol.style.Style({stroke : warstwaStroke2});
	warstwa2.setStyle(warstwaStyle2);
	vectorSource1.addFeature(warstwa1);
	vectorSource2.addFeature(warstwa2);
}
var PrzystanekNaMapie = function (name, id, coordinates, vectorSource)
{
	this.name = name;
	this.coordinates = coordinates;
	this.propsedColor = null;
	this.id = id;
	this.minTime = null;
	this.minTimeLine = null;
	this.maxTime = null;
	this.maxTimeLine = null;
	this.feature = new ol.Feature({geometry: new ol.geom.Point(coordinates)});
	this.feature.setId(id);
	vectorSource.addFeature(this.feature);
};

PrzystanekNaMapie.prototype.addLine = function (time, line, color)
{
	console.log(time);
	if(this.propsedColor == null)
		this.propsedColor = color;
	else
		this.propsedColor = '#000';
	if(this.minTime == null || time < this.mintime)
	{
		this.minTime = time;
		this.minTimeLine = line;
	}
	if(this.maxTime == null || time < this.maxtime)
	{
		this.maxTime = time;
		this.maxTimeLine = line;
	}
};

PrzystanekNaMapie.prototype.setBasicStyle = function ()
{
	this.feature.setStyle(
		new ol.style.Style({
	    image: new ol.style.Circle({
	      fill: new ol.style.Fill({
		color: this.propsedColor
	      }),
	      radius: 8,
	      stroke: new ol.style.Stroke({
		color: '#fff',
		width: 2
	      })
	    })
	  }));
};

PrzystanekNaMapie.prototype.setExtraStyle = function ()
{
	var name = this.name;
	var warstwaStrokeText1 = new ol.style.Stroke ({color: '#fff', width: 6});
	var warstwaFont1 = '18px helvetica,sans-serif,bold';
	var warstwaFill1 = new ol.style.Fill({color: this.propsedColor});
	var txt = new ol.style.Text({text: name, font: warstwaFont1, fill: warstwaFill1, stroke: warstwaStrokeText1});
	this.feature.setStyle(
		new ol.style.Style({
	    image: new ol.style.Circle({
	      fill: new ol.style.Fill({
		color: this.propsedColor
	      }),
	      radius: 12,
	      stroke: new ol.style.Stroke({
		color: '#fff',
		width: 4
	      })
	    }), text : txt
	  }));

};

function stringTime(time)
{
				var tim=new Date(time*1000);
var mins=tim.getMinutes();
				var hours=tim.getHours();
				var timString="";
				if(hours<10)
					timString+="0";
				timString+=hours+":";
				if(mins<10)
					timString+="0";
				timString+=mins;
				return timString;
}


function loadJSON(url) {
var resp ;
var xmlHttp ;

resp  = '' ;
xmlHttp = new XMLHttpRequest();

if(xmlHttp != null)
{
xmlHttp.open( "GET", url, false );
xmlHttp.send( null );
resp = xmlHttp.responseText;
}
       return JSON.parse(resp); 
}

var tabsource = null;

var basicVector = new ol.source.Vector({});
var basicZnaczniki = new ol.source.Vector({});
var projection = ol.proj.get('EPSG:3857');
var layerLines = new ol.layer.Vector({
source: basicVector,
    style: function(feature, resolution) {
    return style[feature.getGeometry().getType()];
  }
});
var layerLines2 = new ol.layer.Vector({
  source: basicVector,
    style: function(feature, resolution) {
    return style2[feature.getGeometry().getType()];
  }
});
var layerLinesZnaczniki = new ol.layer.Vector({
  source: basicZnaczniki,
    style: function(feature, resolution) {
    return style2[feature.getGeometry().getType()];
  }
});




      var map = new ol.Map({
        target: 'map',
        layers: [
          new ol.layer.Tile({
            source: new ol.source.OSM({url: " http://a.tiles.wmflabs.org/hikebike/{z}/{x}/{y}.png", crossOrigin: null})
          }), layerLines, layerLines2, layerLinesZnaczniki],
        view: new ol.View({
          center: ol.proj.transform([21.05, 52.23], 'EPSG:4326', 'EPSG:3857'),
          zoom: 12
	})});
//document.body.style.cursor = 'url("marker-icon.png"), auto';

//fett.style.display="none";
for(var i=0; i<24; i++)
{
	var option = document.createElement("option");
	option.text=i;
	option.value=i;
	var d=new Date();
	if(d.getHours()==i)
		option.selected="selected";
	document.getElementById("wyborGodziny").add(option);
}
for(var i=0; i<60; i++)
{
	var option = document.createElement("option");
	option.text=i;
	option.value=i;
	var d=new Date();
	if(d.getMinutes()==i)
		option.selected="selected";
	if(i<10)
	{
		option.text="0"+i;
	}
	document.getElementById("wyborMinuty").add(option);
}

function eleganckiCzas(czas)
{
	var wynik = "";
	wynik+=dni[czas.getDay()]+", ";
	wynik+=czas.getDate()+" ";
	wynik+=miesiace[czas.getMonth()]+" ";
	return wynik;
}


for(var i=0; i<3; i++)
{
	var d = new Date();
	var d_prim = new Date();
	d_prim.setTime(d.getTime()+i*24*60*60*1000);
	var option = document.createElement("option");
	option.text=eleganckiCzas(d_prim);
	option.value=i;
	document.getElementById("wyborDnia").add(option);
}

function getSettedTime()
{
	var d = new Date();
	var d1 = new Date();
	var wyborDnia=document.getElementById("wyborDnia");
	var wyborGodziny=document.getElementById("wyborGodziny");
	var wyborMinuty=document.getElementById("wyborMinuty");
	var wyb=parseInt(wyborDnia.options[wyborDnia.selectedIndex].value);
	d1.setTime(d.getTime()+wyb*24*60*60*1000);
	var wybMin=parseInt(wyborMinuty.options[wyborMinuty.selectedIndex].value);
	var wybGodz=parseInt(wyborGodziny.options[wyborGodziny.selectedIndex].value);
	
	
	var d2=new Date(d1.getFullYear(), d1.getMonth(), d1.getDate(), wybGodz, wybMin,0, 0);
	var wynik=d2.getTime();
	console.log(wynik);
	console.log(wybMin);
	console.log(wybGodz);
	return wynik;
}

var today = new Date();
var aktfeature = null;
var mapcnt=document.getElementById("map");






	
mapcnt.onmousemove = function(e)
{
	var pos   = {X : mapcnt.offsetLeft, Y : mapcnt.offsetTop};
	var mPos  = {X : e.clientX - pos.X, Y : e.clientY - pos.Y};
	var newfeature = null;
	map.forEachFeatureAtPixel([mPos.X, mPos.Y], function(feature, layer){
	if(feature.getGeometry().getType()=='Point')
	{
		newfeature=feature.getId();
	}
	});
	if(newfeature!=aktfeature)
	{
		if(newfeature!=null)
		{
			tabsource.przystanki[newfeature].setExtraStyle();
		}
		if(aktfeature!=null)
		{
			tabsource.przystanki[aktfeature].setBasicStyle();
		}
		aktfeature=newfeature;
	}
}

function changesource()
{
	tribe=0;
    	var from = wpf.instantCoordinates();
    	var to = wpt.instantCoordinates();
	if(wpf.show==0 || wpt.show==0)
	{
		alert("Brak wprowadzonych danych");
	}
	else
	{
		var time = parseInt(getSettedTime()/1000);
		tabsource = new WizualizacjaPodrozy(from, to, time, 0);
	}
}

function changesource2(from, time)
{
    	var heart = document.getElementById("heart");
	var visu = new WizualizacjaDrzewa(from, time, null, heart, 0);
}

function zrownaj()
{
    	var heart = document.getElementById("heart");
    	var map0 = document.getElementById("map0");
	heart.style.maxHeight=(window.innerHeight-110)+"px";
}
zrownaj();

/*map.on('click', function(evt){
console.log("dupa");
});*/

/*
if (navigator.geolocation) {
	        navigator.geolocation.getCurrentPosition(showPosition);
		    }
*/

var wpf = new WprowadzanieWspolrzednych(document.getElementById("from"), "green.png", "Z...");
var wpt = new WprowadzanieWspolrzednych(document.getElementById("to"), "red.png", "Do...");
window.addEventListener("resize", zrownaj, false);
document.getElementById("overlay").style.display="none";






$(document).ready(function() {

console.log("ready");
//if ($("#map0").addEventListener) {
//$("#map0").addEventListener('contextmenu', function(e) {
//al/ert("You've tried to open context menu"); //here you draw your own menu
//e.preventDefault();
//}, false);
//}
// else {
//document.getElementById("test").attachEvent('oncontextmenu', function() {
//$(".test").bind('contextmenu', function() {
$('#map0').on('contextmenu', '', function() {
console.log(map);
var pix=[mouseX(event), mouseY(event)];
this.wsp=ol.proj.transform(map.getCoordinateFromPixel(pix), 'EPSG:3857', 'EPSG:4326');
console.log("xDDDD");
document.getElementById("startlink").wsp = this.wsp;
document.getElementById("stoplink").wsp = this.wsp;
document.getElementById("startlink").onclick = function(){wpf.onIndicatePointer(this.wsp)};
document.getElementById("stoplink").onclick = function(){wpt.onIndicatePointer(this.wsp)};
document.getElementById("rmenu").className = "show";  
document.getElementById("rmenu").style.top =  mouseY(event) + "px";
document.getElementById("rmenu").style.left = mouseX(event) + "px";

window.event.returnValue = false;


});
//}
});

// this is from another SO post...  
$(document).bind("click", function(event) {
document.getElementById("rmenu").className = "hide";
});



function mouseX(evt) {
if (evt.pageX) {
return evt.pageX;
} else if (evt.clientX) {
return evt.clientX + (document.documentElement.scrollLeft ?
document.documentElement.scrollLeft :
document.body.scrollLeft);
} else {
return null;
}
}

function mouseY(evt) {
if (evt.pageY) {
return evt.pageY;
} else if (evt.clientY) {
return evt.clientY + (document.documentElement.scrollTop ?
document.documentElement.scrollTop :
document.body.scrollTop);
} else {
return null;
}
}
