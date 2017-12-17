/* Change this if the SERVER_NAME environment variable does not report
	the true name of your web server. */
#if 1
#define SERVER_NAME cgiServerName
#endif
#if 0
#define SERVER_NAME "www.boutell.com"
#endif

/* You may need to change this, particularly under Windows;
	it is a reasonable guess as to an acceptable place to
	store a saved environment in order to test that feature. 
	If that feature is not important to you, you needn't
	concern yourself with this. */

#ifdef WIN32
#define SAVED_ENVIRONMENT "c:\\cgicsave.env"
#else
#define SAVED_ENVIRONMENT "/tmp/cgicsave.env"
#endif /* WIN32 */

#include <stdio.h>
#include "cgic.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <malloc.h>
#include "GatewayPragma.h"

void HandleSubmit();
void ShowForm();
void CookieSet();
void Name();
void Address();
void Hungry();
void Temperature();
void Frogs();
void datarate1();
void Flavors();
void NonExButtons();
void nettype();
void File();
void Entries();
void Cookies();
void LoadEnvironment();
void SaveEnvironment();

gateway_pragma_t gateway_pragma;
FILE * file = NULL;

void str2Hex(uint8_t *str,uint8_t *dest)    
{      
	unsigned char hb;
	unsigned char lb;
	int i = 0,j = 0;
	  
	if(strlen(str)%2!=0)  
	return;  
	  	  
	for(i=0, j=0;i<strlen(str);i++)	
		{  
			hb=str[i];	
			if( hb>='A' && hb<='F' )  
				hb = hb - 'A' + 10;  
			else if( hb>='0' && hb<='9' )  
				hb = hb - '0';	
			else  
				return;  
	  
			i++;  
			lb=str[i];	
			if( lb>='A' && lb<='F' )  
				lb = lb - 'A' + 10;  
			else if( lb>='0' && lb<='9' )  
				lb = lb - '0';	
			else  
				return;  
	  
			dest[j++]=(hb<<4)|(lb);  
		}  
}    

void hex2str(uint8_t *byte,uint8_t *str,int len)
{
	char *ch = {"0123456789ABCDEF"};
	int loop;
	for(loop = 0;loop < len;loop++)
	{
		str[loop * 2] = ch[byte[loop] >> 4];
		str[loop * 2 + 1] = ch[byte[loop] & 0x0f];
	}
}
void GetGatewayPragma(void)
{
	int len;
	int32_t tmp;
	struct json_object *pragma = NULL;
	struct json_object *chip = NULL;
	struct json_object *obj = NULL;
	struct json_object *array = NULL;
	int i = 0;
	uint8_t *buf = malloc(1024);

	memset(buf,0,1024);
	file = fopen(GATEWAY_PRAGMA_FILE_PATH, "wt+");
	len = fread ( buf, 1, 1024, file) ;
	pragma = (struct json_object *)buf;
	obj = json_object_object_get(pragma,"NetType");
	if((!len) || (!obj))
	{
		pragma = json_object_new_object();
		json_object_object_add(pragma,"NetType",json_object_new_string("Private"));
		json_object_object_add(pragma,"APPKEY",json_object_new_string("2B7E151628AED2A6ABF7158809CF4F3C"));
		json_object_object_add(pragma,"AppNonce",json_object_new_string("012345"));
		json_object_object_add(pragma,"NetID",json_object_new_string("ABCDEF"));
		
		json_object_object_add(pragma,"serverip",json_object_new_string("192.168.1.100"));
		json_object_object_add(pragma,"serverport",json_object_new_int(32500));
		json_object_object_add(pragma,"localip",json_object_new_string("192.168.1.100"));
		json_object_object_add(pragma,"localport",json_object_new_int(32500));

		json_object_object_add(pragma,"radio",array = json_object_new_array());
		json_object_array_add(array,chip=json_object_new_object());
		json_object_object_add(chip,"index",json_object_new_int(0));
		json_object_object_add(chip,"channel",json_object_new_int(0));
		json_object_object_add(chip,"datarate",json_object_new_int(7));
		json_object_array_add(array,chip=json_object_new_object());
		json_object_object_add(chip,"index",json_object_new_int(1));
		json_object_object_add(chip,"channel",json_object_new_int(1));
		json_object_object_add(chip,"datarate",json_object_new_int(10));
		json_object_array_add(array,chip=json_object_new_object());
		json_object_object_add(chip,"index",json_object_new_int(2));
		json_object_object_add(chip,"channel",json_object_new_int(2));
		json_object_object_add(chip,"datarate",json_object_new_int(12));

		strcpy(buf,json_object_to_json_string(pragma));
		fwrite(buf,strlen(buf),1,file);
    	//printf("%s",buf);
    	json_object_put(pragma);
	}
	pragma = json_tokener_parse((struct json_object *)buf);
	obj = json_object_object_get(pragma,"NetType");
	if(strcmp(json_object_get_string(obj),"Private") == 0)
	{
		gateway_pragma.nettype = 0;
	}
	else
	{
		gateway_pragma.nettype = 1;
	}
	obj = json_object_object_get(pragma,"APPKEY");
	str2Hex(json_object_get_string(obj),gateway_pragma.APPKEY);
	obj = json_object_object_get(pragma,"AppNonce");
	str2Hex(json_object_get_string(obj),gateway_pragma.AppNonce);
	obj = json_object_object_get(pragma,"NetID");
	str2Hex(json_object_get_string(obj),gateway_pragma.NetID);
	obj = json_object_object_get(pragma,"serverip");
	gateway_pragma.server_ip= json_object_get_int(obj);
	obj = json_object_object_get(pragma,"serverport");
	gateway_pragma.server_port= json_object_get_int(obj);
	obj = json_object_object_get(pragma,"localip");
	gateway_pragma.local_ip= json_object_get_int(obj);
	obj = json_object_object_get(pragma,"localport");
	gateway_pragma.local_port= json_object_get_int(obj);
	
	array = json_object_object_get(pragma,"radio");
	for(i = 0; i < json_object_array_length(array); i++) {
		chip = json_object_array_get_idx(array, i);
		obj = json_object_object_get(chip,"index");
		tmp = json_object_get_int(obj);
		obj = json_object_object_get(chip,"channel");
		gateway_pragma.radiochannel[i] = json_object_get_int(obj);
		obj = json_object_object_get(chip,"datarate");
		gateway_pragma.radiodatarate[i] = json_object_get_int(obj);
		fprintf(cgiOut, "<P>channel = %d;datarate = %d\n",gateway_pragma.radiochannel[i],gateway_pragma.radiodatarate[i]);
    }
    fclose(file);
    json_object_put(pragma);
    free(buf);
}

