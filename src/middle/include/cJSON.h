/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef divoom_cJSON__h
#define divoom_cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

/* divoom_cJSON Types: */
#define divoom_cJSON_False  (1 << 0)
#define divoom_cJSON_True   (1 << 1)
#define divoom_cJSON_NULL   (1 << 2)
#define divoom_cJSON_Number (1 << 3)
#define divoom_cJSON_String (1 << 4)
#define divoom_cJSON_Array  (1 << 5)
#define divoom_cJSON_Object (1 << 6)

#define divoom_cJSON_IsReference 256
#define divoom_cJSON_StringIsConst 512

/* The divoom_cJSON structure: */
typedef struct divoom_cJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct divoom_cJSON *next;
    struct divoom_cJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct divoom_cJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==divoom_cJSON_String */
    char *valuestring;
    /* The item's number, if type==divoom_cJSON_Number */
    int valueint;
    /* The item's number, if type==divoom_cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} divoom_cJSON;

typedef struct divoom_cJSON_Hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} divoom_cJSON_Hooks;

/* Supply malloc, realloc and free functions to divoom_cJSON */
extern void divoom_cJSON_InitHooks(divoom_cJSON_Hooks* hooks);


/* Supply a block of JSON, and this returns a divoom_cJSON object you can interrogate. Call divoom_cJSON_Delete when finished. */
extern divoom_cJSON *divoom_cJSON_Parse(const char *value);
/* Render a divoom_cJSON entity to text for transfer/storage. Free the char* when finished. */
extern char  *divoom_cJSON_Print(const divoom_cJSON *item);
/* Render a divoom_cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char  *divoom_cJSON_PrintUnformatted(const divoom_cJSON *item);
/* Render a divoom_cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
extern char *divoom_cJSON_PrintBuffered(const divoom_cJSON *item, int prebuffer, int fmt);
/* Render a divoom_cJSON entity to text using a buffer already allocated in memory with length buf_len */
extern int divoom_cJSON_PrintPreallocated(divoom_cJSON *item, char *buf, const int len, const int fmt);
/* Delete a divoom_cJSON entity and all subentities. */
extern void   divoom_cJSON_Delete(divoom_cJSON *c);

