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

/* cJSON */
/* JSON parser in C. */
//#undef _CRTDBG_MAP_ALLOC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "cJSON.h"

#include <stdarg.h>

static const char *ep;

//#define  MALLOC_TEST 

#define  DEFAULT_BLOCK_JSON_SIZE	 (20*1024)

typedef struct tagJsonMalloc JsonMalloc_T;

typedef struct tagJsonMalloc
{
	int				curLen;
	int				totalLen;
	void			*pMem;
	JsonMalloc_T	*pNext;
}tagJsonMalloc_T;

static cJSON *cJSON_New_Item(void);
static cJSON *create_reference(cJSON *item);

#ifdef MALLOC_TEST
static JsonMalloc_T *pMemBlock = NULL;
static JsonMalloc_T	*jsonMallocCreate(JsonMalloc_T **ppHead, int nMax, int cbElement);
static JsonMalloc_T	*jsonMallocGetLast(JsonMalloc_T *pHead);
static void			 jsonMallocFree(JsonMalloc_T **pSelf);

static JsonMalloc_T *jsonMallocCreate(JsonMalloc_T **ppHead, int nMax, int cbElement)
{
    JsonMalloc_T *p		= NULL;
	JsonMalloc_T *pHead	= NULL;
	
    if (NULL == ppHead || nMax <= 0 || cbElement <= 0)
    {
		return NULL;
    }
    
    p = (JsonMalloc_T*)malloc(sizeof(JsonMalloc_T));
	if(NULL == p)
	{
		return p;
	}
	memset(p, 0, sizeof(JsonMalloc_T));
	
	p->curLen	= 0;
	p->pNext	= NULL;
	p->totalLen = nMax;
	p->pMem		= (void *)malloc(nMax);
	if (NULL == p->pMem)
	{
		return p;
	}
	memset(p->pMem, 0, nMax);
//	BYS_TRACE_INFO("jsonMallocCreate p->pMem = 0x%x", p->pMem);

	if (NULL == *ppHead)
	{
		*ppHead = p;
	}
	else
	{
		pHead = *ppHead;

		while(pHead->pNext)
		{
			pHead = pHead->pNext;
		}
		pHead->pNext = p;
	}
	
//	BYS_TRACE_INFO("jsonMallocCreate p = 0x%x", p);

    return p;
}

static JsonMalloc_T *jsonMallocGetLast(JsonMalloc_T *pHead)
{
    JsonMalloc_T *p		= pHead;
	
    if (NULL == pHead)
    {
		return NULL;
    }
		
	while(p->pNext)
	{
		p = p->pNext;
	}

    return p;
}

static void jsonMallocFree(JsonMalloc_T **pSelf)
{
	JsonMalloc_T		*p		= NULL;
	JsonMalloc_T		*pTmp	= NULL;
	JsonMalloc_T		*pNext  = NULL;
	
    if(NULL == pSelf || NULL == *pSelf)
	{
		return;
	}
	
	p = *pSelf;

    while (p != NULL)
	{
		pTmp   = p;
		pNext  = p->pNext;
		free(pTmp->pMem);
		free(pTmp);
		p = pNext;
	}
	*pSelf = NULL;
}
#endif

const char *cJSON_GetErrorPtr() 
{
	return ep;
}

static int cJSON_strcasecmp(const char *s1,const char *s2)
{
	if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
	for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0)	return 0;
	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

//static void *(*cJSON_malloc)(size_t sz) = malloc;
//static void (*cJSON_free)(void *ptr) = free;

static void *cJSON_malloc(size_t sz) 
{
//	BYS_TRACE_INFO("cJSON_malloc");
	void *ptr;
	
	ptr = malloc(sz);
	if (ptr)
	{
		memset(ptr, 0, sz);
	}
	return ptr;
}

static void cJSON_free(void *ptr)
{
//	BYS_TRACE_INFO("cJSON_free");
	free(ptr);
}