void SetGatewayPragma(void)
{
	int len;
	int32_t tmp;
	struct json_object *pragma = NULL;
	struct json_object *chip = NULL;
	struct json_object *obj = NULL;
	struct json_object *array = NULL;
	int i = 0;
	int loop = 0;
	uint8_t byte[100];
	uint8_t *buf = malloc(1024);

	memset(buf,0,1024);
	file = fopen(GATEWAY_PRAGMA_FILE_PATH, "w+");
	len = fread ( buf, 1, 1024, file) ;
	pragma = (struct json_object *)buf;
	obj = json_object_object_get(pragma,"NetType");
	if((!len) || (!obj))
	{
		fprintf(cgiOut, "WRITE CFG\n");
		pragma = json_object_new_object();
		
		if(gateway_pragma.nettype == 0)
		{
			
			json_object_object_add(pragma,"NetType",json_object_new_string("Private"));
		}
		else
		{
			json_object_object_add(pragma,"NetType",json_object_new_string("Public"));
		}
		memset(byte,0,100);
		hex2str(gateway_pragma.APPKEY,byte,16);
		json_object_object_add(pragma,"APPKEY",json_object_new_string(byte));
		memset(byte,0,100);
		hex2str(gateway_pragma.AppNonce,byte,3);
		json_object_object_add(pragma,"AppNonce",json_object_new_string(byte));
		memset(byte,0,100);
		hex2str(gateway_pragma.NetID,byte,3);
		json_object_object_add(pragma,"NetID",json_object_new_string(byte));
		json_object_object_add(pragma,"serverip",json_object_new_string("192.168.1.100"));
		json_object_object_add(pragma,"serverport",json_object_new_int(32500));
		json_object_object_add(pragma,"localip",json_object_new_string("192.168.1.100"));
		json_object_object_add(pragma,"localport",json_object_new_int(32500));

		json_object_object_add(pragma,"radio",array = json_object_new_array());
		for(loop = 0;loop < 3;loop++)
		{
			json_object_array_add(array,chip=json_object_new_object());
			json_object_object_add(chip,"index",json_object_new_int(0));
			json_object_object_add(chip,"channel",json_object_new_int(gateway_pragma.radiochannel[loop]));
			json_object_object_add(chip,"datarate",json_object_new_int(gateway_pragma.radiodatarate[loop]));
		}
		
		strcpy(buf,json_object_to_json_string(pragma));
		fwrite(buf,strlen(buf),1,file);
    	//printf("%s",buf);
    	json_object_put(pragma);
	}
    fclose(file);
    json_object_put(pragma);
    free(buf);
}