/* Returns the number of items in an array (or object). */
extern int	  divoom_cJSON_GetArraySize(const divoom_cJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern divoom_cJSON *divoom_cJSON_GetArrayItem(const divoom_cJSON *array, int item);
/* Get item "string" from object. Case insensitive. */
extern divoom_cJSON *divoom_cJSON_GetObjectItem(const divoom_cJSON *object, const char *string);
int divoom_cJSON_GetObjectInt(const divoom_cJSON *object, const char *string);
void divoom_cJSON_GetObjectString(const divoom_cJSON *object, const char *string, char *buf);
/* buf_len includes the trailing terminator. */
void divoom_cJSON_GetObjectStringN(const divoom_cJSON *object, const char *string, char *buf, int buf_len);
double divoom_cJSON_GetObjectdouble(const divoom_cJSON *object, const char *string);

extern int divoom_cJSON_HasObjectItem(const divoom_cJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when divoom_cJSON_Parse() returns 0. 0 when divoom_cJSON_Parse() succeeds. */
extern const char *divoom_cJSON_GetErrorPtr(void);

/* These calls create a divoom_cJSON item of the appropriate type. */
extern divoom_cJSON *divoom_cJSON_CreateNull(void);
extern divoom_cJSON *divoom_cJSON_CreateTrue(void);
extern divoom_cJSON *divoom_cJSON_CreateFalse(void);
extern divoom_cJSON *divoom_cJSON_CreateBool(int b);
extern divoom_cJSON *divoom_cJSON_CreateNumber(double num);
extern divoom_cJSON *divoom_cJSON_CreateString(const char *string);
extern divoom_cJSON *divoom_cJSON_CreateArray(void);
extern divoom_cJSON *divoom_cJSON_CreateObject(void);

/* These utilities create an Array of count items. */
extern divoom_cJSON *divoom_cJSON_CreateIntArray(const int *numbers, int count);
extern divoom_cJSON *divoom_cJSON_CreateFloatArray(const float *numbers, int count);
extern divoom_cJSON *divoom_cJSON_CreateDoubleArray(const double *numbers, int count);
extern divoom_cJSON *divoom_cJSON_CreateStringArray(const char **strings, int count);

/* Append item to the specified array/object. */
extern void divoom_cJSON_AddItemToArray(divoom_cJSON *array, divoom_cJSON *item);
extern void	divoom_cJSON_AddItemToObject(divoom_cJSON *object, const char *string, divoom_cJSON *item);
extern void	divoom_cJSON_AddItemToObjectCS(divoom_cJSON *object, const char *string, divoom_cJSON *item);	/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the divoom_cJSON object */
/* Append reference to item to the specified array/object. Use this when you want to add an existing divoom_cJSON to a new divoom_cJSON, but don't want to corrupt your existing divoom_cJSON. */
extern void divoom_cJSON_AddItemReferenceToArray(divoom_cJSON *array, divoom_cJSON *item);
extern void	divoom_cJSON_AddItemReferenceToObject(divoom_cJSON *object, const char *string, divoom_cJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
extern divoom_cJSON *divoom_cJSON_DetachItemFromArray(divoom_cJSON *array, int which);
extern void   divoom_cJSON_DeleteItemFromArray(divoom_cJSON *array, int which);
extern divoom_cJSON *divoom_cJSON_DetachItemFromObject(divoom_cJSON *object, const char *string);
extern void   divoom_cJSON_DeleteItemFromObject(divoom_cJSON *object, const char *string);

/* Update array items. */
extern void divoom_cJSON_InsertItemInArray(divoom_cJSON *array, int which, divoom_cJSON *newitem); /* Shifts pre-existing items to the right. */
extern void divoom_cJSON_ReplaceItemInArray(divoom_cJSON *array, int which, divoom_cJSON *newitem);
extern void divoom_cJSON_ReplaceItemInObject(divoom_cJSON *object,const char *string,divoom_cJSON *newitem);

/* Duplicate a divoom_cJSON item */
extern divoom_cJSON *divoom_cJSON_Duplicate(const divoom_cJSON *item, int recurse);
/* Duplicate will create a new, identical divoom_cJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */

/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error. If not, then divoom_cJSON_GetErrorPtr() does the job. */
extern divoom_cJSON *divoom_cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);

extern void divoom_cJSON_Minify(char *json);

/* Macros for creating things quickly. */
#define divoom_cJSON_AddNullToObject(object,name) divoom_cJSON_AddItemToObject(object, name, divoom_cJSON_CreateNull())
#define divoom_cJSON_AddTrueToObject(object,name) divoom_cJSON_AddItemToObject(object, name, divoom_cJSON_CreateTrue())
#define divoom_cJSON_AddFalseToObject(object,name) divoom_cJSON_AddItemToObject(object, name, divoom_cJSON_CreateFalse())
#define divoom_cJSON_AddBoolToObject(object,name,b) divoom_cJSON_AddItemToObject(object, name, divoom_cJSON_CreateBool(b))
#define divoom_cJSON_AddNumberToObject(object,name,n) divoom_cJSON_AddItemToObject(object, name, divoom_cJSON_CreateNumber(n))
#define divoom_cJSON_AddStringToObject(object,name,s) divoom_cJSON_AddItemToObject(object, name, divoom_cJSON_CreateString(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define divoom_cJSON_SetIntValue(object,val) ((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))
#define divoom_cJSON_SetNumberValue(object,val) ((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))

/* Macro for iterating over an array */
#define divoom_cJSON_ArrayForEach(pos, head) for(pos = (head)->child; pos != NULL; pos = pos->next)

#ifdef __cplusplus
}
#endif

#endif