static void *cJSON_buffer_malloc(size_t sz) 
{
#ifdef MALLOC_TEST
	JsonMalloc_T	*p		= jsonMallocGetLast(pMemBlock);
	char			*pTest	= NULL;
	void			*pStr	= NULL;

	if(0 != (sz&1))
	{
		sz++;
	}
//	BYS_TRACE_INFO("cJSON_buffer_malloc sz = %d", sz);

	if (NULL == pMemBlock || p->curLen+(int)sz >= p->totalLen)
	{
	//	BYS_TRACE_INFO("cJSON_buffer_malloc pMemBlock = 0x%x", pMemBlock);
		p = jsonMallocCreate(&pMemBlock, DEFAULT_BLOCK_JSON_SIZE, 1);
	}

//	BYS_TRACE_INFO("cJSON_buffer_malloc p->curLen = %d", p->curLen);

	pTest = (char *)p->pMem;
	
	pStr = pTest+p->curLen;
	p->curLen += sz;
	
//	BYS_TRACE_INFO("cJSON_buffer_malloc pStr = 0x%x", pStr);
	return pStr;
#else
	void *ptr;

	ptr = malloc(sz);
	if (ptr)
	{
		memset(ptr, 0, sz);
	}
	return ptr;
#endif
}

static void cJSON_buffer_free(void *ptr)
{
//	BYS_TRACE_INFO("cJSON_buffer_free");
#ifndef MALLOC_TEST
	free(ptr);
#endif
}

static char* cJSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)cJSON_malloc(len))) return 0;/*lint !e820*/
      memcpy(copy,str,len);
      return copy;
}
#if 0

void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    if (!hooks) { /* Reset hooks */
        cJSON_malloc = malloc;
        cJSON_free = free;
        return;
    }

	cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
	cJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
}

#endif

/* Internal constructor. */
static cJSON *cJSON_New_Item(void)
{
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
	return node;
}


/* Internal constructor. */
static cJSON *cJSON_Buffer_New_Item(void)
{
	cJSON* node = (cJSON*)cJSON_buffer_malloc(sizeof(cJSON));

	return node;
}

