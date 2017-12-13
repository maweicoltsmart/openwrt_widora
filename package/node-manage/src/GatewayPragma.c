#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <malloc.h>
#include "GatewayPragma.h"
#include "nodedatabase.h"

#define GATEWAY_PRAGMA_FILE_PATH	"/usr/gatewaypragma.cfg"

gateway_pragma_t gateway_pragma;
FILE * file = NULL;

void GetGatewayPragma(gateway_pragma_t *gateway)
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
	obj = json_object_object_get(pragma,"radio");
	if((!len) || (!obj))
	{
		pragma = json_object_new_object();
		json_object_object_add(pragma,"radio",array = json_object_new_array());
		json_object_array_add(array,chip=json_object_new_object());
		json_object_object_add(chip,"index",json_object_new_int(0));
		json_object_object_add(chip,"channel",json_object_new_int(0));
		json_object_object_add(chip,"datarate",json_object_new_int(7));
		json_object_array_add(array,chip=json_object_new_object());
		json_object_object_add(chip,"index",json_object_new_int(1));
		json_object_object_add(chip,"channel",json_object_new_int(1));
		json_object_object_add(chip,"datarate",json_object_new_int(7));
		json_object_array_add(array,chip=json_object_new_object());
		json_object_object_add(chip,"index",json_object_new_int(2));
		json_object_object_add(chip,"channel",json_object_new_int(2));
		json_object_object_add(chip,"datarate",json_object_new_int(7));

		strcpy(buf,json_object_to_json_string(pragma));
		fwrite(buf,strlen(buf),1,file);
    	//printf("%s",buf);
    	json_object_put(pragma);
	}
	pragma = json_tokener_parse((struct json_object *)buf);
	array = json_object_object_get(pragma,"radio");
	for(i = 0; i < json_object_array_length(array); i++) {
		chip = json_object_array_get_idx(array, i);
		obj = json_object_object_get(chip,"index");
		tmp = json_object_get_int(obj);
		obj = json_object_object_get(chip,"datarate");
		tmp = json_object_get_int(obj);
    }
    fclose(file);
    json_object_put(pragma);
    free(buf);
}