int cgiMain() {
#ifdef DEBUG
	//LoadEnvironment();
#endif /* DEBUG */
	/* Load a previously saved CGI scenario if that button
		has been pressed. */
	//if (cgiFormSubmitClicked("loadenvironment") == cgiFormSuccess) {
	//	LoadEnvironment();
	//}
	/* Set any new cookie requested. Must be done *before*
		outputting the content type. */
	//CookieSet();
	/* Send the content type, letting the browser know this is HTML */
	cgiHeaderContentType("text/html");
	/* Top of the page */
	fprintf(cgiOut, "<HTML><HEAD>\n");
	fprintf(cgiOut, "<TITLE>网关参数配置</TITLE></HEAD>\n");
	fprintf(cgiOut, "<BODY><H1>网关参数配置</H1>\n");
	/* If a submit button has already been clicked, act on the 
		submission of the form. */
	if ((cgiFormSubmitClicked("update") == cgiFormSuccess))// ||
	//	cgiFormSubmitClicked("saveenvironment") == cgiFormSuccess)
	{
		HandleSubmit();
		fprintf(cgiOut, "<hr>\n");
	}
	/* Now show the form */
	ShowForm();
	/* Finish up the page */
	fprintf(cgiOut, "</BODY></HTML>\n");
	return 0;
}


void APPKEY() {
	char APPKEY[241];
	cgiFormString("APPKEY", APPKEY, 241);
	str2Hex(APPKEY,gateway_pragma.APPKEY);
	//fprintf(cgiOut, "appkey: %s<PRE>\n",APPKEY);
	//cgiHtmlEscape(address);
	//fprintf(cgiOut, "</PRE>\n");
}

void AppNonce() {
	char AppNonce[241];
	cgiFormString("AppNonce", AppNonce, 241);
	str2Hex(AppNonce,gateway_pragma.AppNonce);
	//fprintf(cgiOut, "appkey: <PRE>\n");
	//cgiHtmlEscape(address);
	//fprintf(cgiOut, "</PRE>\n");
}

void NetID() {
	char NetID[241];
	cgiFormString("NetID", NetID, 241);
	str2Hex(NetID,gateway_pragma.NetID);
	//fprintf(cgiOut, "appkey: <PRE>\n");
	//cgiHtmlEscape(address);
	//fprintf(cgiOut, "</PRE>\n");
}

void channel1() {
	char channel1[241];
	cgiFormString("channel1", channel1, 241);
	str2Hex(channel1,gateway_pragma.radiochannel[0]);
	//fprintf(cgiOut, "appkey: <PRE>\n");
	//cgiHtmlEscape(address);
	//fprintf(cgiOut, "</PRE>\n");
}

void channel2() {
	char channel2[241];
	cgiFormString("channel2", channel2, 241);
	str2Hex(channel2,gateway_pragma.radiochannel[1]);
	//fprintf(cgiOut, "appkey: <PRE>\n");
	//cgiHtmlEscape(address);
	//fprintf(cgiOut, "</PRE>\n");
}

void channel3() {
	char channel3[241];
	cgiFormString("channel3", channel3, 241);
	str2Hex(channel3,gateway_pragma.radiochannel[2]);
	//fprintf(cgiOut, "appkey: <PRE>\n");
	//cgiHtmlEscape(address);
	//fprintf(cgiOut, "</PRE>\n");
}


char *colors[] = {
	"DR_0",
	"DR_1",
	"DR_2",
	"DR_3",
	"DR_4",
	"DR_5"
};

void datarate1() {
	int colorChoice;
	cgiFormSelectSingle("datarate1", colors, 6, &colorChoice, 0);
	gateway_pragma.radiodatarate[0] = colorChoice + 7;
	fprintf(cgiOut, "I am: %s<BR>\n", colors[colorChoice]);
}	 

void datarate2() {
	int colorChoice;
	cgiFormSelectSingle("datarate2", colors, 6, &colorChoice, 0);
	gateway_pragma.radiodatarate[1] = colorChoice + 7;
	fprintf(cgiOut, "I am: %s<BR>\n", colors[colorChoice]);
}	 

void datarate3() {
	int colorChoice;
	cgiFormSelectSingle("datarate3", colors, 6, &colorChoice, 0);
	gateway_pragma.radiodatarate[2] = colorChoice + 7;
	fprintf(cgiOut, "I am: %s<BR>\n", colors[colorChoice]);
}	 

