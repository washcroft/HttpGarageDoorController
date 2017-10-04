/*
MIT License

Copyright (c) 2017 Warren Ashcroft

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "Utilities.h"

#ifdef ESP8266
static const unsigned char charmap[] = {
  '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
  '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
  '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
  '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
  '\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
  '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
  '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
  '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
  '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
  '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
  '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
  '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
  '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
  '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
  '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
  '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
  '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
  '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
  '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
  '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
  '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
  '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
  '\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
  '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
  '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
  '\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
  '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
  '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
  '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
  '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int
strncasecmp(const char *s1, const char *s2, size_t n)
{
  unsigned char u1, u2;

  for (; n != 0; --n) {
    u1 = (unsigned char) * s1++;
    u2 = (unsigned char) * s2++;

    if (charmap[u1] != charmap[u2]) {
      return charmap[u1] - charmap[u2];
    }

    if (u1 == '\0') {
      return 0;
    }
  }

  return 0;
}
#endif

bool strcomparator(char *k1, char *k2)
{
  return (strcmp(k1, k2) == 0);
}

void strtoupper(char *str)
{
  while (*str != '\0') {
    *str = toupper(*str);
    str++;
  }
}

char *strcasestr(const char *haystack, const char *needle)
{
  do {
    const char *h = haystack;
    const char *n = needle;

    while (tolower((unsigned char) *h) == tolower((unsigned char) *n) && *n) {
      h++;
      n++;
    }

    if (*n == 0) {
      return (char *) haystack;
    }
  } while (*haystack++);

  return 0;
}

size_t strextract(const char *src, const char *p1, const char *p2, char *dest, size_t size)
{
  size_t length = 0;
  char *start, *end;

  if (start = strstr(src, p1)) {
    start += strlen(p1);

    if (end = strstr(start, p2)) {
      length = (end - start);

      if (length > (size - 1)) {
        length = (size - 1);
      }

      memcpy(dest, start, length);
      dest[length] = 0;
    }
  }

  return length;
}

size_t strcaseextract(const char *src, const char *p1, const char *p2, char *dest, size_t size)
{
  size_t length = 0;
  char *start, *end;

  if (start = strcasestr(src, p1)) {
    start += strlen(p1);

    if (end = strcasestr(start, p2)) {
      length = (end - start);

      if (length > (size - 1)) {
        length = (size - 1);
      }

      memcpy(dest, start, length);
      dest[length] = 0;
    }
  }

  return length;
}

int striendswith(const char *str, const char *suffix)
{
  if (!str || !suffix) {
    return 0;
  }

  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);

  if (lensuffix > lenstr) {
    return 0;
  }

  return (strncasecmp(str + lenstr - lensuffix, suffix, lensuffix) == 0);
}

bool array_less_than(char *ptr_1, char *ptr_2)
{
  char check1;
  char check2;

  int i = 0;

  while (i < strlen(ptr_1)) {
    check1 = (char)ptr_1[i];

    if (strlen(ptr_2) < i) {
      return true;
    } else {
      check2 = (char)ptr_2[i];

      if (check2 > check1) {
        return true; // this character is greater
      }

      if (check2 < check1) {
        return false; // this character is less
      }

      // this character is equal, continue
      i++;
    }
  }

  return false;
}

void array_sort(char *a[], size_t size)
{
  int innerLoop ;
  int mainLoop ;

  for (mainLoop = 1; mainLoop < size; mainLoop++) {
    innerLoop = mainLoop;

    while (innerLoop  >= 1) {
      if (array_less_than(a[innerLoop], a[innerLoop - 1]) == 1) {
        char *temp;
        temp = a[innerLoop - 1];
        a[innerLoop - 1] = a[innerLoop];
        a[innerLoop] = temp;
      }

      innerLoop--;
    }
  }
}

char chartohex(char c)
{
  return (char)(c > 9 ? c + 55 : c + 48);
}

size_t percent_encode(const char *src, size_t src_length, char *dest, size_t dest_size)
{
  // RFC3986 compliant - returns the required buffer size when dest buffer is NULL

  char ch;
  size_t ssize = 0;
  size_t dsize = 0;

  while (*src) {
    ch = (char) * src;

    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || strchr("-._~", ch)) {
      if ((dest != NULL) && ((dsize + 1) > (dest_size - 1))) {
        break;
      }

      if (dest != NULL) {
        *dest++ = *src;
      }

      dsize++;

    } else {
      if ((dest != NULL) && ((dsize) + 3 > (dest_size - 1))) {
        break;
      }

      if (dest != NULL) {
        *dest++ = '%';
        *dest++ = chartohex((char)(ch >> 4));
        *dest++ = chartohex((char)(ch % 16));
      }

      dsize = dsize + 3;
    }

    src++;
    ssize++;

    if (ssize >= src_length) {
      break;
    }
  }

  dsize++;

  if (dest != NULL) {
    *dest = 0;
  }

  return dsize;
}

size_t percent_decode(const char *src, char *dest, size_t dest_size)
{
  // RFC3986 compliant - returns the required buffer size when dest buffer is NULL

  char ch;
  char p[2];
  size_t dsize = 0;

  while (*src) {
    if ((dest != NULL) && ((dsize + 1) > (dest_size - 1))) {
      break;
    }

    memset(p, 0, 2);
    ch = (char) * src;

    if (ch != '%') {
      if (dest != NULL) {
        *dest = ch;
      }
    } else {
      p[0] = *(++src);
      p[1] = *(++src);

      if (p[0] == NULL || p[1] == NULL) {
        break;
      }

      if (dest != NULL) {
        p[0] = p[0] - 48 - ((p[0] >= 'A') ? 7 : 0) - ((p[0] >= 'a') ? 32 : 0);
        p[1] = p[1] - 48 - ((p[1] >= 'A') ? 7 : 0) - ((p[1] >= 'a') ? 32 : 0);
        *dest = (char)(p[0] * 16 + p[1]);
      }
    }

    src++;
    dsize++;

    if (dest != NULL) {
      dest++;
    }
  }

  dsize++;

  if (dest != NULL) {
    *dest = 0;
  }

  return dsize;
}

void print_hex(char *data, size_t size)
{
  char tmp[16];

  for (int i = 0; i < size; i++) {
    sprintf(tmp, "%02X ", data[i]);
    Serial.print(tmp);
  }

  Serial.println();
}