/* Delete a cJSON structure. */
void cJSON_Delete(cJSON *c)
{
	cJSON *next;

//	BYS_TRACE_INFO("cJSON_Delete");

	while (c)
	{
		next=c->next;
		if (!(c->type&cJSON_IsReference) && c->child) cJSON_Delete(c->child);
		if (!(c->type&cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
		if (c->string) cJSON_free(c->string);
		cJSON_free(c);
		c=next;
	}
}

void cJSON_buffer_Delete(cJSON *c)
{
#ifdef MALLOC_TEST
//	BYS_TRACE_INFO("cJSON_buffer_Delete pMemBlock = 0x%x", pMemBlock);
	jsonMallocFree(&pMemBlock);
#else
	cJSON *next;
	while (c)
	{
		next=c->next;
		if (!(c->type&cJSON_IsReference) && c->child) cJSON_Delete(c->child);
		if (!(c->type&cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
		if (c->string) cJSON_free(c->string);
		cJSON_free(c);
		c=next;
	}
#endif
}

/* Parse the input text to generate a number, and populate the result into item. */
const char *parse_number(cJSON *item, const char *num)
{
	double n	 = 0;
	double sign  = 1;
	double scale = 0;

	int subscale = 0;
	int signsubscale = 1;

//	BYS_TRACE_INFO("parse_number *num = 0x%x", *num);

	/* Could use sscanf for this? */
	if (*num=='-')
	{
		sign=-1;
		num++;	/* Has sign? */
	}

//	BYS_TRACE_INFO("parse_number sign1 = 0x%x, n1 = 0x%x", sign, n);
//	BYS_TRACE_INFO("parse_number sign1 = %f", sign);
//	BYS_TRACE_INFO("parse_number n1 = %f", n);

	if (*num=='0')
	{
		num++;			/* is zero */
	}
	if (*num>='1' && *num<='9')
	{
		do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
	}

	if (*num=='.')
	{
		num++;
		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');
	}	/* Fractional part? */

	if (*num=='e' || *num=='E')		/* Exponent? */
	{
		num++;
		if (*num=='+')
		{
			num++;	
		}
		else if (*num=='-')
		{
			signsubscale = -1;
			num++;		/* With sign? */
		}
		while (*num>='0' && *num<='9')
		{
			subscale = (subscale*10)+(*num++ - '0');	/* Number? */
		}
	}
//	BYS_TRACE_INFO("parse_number sign2 = %f", sign);
//	BYS_TRACE_INFO("parse_number n  = %f", n);

	n=sign*n;	/* number = +/- number.fraction * 10^+/- exponent */
	if ((scale+(subscale*signsubscale)) > 0) /*lint !e790*/
	{
		//BYS_TRACE_WARNING("parse_number use pow");
		n = n*pow(10.0,(scale+subscale*signsubscale));/*lint !e718 !e628*//*lint !e746*//*lint !e790*/
	}
	else if(scale < 0)
	{
		double m = 0.1;

		do n = n*m,scale ++; while(scale < 0);
		item->type = cJson_Double;
		item->valuedouble = n;
		

		return num;
	}

//	BYS_TRACE_INFO("parse_number item->valuedouble = %f", n);
	item->valuedouble = n;

	if (n<=INT_MAX && n>=INT_MIN)
	{
		item->valueint = (int)n;
	}
//	BYS_TRACE_INFO("parse_number item->valueint = %d", item->valueint);

	item->type = cJSON_Number;

	return num;
}

/* Render the number nicely from the given item into a string. */
static char *print_number(cJSON *item)
{
	char *str;
	double d=item->valuedouble;
//	BYS_TRACE_INFO("in print_number");

	if (cjson_fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		str=(char*)cJSON_buffer_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
		if (str) sprintf(str,"%d",item->valueint);
	}
	else
	{
		str=(char*)cJSON_buffer_malloc(64);	/* This is a nice tradeoff. */
		if (str)
		{
			if (cjson_fabs(floor(d)-d)<=DBL_EPSILON)			sprintf(str,"%.0f",d);/*lint !e666*//*lint !e718*//*lint !e746*/
			else if (cjson_fabs(d)<1.0e-6 || cjson_fabs(d)>1.0e9)	sprintf(str,"%e",d);
			else										sprintf(str,"%f",d);
		}
	}
	return str;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
const char *parse_string(cJSON *item,const char *str)
{
	const char *ptr=str+1;
	char *ptr2;
	char *out;
	int len=0;
	unsigned uc;

//	BYS_TRACE_INFO("in parse_string");
	if (*str!='\"') {ep=str;return 0;}	/* not a string! */
	
	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */
	
	out=(char*)cJSON_buffer_malloc(len+1);	/* This is how long we need for the string, roughly. */
	if (!out) return 0;
	
	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
				case 'b': *ptr2++='\b';	break;
				case 'f': *ptr2++='\f';	break;
				case 'n': *ptr2++='\n';	break;
				case 'r': *ptr2++='\r';	break;
				case 't': *ptr2++='\t';	break;
				case 'u':	 /* transcode utf16 to utf8. DOES NOT SUPPORT SURROGATE PAIRS CORRECTLY. */
					sscanf(ptr+1,"%4x",&uc);	/* get the unicode char. */
					len=3;if (uc<0x80) len=1;else if (uc<0x800) len=2;ptr2+=len;
					
					switch (len) {
						case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;/*lint !e616*//*lint !e825*/
						case 1: *--ptr2 =(uc | firstByteMark[len]);/*lint !e616*//*lint !e825*/
						default:/*lint !e825*/
							break;
					}
					ptr2+=len;ptr+=4;
					break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;
	item->valuestring=out;
	item->type=cJSON_String;
	return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str)
{
	const char *ptr;char *ptr2,*out;int len=0;unsigned char token;
//	BYS_TRACE_INFO("in print_string_ptr");
	if (!str) return cJSON_strdup("");
	ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}/*lint !e820*/
	
	out=(char*)cJSON_buffer_malloc(len+3);
	if (!out) return 0;

	ptr2=out;ptr=str;
	*ptr2++='\"';
	while (*ptr)
	{
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
		else
		{
			*ptr2++='\\';
			switch (token=*ptr++)
			{
				case '\\':	*ptr2++='\\';	break;
				case '\"':	*ptr2++='\"';	break;
				case '\b':	*ptr2++='b';	break;
				case '\f':	*ptr2++='f';	break;
				case '\n':	*ptr2++='n';	break;
				case '\r':	*ptr2++='r';	break;
				case '\t':	*ptr2++='t';	break;
				default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* escape and print */
			}
		}
	}
	*ptr2++='\"';*ptr2++=0;
	return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
static char *print_string(cJSON *item)	{return print_string_ptr(item->valuestring);}

/* Predeclare these prototypes. */
//static const char *parse_value(cJSON *item,const char *value);
static char *print_value(cJSON *item,int depth,int fmt);
static const char *parse_array(cJSON *item,const char *value);
static char *print_array(cJSON *item,int depth,int fmt);
static const char *parse_object(cJSON *item,const char *value);
static char *print_object(cJSON *item,int depth,int fmt);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in)
{
	while (in && *in && (unsigned char)*in<=32)
	{
		in++; 
	}
	return in;
}

/* Parse an object - create a new root, and populate. */
cJSON *cJSON_Parse(const char *value)
{
	cJSON *c=cJSON_Buffer_New_Item();
	ep=0;
	if (!c) return 0;       /* memory fail */

	if (!parse_value(c,skip(value)))
	{
		cJSON_buffer_Delete(c);
		return 0;
	}
	return c;
}

/* Render a cJSON item/entity/structure to text. */
char *cJSON_Print(cJSON *item)				{return print_value(item,0,1);}
char *cJSON_PrintUnformatted(cJSON *item)	{return print_value(item,0,0);}

/* Parser core - when encountering text, process appropriately. */
const char *parse_value(cJSON *item,const char *value)
{
	if (!value)
	{
		return 0;	/* Fail on null. */
	}

	if (!strncmp(value,"null",4))	
	{ 
		item->type=cJSON_NULL;  
		return value+4; 
	}

	if (!strncmp(value,"false",5))
	{ 
		item->type=cJSON_False;
		return value+5; 
	}

	if (!strncmp(value,"true",4))
	{ 
		item->type=cJSON_True;
		item->valueint=1;
		return value+4; 
	}
	if (*value=='\"')
	{
		return parse_string(item,value); 
	}
	if (*value=='-' || (*value>='0' && *value<='9'))
	{ 
		return parse_number(item,value); 
	}
	if (*value=='[')				
	{ 
		return parse_array(item,value); 
	}
	if (*value=='{')				
	{
		return parse_object(item,value); 
	}

	ep=value;
	return 0;	/* failure. */
}

/* Render a value to text. */
static char *print_value(cJSON *item,int depth,int fmt)
{
	char *out=0;
	if (!item) return 0;
	switch ((item->type)&255)
	{
		case cJSON_NULL:	
			out=cJSON_strdup("null");	
			break;
		case cJSON_False:	
			out=cJSON_strdup("false");
			break;
		case cJSON_True:	
			out=cJSON_strdup("true"); 
			break;
		case cJSON_Number:	
			out=print_number(item);
			break;
		case cJSON_String:	
			out=print_string(item);
			break;
		case cJSON_Array:
			out=print_array(item,depth,fmt);
			break;
		case cJSON_Object:	
			out=print_object(item,depth,fmt);
			break;
		default:
			break;
	}
	return out;
}

/* Build an array from input text. */
static const char *parse_array(cJSON *item,const char *value)
{
	cJSON *child;
	cJSON *new_item;

//	BYS_TRACE_INFO("parse_array 1");

	if (*value!='[')
	{
		ep=value;
		return 0;
	}	/* not an array! */

	item->type=cJSON_Array;
	value=skip(value+1);

	if (*value==']')
	{
		return value+1;	/* empty array. */
	}

//	BYS_TRACE_INFO("parse_array 2");

	child = cJSON_Buffer_New_Item();
	item->child = child;
	if (!item->child)
	{
		return 0;		 /* memory fail */
	}

#if CJSON_SUPPORT_PARENT_NODE
	child->parent = item;
#endif

	value = skip(parse_value(child, skip(value)));	/* skip any spacing, get the value. */
	if (!value)
	{
		return 0;
	}

//	BYS_TRACE_INFO("parse_array 3");

	while (*value==',')
	{
		new_item = cJSON_Buffer_New_Item();
		if (!new_item)
		{
			return 0; 	/* memory fail */
		}
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		value = skip(parse_value(child, skip(value+1)));
		if (!value)
		{
			return 0;	/* memory fail */
		}
	}

//	BYS_TRACE_INFO("parse_array 5");

	if (*value==']')
	{
		return value+1;	/* end of array */
	}

	ep=value;
	return 0;	/* malformed. */
}

/* Render an array to text */
static char *print_array(cJSON *item,int depth,int fmt)
{
	char **entries;
	char *out=0,*ptr,*ret;int len=5;
	cJSON *child=item->child;
	int numentries=0,i=0,fail=0;
	
	/* How many entries in the array? */
	while (child) numentries++,child=child->next;
	/* Allocate an array to hold the values for each */
	entries=(char**)cJSON_buffer_malloc(numentries*(int)sizeof(char*));
	if (!entries) return 0;
	/* Retrieve all the results: */
	child=item->child;
	while (child && !fail)
	{
		ret=print_value(child,depth+1,fmt);
		entries[i++]=ret;
		if (ret) len+=(int)strlen(ret)+2+(fmt?1:0); else fail=1;
		child=child->next;
	}
	
	/* If we didn't fail, try to malloc the output string */
	if (!fail) out=(char*)cJSON_buffer_malloc(len);
	/* If that fails, we fail. */
	if (!out) fail=1;

	/* Handle failure. */
	if (fail)
	{
		for (i=0;i<numentries;i++) if (entries[i]) cJSON_buffer_free(entries[i]);
		cJSON_buffer_free(entries);
		return 0;
	}
	
	/* Compose the output array. */
	*out='[';/*lint !e613*/
	ptr=out+1;*ptr=0;/*lint !e613*/
	for (i=0;i<numentries;i++)
	{
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
		cJSON_buffer_free(entries[i]);
	}
	cJSON_buffer_free(entries);
	*ptr++=']';*ptr++=0;
	return out;	
}

/* Build an object from the text. */
static const char *parse_object(cJSON *item,const char *value)
{
	cJSON *child;
	cJSON *new_item;

//	BYS_TRACE_INFO("parse_object 1");
	if (*value!='{')
	{
		ep=value;
		return 0;
	}	/* not an object! */
	
	item->type = cJSON_Object;
	value = skip(value+1);
	if (*value=='}')
	{
		return value+1;	/* empty array. */
	}
	
	child = cJSON_Buffer_New_Item();
	item->child = child;
	if (!item->child)
	{
		return 0;
	}

#if CJSON_SUPPORT_PARENT_NODE
	child->parent = item;
#endif

//	BYS_TRACE_INFO("parse_object 2");
	value = skip(parse_string(child, skip(value)));
	if (!value) 
	{
		return 0;
	}

	child->string = child->valuestring;
	child->valuestring = 0;
	if (*value!=':')
	{
		ep=value;
		return 0;
	}	/* fail! */
//	BYS_TRACE_INFO("parse_object 3");
	value = skip(parse_value(child, skip(value+1)));	/* skip any spacing, get the value. */
	if (!value)
	{
		return 0;
	}
//	BYS_TRACE_INFO("parse_object 4");
	while (*value==',')
	{
		new_item = cJSON_Buffer_New_Item();
		if (!(new_item))
		{
			return 0; /* memory fail */
		}
//		BYS_TRACE_INFO("parse_object tt 2");
		child->next = new_item;
//		BYS_TRACE_INFO("parse_object tt 3 child = 0x%x", child);
//		BYS_TRACE_INFO("parse_object tt 3 new_item = 0x%x", new_item);
//		BYS_TRACE_INFO("parse_object tt 3 new_item->prev = 0x%x", new_item->prev);
		new_item->prev = child;
//		BYS_TRACE_INFO("parse_object tt 4");
		child = new_item;
//		BYS_TRACE_INFO("parse_object tt 5");
		value = skip(parse_string(child, skip(value+1)));
//		BYS_TRACE_INFO("parse_object tt 6");
		if (!value)
		{
			return 0;
		}
		child->string = child->valuestring;
		child->valuestring = 0;
		if (*value!=':')
		{
			ep=value;
			return 0;
		}	/* fail! */
		value=skip(parse_value(child, skip(value+1)));	/* skip any spacing, get the value. */
		if (!value)
		{
			return 0;
		}
	}

//	BYS_TRACE_INFO("parse_object 6");
	if (*value=='}')
	{
		return value+1;	/* end of array */
	}
	ep = value;
	return 0;	/* malformed. */
}

/* Render an object to text. */
static char *print_object(cJSON *item,int depth,int fmt)
{
	char **entries=0,**names=0;
	char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
	cJSON *child=item->child;
	int numentries=0,fail=0;
	/* Count the number of entries. */
	while (child) numentries++,child=child->next;
	/* Allocate space for the names and the objects */
	entries=(char**)cJSON_buffer_malloc(numentries*(int)sizeof(char*));
	if (!entries) return 0;
	names=(char**)cJSON_buffer_malloc(numentries*(int)sizeof(char*));
	if (!names) {cJSON_buffer_free(entries);return 0;}

	/* Collect all the results into our arrays: */
	child=item->child;depth++;if (fmt) len+=depth;
	while (child)
	{
		names[i]=str=print_string_ptr(child->string);
		entries[i++]=ret=print_value(child,depth,fmt);
		if (str && ret) len+=(int)strlen(ret)+(int)strlen(str)+2+(fmt?2+depth:0); else fail=1;
		child=child->next;
	}
	
	/* Try to allocate the output string */
	if (!fail) out=(char*)cJSON_buffer_malloc(len);
	if (!out) fail=1;

	/* Handle failure */
	if (fail)
	{
		for (i=0;i<numentries;i++) {if (names[i]) cJSON_buffer_free(names[i]);if (entries[i]) cJSON_buffer_free(entries[i]);}
		cJSON_buffer_free(names);cJSON_buffer_free(entries);
		return 0;
	}
	
	/* Compose the output: */
	*out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;/*lint !e613*/
	for (i=0;i<numentries;i++)
	{
		if (fmt) for (j=0;j<depth;j++) *ptr++='\t';
		strcpy(ptr,names[i]);ptr+=strlen(names[i]);
		*ptr++=':';if (fmt) *ptr++='\t';
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		if (i!=numentries-1) *ptr++=',';
		if (fmt) *ptr++='\n';*ptr=0;
		cJSON_buffer_free(names[i]);cJSON_buffer_free(entries[i]);
	}
	
	cJSON_buffer_free(names);
	cJSON_buffer_free(entries);
	if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t';
	*ptr++='}';*ptr++=0;
	return out;	
}

/* Get Array size/item / object item. */
int    cJSON_GetArraySize(cJSON *array)							
{
	cJSON *c=array->child;
	int i=0;
	while(c)
		i++,c=c->next;
	return i;
}

cJSON *cJSON_GetArrayItem(cJSON *array,int item)			
{
	cJSON *c = array->child;  
	while (c && item>0) 
		item--,c=c->next; 
	return c;
}
cJSON *cJSON_GetObjectItem(cJSON *object,const char *string)	
{
	cJSON *c; 

	if (NULL == object)
	{
		return NULL;
	}

	c=object->child; 
	while (c && cJSON_strcasecmp(c->string,string))
	{ 
		c=c->next;
	}
	return c;
}

/* Utility for array list handling. */
static void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;}
/* Utility for handling references. */
static cJSON *create_reference(cJSON *item)
{
	cJSON *ref=cJSON_New_Item();
	if (!ref) return 0;
	memcpy(ref,item,sizeof(cJSON));
	ref->string=0;
	ref->type|=cJSON_IsReference;
	ref->next=ref->prev=0;
	return ref;
}

/* Add item to array/object. */
void   cJSON_AddItemToArray(cJSON *array, cJSON *item)						{cJSON *c=array->child;if (!item) return; if (!c) {array->child=item;} else {while (c && c->next) c=c->next; suffix_object(c,item);}}
void   cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item)	
{
	if (!item) 
		return; 
	if (item->string) 
		cJSON_free(item->string);
	item->string=cJSON_strdup(string);
	cJSON_AddItemToArray(object,item);
}
void	cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item)						{cJSON_AddItemToArray(array,create_reference(item));}
void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item)	{cJSON_AddItemToObject(object,string,create_reference(item));}