char *ages[] = {
	"Private",
	"Public"
};

void nettype() {
	int ageChoice;
	char ageText[10];
	/* Approach #1: check for one of several valid responses. 
		Good if there are a short list of possible button values and
		you wish to enumerate them. */
	cgiFormRadio("nettype", ages, 2, &ageChoice, 0);

	fprintf(cgiOut, "Age of Truck: %s (method #1)<BR>\n", 
		ages[ageChoice]);

	/* Approach #2: just get the string. Good
		if the information is not critical or if you wish
		to verify it in some other way. Note that if
		the information is numeric, cgiFormInteger,
		cgiFormDouble, and related functions may be
		used instead of cgiFormString. */	
	cgiFormString("age", ageText, 10);

	fprintf(cgiOut, "Age of Truck: %s (method #2)<BR>\n", ageText);
}

char *votes[] = {
	"A",
	"B",
	"C",
	"D"
};

void Entries()
{
	char **array, **arrayStep;
	fprintf(cgiOut, "List of All Submitted Form Field Names:<p>\n");
	if (cgiFormEntries(&array) != cgiFormSuccess) {
		return;
	}
	arrayStep = array;
	fprintf(cgiOut, "<ul>\n");
	while (*arrayStep) {
		fprintf(cgiOut, "<li>");
		cgiHtmlEscape(*arrayStep);
		fprintf(cgiOut, "\n");
		arrayStep++;
	}
	fprintf(cgiOut, "</ul>\n");
	cgiStringArrayFree(array);
}

void HandleSubmit()
{	
	//Name();
	//Address();
	//Hungry();
	//Temperature();
	//Frogs();
	APPKEY();
	AppNonce();
	NetID();
	nettype();
	channel1();
	datarate1();
	channel2();
	datarate2();
	channel3();
	datarate3();
	SetGatewayPragma();
	//Flavors();
	//NonExButtons();
	
	//File();
	//Entries();
	//Cookies();
	/* The saveenvironment button, in addition to submitting the form,
		also saves the resulting CGI scenario to disk for later
		replay with the 'load saved environment' button. */
	//if (cgiFormSubmitClicked("saveenvironment") == cgiFormSuccess) {
	//	SaveEnvironment();
	//}
}
	

