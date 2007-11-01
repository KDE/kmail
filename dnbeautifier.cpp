/*
  Taken from cermanager/lib/cryptplug.cpp:

  this is a C++-ification of:
  GPGMEPLUG - an GPGME based cryptography plug-in following
              the common CRYPTPLUG specification.

  Copyright (C) 2001 by Klarälvdalens Datakonsult AB
  Copyright (C) 2002 g10 Code GmbH
  Copyright (C) 2004 Klarälvdalens Datakonsult AB

  GPGMEPLUG is free software; you can redistribute it and/or modify
  it under the terms of GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  GPGMEPLUG is distributed in the hope that it will be useful,
  it under the terms of GNU General Public License as published by
  the Free Software Foundation; version 2 of the License
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "dnbeautifier.h"

#include <kleo/oidmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/* some macros to replace ctype ones and avoid locale problems */
#define spacep(p)   (*(p) == ' ' || *(p) == '\t')
#define digitp(p)   (*(p) >= '0' && *(p) <= '9')
#define hexdigitp(a) (digitp (a)                     \
                      || (*(a) >= 'A' && *(a) <= 'F')  \
                      || (*(a) >= 'a' && *(a) <= 'f'))
/* the atoi macros assume that the buffer has only valid digits */
#define atoi_1(p)   (*(p) - '0' )
#define atoi_2(p)   ((atoi_1(p) * 10) + atoi_1((p)+1))
#define atoi_4(p)   ((atoi_2(p) * 100) + atoi_2((p)+2))
#define xtoi_1(p)   (*(p) <= '9'? (*(p)- '0'): \
                     *(p) <= 'F'? (*(p)-'A'+10):(*(p)-'a'+10))
#define xtoi_2(p)   ((xtoi_1(p) * 16) + xtoi_1((p)+1))

struct DnPair {
  char * key;
  char * value;
};

static char *
trim_trailing_spaces( char *string )
{
    char *p, *mark;

    for( mark = NULL, p = string; *p; p++ ) {
	if( isspace( *p ) ) {
	    if( !mark )
		mark = p;
	}
	else
	    mark = NULL;
    }
    if( mark )
	*mark = '\0' ;

    return string ;
}

/* Parse a DN and return an array-ized one.  This is not a validating
   parser and it does not support any old-stylish syntax; gpgme is
   expected to return only rfc2253 compatible strings. */
static const unsigned char *
parse_dn_part (DnPair *array, const unsigned char *string)
{
  const unsigned char *s, *s1;
  size_t n;
  char *p;

  /* parse attributeType */
  for (s = string+1; *s && *s != '='; s++)
    ;
  if (!*s)
    return NULL; /* error */
  n = s - string;
  if (!n)
    return NULL; /* empty key */
  p = (char*)malloc (n+1);


  memcpy (p, string, n);
  p[n] = 0;
  trim_trailing_spaces ((char*)p);
  // map OIDs to their names:
  for ( unsigned int i = 0 ; i < numOidMaps ; ++i )
    if ( !strcasecmp ((char*)p, oidmap[i].oid) ) {
      free( p );
      p = strdup (oidmap[i].name);
      break;
    }
  array->key = p;
  string = s + 1;

  if (*string == '#')
    { /* hexstring */
      string++;
      for (s=string; hexdigitp (s); s++)
        s++;
      n = s - string;
      if (!n || (n & 1))
        return NULL; /* empty or odd number of digits */
      n /= 2;
      array->value = p = (char*)malloc (n+1);


      for (s1=string; n; s1 += 2, n--)
        *p++ = xtoi_2 (s1);
      *p = 0;
   }
  else
    { /* regular v3 quoted string */
      for (n=0, s=string; *s; s++)
        {
          if (*s == '\\')
            { /* pair */
              s++;
              if (*s == ',' || *s == '=' || *s == '+'
                  || *s == '<' || *s == '>' || *s == '#' || *s == ';'
                  || *s == '\\' || *s == '\"' || *s == ' ')
                n++;
              else if (hexdigitp (s) && hexdigitp (s+1))
                {
                  s++;
                  n++;
                }
              else
                return NULL; /* invalid escape sequence */
            }
          else if (*s == '\"')
            return NULL; /* invalid encoding */
          else if (*s == ',' || *s == '=' || *s == '+'
                   || *s == '<' || *s == '>' || *s == '#' || *s == ';' )
            break;
          else
            n++;
        }

      array->value = p = (char*)malloc (n+1);


      for (s=string; n; s++, n--)
        {
          if (*s == '\\')
            {
              s++;
              if (hexdigitp (s))
                {
                  *p++ = xtoi_2 (s);
                  s++;
                }
              else
                *p++ = *s;
            }
          else
            *p++ = *s;
        }
      *p = 0;
    }
  return s;
}

/* Parse a DN and return an array-ized one.  This is not a validating
   parser and it does not support any old-stylish syntax; gpgme is
   expected to return only rfc2253 compatible strings. */
