#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <time.h>     //C语言的头文件

int main(void) 
{
    int i,n;
    printf("Contenttype:text/plainnn");
    n=0;
    if(getenv("CONTENT-LENGTH")) 
        n = atoi(getenv("CONTENT-LENGTH")); 
    unsigned char* html = malloc(n);
    gets(html);
    struct json_object *json = (struct json_object *)(html);
    struct json_object *node = NULL;
    //cJOSN_GetObjectItem 根据key来查找json节点 若果有返回非空
#if 0
    if(1 == cJSON_HasObjectItem(json,"devices"))
    {
        printf("found devices node\n");
    }
    else
    {
        printf("not found devices node\n");
    }
#endif
    node = json_object_object_get(json,"devices");
#if 0
    if(node->type == cJSON_Array)
    {
        printf("array size is %d\r\n",cJSON_GetArraySize(node));
    }
    else
    {
        printf("devices is not a node\r\n");
        return 0;
    }
#endif
    struct json_object *obj;
    for(i = 0; i < json_object_array_length(node); i++) {
      obj = json_object_array_get_idx(node, i);
      printf("\t[%d]=%s\n", i, json_object_to_json_string(obj));
    }

    json_object_put(json);//free  
    free(html);
} 