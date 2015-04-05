/*****************************************************************************
 * getaddrinfo.h: getaddrinfo() implementation for OS/2 kLIBC
 *****************************************************************************
 * Copyright (C) 2015 M2X BV
 *
 * Authors: KO Myung-Hun <komh@chollian.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *****************************************************************************/

#ifndef DVBINFO_GETADDRINFO_H_
#define DVBINFO_GETADDRINFO_H_

#include <stddef.h>

/* GAI error codes */
#ifndef EAI_BADFLAGS
#define EAI_BADFLAGS    -1
#endif
#ifndef EAI_NONAME
#define EAI_NONAME      -2
#endif
#ifndef EAI_AGAIN
#define EAI_AGAIN       -3
#endif
#ifndef EAI_FAIL
#define EAI_FAIL        -4
#endif
#ifndef EAI_NODATA
#define EAI_NODATA      -5
#endif
#ifndef EAI_FAMILY
#define EAI_FAMILY      -6
#endif
#ifndef EAI_SOCKTYPE
#define EAI_SOCKTYPE    -7
#endif
#ifndef EAI_SERVICE
#define EAI_SERVICE     -8
#endif
#ifndef EAI_ADDRFAMILY
#define EAI_ADDRFAMILY  -9
#endif
#ifndef EAI_MEMORY
#define EAI_MEMORY      -10
#endif
#ifndef EAI_OVERFLOW
#define EAI_OVERFLOW    -11
#endif
#ifndef EAI_SYSTEM
#define EAI_SYSTEM      -12
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x01
#endif
#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 0x02
#endif
#ifndef NI_NOFQDN
#define NI_NOFQDN      0x04
#endif
#ifndef NI_NAMEREQD
#define NI_NAMEREQD    0x08
#endif
#ifndef NI_DGRAM
#define NI_DGRAM       0x10
#endif

#ifndef AI_PASSIVE
#define AI_PASSIVE     1
#endif
#ifndef AI_CANONNAME
#define AI_CANONNAME   2
#endif
#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 4
#endif

typedef int socklen_t;

struct addrinfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    size_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};

const char *gai_strerror (int errnum);
int getaddrinfo (const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo (struct addrinfo *res);
int getnameinfo (const struct sockaddr *sa, socklen_t salen, char *host,
                 int hostlen, char *serv, int servlen, int flags);
#endif