cJSON *cJSON_DetachItemFromArray(cJSON *array,int which)			{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return 0;
	if (c->prev) c->prev->next=c->next;if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;c->prev=c->next=0;return c;}
void   cJSON_DeleteItemFromArray(cJSON *array,int which)			{cJSON_Delete(cJSON_DetachItemFromArray(array,which));}
cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string) {int i=0;cJSON *c=object->child;while (c && cJSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return cJSON_DetachItemFromArray(object,i);return 0;}
void   cJSON_DeleteItemFromObject(cJSON *object,const char *string) {cJSON_Delete(cJSON_DetachItemFromObject(object,string));}

/* Replace array/object items with new ones. */
void   cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem)		{cJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return;
	newitem->next=c->next;newitem->prev=c->prev;if (newitem->next) newitem->next->prev=newitem;
	if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;c->next=c->prev=0;cJSON_Delete(c);}
void   cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem){int i=0;cJSON *c=object->child;while(c && cJSON_strcasecmp(c->string,string))i++,c=c->next;if(c){newitem->string=cJSON_strdup(string);cJSON_ReplaceItemInArray(object,i,newitem);}}

/* Create basic types: */
cJSON *cJSON_CreateNull()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_NULL;return item;}
cJSON *cJSON_CreateTrue()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_True;return item;}
cJSON *cJSON_CreateFalse()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_False;return item;}
cJSON *cJSON_CreateBool(int b)					{cJSON *item=cJSON_New_Item();if(item)item->type=b?cJSON_True:cJSON_False;return item;}
cJSON *cJSON_CreateNumber(double num)			{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;}
cJSON *cJSON_CreateString(const char *string)	{cJSON *item=cJSON_New_Item();if(item){item->type=cJSON_String;item->valuestring=cJSON_strdup(string);}return item;}
cJSON *cJSON_CreateArray()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Array;return item;}
cJSON *cJSON_CreateObject()						{cJSON *item=cJSON_New_Item();if(item)item->type=cJSON_Object;return item;}

/* Create Arrays: */
cJSON *cJSON_CreateIntArray(int *numbers,int count)				{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber((double)numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateFloatArray(float *numbers,int count)			{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber((double)numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}/*modified by zj*/
cJSON *cJSON_CreateDoubleArray(double *numbers,int count)		{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
cJSON *cJSON_CreateStringArray(const char **strings,int count)	{int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();for(i=0;a && i<count;i++){n=cJSON_CreateString(strings[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