static DnPair *
parse_dn (const unsigned char *string)
{
  struct DnPair *array;
  size_t arrayidx, arraysize;

  if( !string )
    return NULL;

  arraysize = 7; /* C,ST,L,O,OU,CN,email */
  arrayidx = 0;
  array = (DnPair*)malloc ((arraysize+1) * sizeof *array);


  while (*string)
    {
      while (*string == ' ')
        string++;
      if (!*string)
        break; /* ready */
      if (arrayidx >= arraysize)
        { /* mutt lacks a real safe_realoc - so we need to copy */
          struct DnPair *a2;

          arraysize += 5;
          a2 = (DnPair*)malloc ((arraysize+1) * sizeof *array);
          for (unsigned int i=0; i < arrayidx; i++)
            {
              a2[i].key = array[i].key;
              a2[i].value = array[i].value;
            }
          free (array);
          array = a2;
        }
      array[arrayidx].key = NULL;
      array[arrayidx].value = NULL;
      string = parse_dn_part (array+arrayidx, string);
      arrayidx++;
      if (!string)
        goto failure;
      while (*string == ' ')
        string++;
      if (*string && *string != ',' && *string != ';' && *string != '+')
        goto failure; /* invalid delimiter */
      if (*string)
        string++;
    }
  array[arrayidx].key = NULL;
  array[arrayidx].value = NULL;
  return array;

 failure:
  for (unsigned i=0; i < arrayidx; i++)
    {
      free (array[i].key);
      free (array[i].value);
    }
  free (array);
  return NULL;
}

static void
add_dn_part( QCString& result, struct DnPair& dnPair )
{
  /* email hack */
  QCString mappedPart( dnPair.key );
  for ( unsigned int i = 0 ; i < numOidMaps ; ++i ){
    if( !strcasecmp( dnPair.key, oidmap[i].oid ) ) {
      mappedPart = oidmap[i].name;
      break;
    }
  }
  result.append( mappedPart );
  result.append( "=" );
  result.append( dnPair.value );
}

static int
add_dn_parts( QCString& result, struct DnPair* dn, const char* part )
{
  int any = 0;

  if( dn ) {
    for(; dn->key; ++dn ) {
      if( !strcmp( dn->key, part ) ) {
        if( any )
          result.append( "," );
        add_dn_part( result, *dn );
        any = 1;
      }
    }
  }
  return any;
}

static char*
reorder_dn( struct DnPair *dn,
            char** attrOrder = 0,
            const char* unknownAttrsHandling = 0 )
{
  struct DnPair *dnOrg = dn;

  /* note: The must parts are: CN, L, OU, O, C */
  const char* defaultpart[] = {
    "CN", "S", "SN", "GN", "T", "UID",
          "MAIL", "EMAIL", "MOBILE", "TEL", "FAX", "STREET",
    "L",  "PC", "SP", "ST",
    "OU",
    "O",
    "C",
    NULL
  };
  const char** stdpart = attrOrder ? ((const char**)attrOrder) : defaultpart;
  int any=0, any2=0, found_X_=0, i;
  QCString result;
  QCString resultUnknowns;

  /* find and save the non-standard parts in their original order */
  if( dn ){
    for(; dn->key; ++dn ) {
      for( i = 0; stdpart[i]; ++i ) {
        if( !strcmp( dn->key, stdpart[i] ) ) {
          break;
        }
      }
      if( !stdpart[i] ) {
        if( any2 )
          resultUnknowns.append( "," );
        add_dn_part( resultUnknowns, *dn );
        any2 = 1;
      }
    }
    dn = dnOrg;
  }

  /* prepend the unknown attrs if desired */
  if( unknownAttrsHandling &&
      !strcmp(unknownAttrsHandling, "PREFIX")
      && *resultUnknowns ){
    result.append( resultUnknowns );
    any = 1;
  }else{
    any = 0;
  }

  /* add standard parts */
  for( i = 0; stdpart[i]; ++i ) {
    dn = dnOrg;
    if( any ) {
      result.append( "," );
    }
    if( any2 &&
      !strcmp(stdpart[i], "_X_") &&
      unknownAttrsHandling &&
      !strcmp(unknownAttrsHandling, "INFIX") ){
      if ( !resultUnknowns.isEmpty() ) {
        result.append( resultUnknowns );
        any = 1;
      }
      found_X_ = 1;
    }else{
      any = add_dn_parts( result, dn, stdpart[i] );
    }
  }

  /* append the unknown attrs if desired */
  if( !unknownAttrsHandling ||
      !strcmp(unknownAttrsHandling, "POSTFIX") ||
      ( !strcmp(unknownAttrsHandling, "INFIX") && !found_X_ ) ){
    if( !resultUnknowns.isEmpty() ) {
      if( any ){
        result.append( "," );
      }
      result.append( resultUnknowns );
    }
  }

  char* cResult = (char*)malloc( (result.length()+1)*sizeof(char) );
  if( result.isEmpty() )
    *cResult = 0;
  else
    strcpy( cResult, result );
  return cResult;
}

QString KMail::DNBeautifier::reorderDN(const QString &attr_string )
{
  DnPair* a = parse_dn( (const unsigned char*)attr_string.latin1() );
  return QString::fromLatin1( reorder_dn( a ) );
}
