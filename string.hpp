#ifndef H_STRING
#define H_STRING

#include <stdint.h>
#include <stddef.h>
#include "linked_list.hpp"

typedef LinkedList<uint8_t> String;

String StrFromCStr(const char* str);
String StrFromArray(char* str, size_t size);
bool StrStartsWith(String str, String startswith);
bool StrHas(String str, char what);
bool StrEqualsWith(String x, String y);
bool CStrEqualsWith(char* x, size_t xSize, char* y, size_t ySize);
String StrFormat(const char* Format, ...);
String StrAppend(String x, String y);

#endif // H_STRING