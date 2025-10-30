#include "Utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

char* ToUpper(const char* str)
{
  if (str == NULL) return NULL;
  size_t len = strlen(str);
  char* result = (char*)malloc(len + 1);
  for (size_t i = 0; i < len; i++)
  {
    result[i] = toupper(str[i]);
  }
  result[len] = '\0';
  return result;
}

char* ToLower(const char* str)
{
  if (str == NULL) return NULL;
  size_t len = strlen(str);
  char* result = (char*)malloc(len + 1);
  for (size_t i = 0; i < len; i++)
  {
    result[i] = tolower(str[i]);
  }
  result[len] = '\0';
  return result;
}

char* Trim(const char* str)
{
  if (str == NULL) return NULL;
  const char* start = str;
  while (isspace(*start)) start++;
  const char* end = str + strlen(str) - 1;
  while (end > start && isspace(*end)) end--;
  size_t len = end - start + 1;
  char* result = (char*)malloc(len + 1);
  strncpy_s(result, len + 1, start, len);
  result[len] = '\0';
  return result;
}

char* HashString(const char* str)
{
  if (str == NULL) return NULL;
  unsigned int hash = 5381;
  int c;
  while ((c = *str++))
  {
    hash = ((hash << 5) + hash) + c;
  }
  char* result = (char*)malloc(11); // enough for 32-bit unsigned int
  snprintf(result, 11, "%u", hash);
  return result;
}