void ShowForm()
{
	int loop = 0;
	uint8_t byte[100];
	GetGatewayPragma();
	fprintf(cgiOut, "<!-- 2.0: multipart/form-data is required for file uploads. -->");
	fprintf(cgiOut, "<form method=\"POST\" enctype=\"multipart/form-data\" ");
	fprintf(cgiOut, "	action=\"");
	cgiValueEscape(cgiScriptName);
	fprintf(cgiOut, "\">\n");
	fprintf(cgiOut, "<p>\n");

	memset(byte,0,100);
	hex2str(gateway_pragma.APPKEY,byte,16);
	fprintf(cgiOut, "<p>APPKEY<input name=\"APPKEY\" value=\"%s\">\n",byte);
	memset(byte,0,100);
	hex2str(gateway_pragma.AppNonce,byte,3);
	fprintf(cgiOut, "<p>AppNonce<input name=\"AppNonce\" value=\"%s\">\n",byte);
	memset(byte,0,100);
	hex2str(gateway_pragma.NetID,byte,3);
	fprintf(cgiOut, "<p>NetID<input name=\"NetID\" value=\"%s\">\n",byte);
	
	fprintf(cgiOut, "<p>网络类型:\n");
	if(gateway_pragma.nettype == 0)
	{
		fprintf(cgiOut, "<input type=\"radio\" name=\"nettype\" value=\"Private\" checked>Private\n");
		fprintf(cgiOut, "<input type=\"radio\" name=\"nettype\" value=\"Public\">Public\n");
	}
	else
	{
		fprintf(cgiOut, "<input type=\"radio\" name=\"nettype\" value=\"Private\">Private\n");
		fprintf(cgiOut, "<input type=\"radio\" name=\"nettype\" value=\"Public\" checked>Public\n");
	}
	for(loop = 0;loop < 3;loop++)
	{
	fprintf(cgiOut, "<p>\n");
	fprintf(cgiOut, "收发器%d信道<input name=\"channel%d\" value=\"%d\">\n",loop,loop,gateway_pragma.radiochannel[loop]);
	fprintf(cgiOut, "<p>\n");
	fprintf(cgiOut, "收发器%d速率\n",loop);
	switch(gateway_pragma.radiodatarate[loop])
	{
		case 7:
			fprintf(cgiOut, "<select name=\"datarate%d\">\n",loop);
			fprintf(cgiOut, "<option value=\"DR_0\" selected = \"selected\">DR_0\n");
			fprintf(cgiOut, "<option value=\"DR_1\">DR_1\n");
			fprintf(cgiOut, "<option value=\"DR_2\">DR_2\n");
			fprintf(cgiOut, "<option value=\"DR_3\">DR_3\n");
			fprintf(cgiOut, "<option value=\"DR_4\">DR_4\n");
			fprintf(cgiOut, "<option value=\"DR_5\">DR_5\n");
			fprintf(cgiOut, "</select>\n");
			break;
		
		case 8:
			fprintf(cgiOut, "<select name=\"datarate%d\">\n",loop);
			fprintf(cgiOut, "<option value=\"DR_0\">DR_0\n");
			fprintf(cgiOut, "<option value=\"DR_1\" selected = \"selected\">DR_1\n");
			fprintf(cgiOut, "<option value=\"DR_2\">DR_2\n");
			fprintf(cgiOut, "<option value=\"DR_3\">DR_3\n");
			fprintf(cgiOut, "<option value=\"DR_4\">DR_4\n");
			fprintf(cgiOut, "<option value=\"DR_5\">DR_5\n");
			fprintf(cgiOut, "</select>\n");
			break;
		case 9:
			fprintf(cgiOut, "<select name=\"datarate%d\">\n",loop);
			fprintf(cgiOut, "<option value=\"DR_0\">DR_0\n");
			fprintf(cgiOut, "<option value=\"DR_1\">DR_1\n");
			fprintf(cgiOut, "<option value=\"DR_2\" selected = \"selected\">DR_2\n");
			fprintf(cgiOut, "<option value=\"DR_3\">DR_3\n");
			fprintf(cgiOut, "<option value=\"DR_4\">DR_4\n");
			fprintf(cgiOut, "<option value=\"DR_5\">DR_5\n");
			fprintf(cgiOut, "</select>\n");
			break;
		case 10:
			fprintf(cgiOut, "<select name=\"datarate%d\">\n",loop);
			fprintf(cgiOut, "<option value=\"DR_0\">DR_0\n");
			fprintf(cgiOut, "<option value=\"DR_1\">DR_1\n");
			fprintf(cgiOut, "<option value=\"DR_2\">DR_2\n");
			fprintf(cgiOut, "<option value=\"DR_3\" selected = \"selected\">DR_3\n");
			fprintf(cgiOut, "<option value=\"DR_4\">DR_4\n");
			fprintf(cgiOut, "<option value=\"DR_5\">DR_5\n");
			fprintf(cgiOut, "</select>\n");
			break;
		case 11:
			fprintf(cgiOut, "<select name=\"datarate%d\">\n",loop);
			fprintf(cgiOut, "<option value=\"DR_0\">DR_0\n");
			fprintf(cgiOut, "<option value=\"DR_1\">DR_1\n");
			fprintf(cgiOut, "<option value=\"DR_2\">DR_2\n");
			fprintf(cgiOut, "<option value=\"DR_3\">DR_3\n");
			fprintf(cgiOut, "<option value=\"DR_4\" selected = \"selected\">DR_4\n");
			fprintf(cgiOut, "<option value=\"DR_5\">DR_5\n");
			fprintf(cgiOut, "</select>\n");
			break;
		case 12:
			fprintf(cgiOut, "<select name=\"datarate%d\">\n",loop);
			fprintf(cgiOut, "<option value=\"DR_0\">DR_0\n");
			fprintf(cgiOut, "<option value=\"DR_1\">DR_1\n");
			fprintf(cgiOut, "<option value=\"DR_2\">DR_2\n");
			fprintf(cgiOut, "<option value=\"DR_3\">DR_3\n");
			fprintf(cgiOut, "<option value=\"DR_4\">DR_4\n");
			fprintf(cgiOut, "<option value=\"DR_5\" selected = \"selected\">DR_5\n");
			fprintf(cgiOut, "</select>\n");
			break;
		default:
			break;
	}
	}
	fprintf(cgiOut, "<br>\n");
	fprintf(cgiOut, "<input type=\"submit\" name=\"update\" value=\"提交\">\n");
	fprintf(cgiOut, "<input type=\"reset\" name=\"factory\" value=\"重置\">\n");
	fprintf(cgiOut, "</form>\n");
}


