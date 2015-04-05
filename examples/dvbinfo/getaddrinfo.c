/*****************************************************************************
 * getaddrinfo.c: getaddrinfo() implementation for OS/2 kLIBC
 *****************************************************************************
 * Copyright (C) 2005 the VideoLAN team
 * Copyright (C) 2002-2007 Rémi Denis-Courmont
 * Copyright (C) 2011-2015 KO Myung-Hun
 * Copyright (C) 2015 M2X BV
 *
 * Authors: KO Myung-Hun <komh@chollian.net>
 *          Rémi Denis-Courmont <rem # videolan.org>
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

#include "getaddrinfo.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef TCPV40HDRS
#define HAVE_SA_LEN
#endif

static const struct
{
    int        code;
    const char msg[41];
} gai_errlist[] =
{
    { 0,              "Error 0" },
    { EAI_BADFLAGS,   "Invalid flag used" },
    { EAI_NONAME,     "Host or service not found" },
    { EAI_AGAIN,      "Temporary name service failure" },
    { EAI_FAIL,       "Non-recoverable name service failure" },
    { EAI_NODATA,     "No data for host name" },
    { EAI_FAMILY,     "Unsupported address family" },
    { EAI_SOCKTYPE,   "Unsupported socket type" },
    { EAI_SERVICE,    "Incompatible service for socket type" },
    { EAI_ADDRFAMILY, "Unavailable address family for host name" },
    { EAI_MEMORY,     "Memory allocation failure" },
    { EAI_OVERFLOW,   "Buffer overflow" },
    { EAI_SYSTEM,     "System error" },
    { 0,              "" },
};

static const char gai_unknownerr[] = "Unrecognized error number";

/****************************************************************************
 * Converts an EAI_* error code into human readable english text.
 ****************************************************************************/
const char *gai_strerror (int errnum)
{
    unsigned i;

    for (i = 0; *gai_errlist[i].msg; i++)
        if (errnum == gai_errlist[i].code)
            return gai_errlist[i].msg;

    return gai_unknownerr;
}

#define _NI_MASK (NI_NUMERICHOST|NI_NUMERICSERV|NI_NOFQDN|NI_NAMEREQD|\
                  NI_DGRAM)
/*
 * getnameinfo() non-thread-safe IPv4-only implementation,
 * Address-family-independent address to hostname translation
 * (reverse DNS lookup in case of IPv4).
 *
 * This is meant for use on old IP-enabled systems that are not IPv6-aware,
 * and probably do not have getnameinfo(), but have the old gethostbyaddr()
 * function.
 */
int
getnameinfo (const struct sockaddr *sa, socklen_t salen,
             char *host, int hostlen, char *serv, int servlen, int flags)
{
    if (((size_t)salen < sizeof (struct sockaddr_in))
     || (sa->sa_family != AF_INET))
        return EAI_FAMILY;
    else if (flags & (~_NI_MASK))
        return EAI_BADFLAGS;
    else
    {
        const struct sockaddr_in *addr;

        addr = (const struct sockaddr_in *)sa;

        if (host != NULL)
        {
            /* host name resolution */
            if (!(flags & NI_NUMERICHOST))
            {
                if (flags & NI_NAMEREQD)
                    return EAI_NONAME;
            }

            /* inet_ntoa() is not thread-safe, do not use it */
            uint32_t ipv4 = ntohl (addr->sin_addr.s_addr);

            if (snprintf (host, hostlen, "%u.%u.%u.%u", ipv4 >> 24,
                          (ipv4 >> 16) & 0xff, (ipv4 >> 8) & 0xff,
                          ipv4 & 0xff) >= (int)hostlen)
                return EAI_OVERFLOW;
        }

        if (serv != NULL)
        {
            if (snprintf (serv, servlen, "%u",
                          (unsigned int)ntohs (addr->sin_port)) >= (int)servlen)
                return EAI_OVERFLOW;
        }
    }
    return 0;
}

#define _AI_MASK (AI_PASSIVE|AI_CANONNAME|AI_NUMERICHOST)
/*
 * Converts the current herrno error value into an EAI_* error code.
 * That error code is normally returned by getnameinfo() or getaddrinfo().
 */
static int
gai_error_from_herrno (void)
{
    switch (h_errno)
    {
        case HOST_NOT_FOUND:
            return EAI_NONAME;

        case NO_ADDRESS:
# if (NO_ADDRESS != NO_DATA)
        case NO_DATA:
# endif
            return EAI_NODATA;

        case NO_RECOVERY:
            return EAI_FAIL;

        case TRY_AGAIN:
            return EAI_AGAIN;
    }
    return EAI_SYSTEM;
}

/*
 * This functions must be used to free the memory allocated by getaddrinfo().
 */
void freeaddrinfo (struct addrinfo *res)
{
    if (res == NULL)
        return;
    free (res->ai_canonname);
    free (res->ai_addr);
    free (res->ai_next);
    free (res);
}

/*
 * Internal function that builds an addrinfo struct.
 */
static struct addrinfo *
makeaddrinfo (int af, int type, int proto,
              const struct sockaddr *addr, size_t addrlen,
              const char *canonname)
{
    struct addrinfo *res;

    res = (struct addrinfo *)malloc (sizeof (struct addrinfo));
    if (res != NULL)
    {
        res->ai_flags = 0;
        res->ai_family = af;
        res->ai_socktype = type;
        res->ai_protocol = proto;
        res->ai_addrlen = addrlen;
        res->ai_addr = malloc (addrlen);
        res->ai_canonname = NULL;
        res->ai_next = NULL;

        if (res->ai_addr != NULL)
        {
            memcpy (res->ai_addr, addr, addrlen);

            if (canonname != NULL)
            {
                res->ai_canonname = strdup (canonname);
                if (res->ai_canonname != NULL)
                    return res; /* success ! */
            }
            else
                return res;
        }
    }
    /* failsafe */
    freeaddrinfo (res);
    return NULL;
}

static struct addrinfo *
makeipv4info (int type, int proto, u_long ip, u_short port, const char *name)
{
    struct sockaddr_in addr;

    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
#ifdef HAVE_SA_LEN
    addr.sin_len = sizeof (addr);
#endif
    addr.sin_port = port;
    addr.sin_addr.s_addr = ip;

    return makeaddrinfo (AF_INET, type, proto,
                         (struct sockaddr*)&addr, sizeof (addr), name);
}

/*
 * getaddrinfo() non-thread-safe IPv4-only implementation
 * Address-family-independent hostname to address resolution.
 *
 * This is meant for IPv6-unaware systems that do probably not provide
 * getaddrinfo(), but still have old function gethostbyname().
 *
 * Only UDP and TCP over IPv4 are supported here.
 */
int
getaddrinfo (const char *node, const char *service,
             const struct addrinfo *hints, struct addrinfo **res)
{
    struct addrinfo *info;
    u_long ip;
    u_short port;
    int protocol = 0, flags = 0;
    const char *name = NULL;

    if (hints != NULL)
    {
        flags = hints->ai_flags;

        if (flags & ~_AI_MASK)
            return EAI_BADFLAGS;
        /* only accept AF_INET and AF_UNSPEC */
        if (hints->ai_family && (hints->ai_family != AF_INET))
            return EAI_FAMILY;

        /* protocol sanity check */
        switch (hints->ai_socktype)
        {
            case SOCK_STREAM:
                protocol = IPPROTO_TCP;
                break;

            case SOCK_DGRAM:
                protocol = IPPROTO_UDP;
                break;

#ifdef SOCK_RAW
            case SOCK_RAW:
#endif
            case 0:
                break;

            default:
                return EAI_SOCKTYPE;
        }
        if (hints->ai_protocol && protocol
         && (protocol != hints->ai_protocol))
            return EAI_SERVICE;
    }

    *res = NULL;

    /* default values */
    if (node == NULL)
    {
        if (flags & AI_PASSIVE)
            ip = htonl (INADDR_ANY);
        else
            ip = htonl (INADDR_LOOPBACK);
    }
    else
    if ((ip = inet_addr (node)) == INADDR_NONE)
    {
        struct hostent *entry = NULL;

        /* hostname resolution */
        if (!(flags & AI_NUMERICHOST))
            entry = gethostbyname (node);

        if (entry == NULL)
            return gai_error_from_herrno ();

        if ((entry->h_length != 4) || (entry->h_addrtype != AF_INET))
            return EAI_FAMILY;

        ip = *((u_long *) entry->h_addr);
        if (flags & AI_CANONNAME)
            name = entry->h_name;
    }

    if ((flags & AI_CANONNAME) && (name == NULL))
        name = node;

    /* service resolution */
    if (service == NULL)
        port = 0;
    else
    {
        unsigned long d;
        char *end;

        d = strtoul (service, &end, 0);
        if (end[0] || (d > 65535u))
            return EAI_SERVICE;

        port = htons ((u_short)d);
    }

    /* building results... */
    if ((!protocol) || (protocol == IPPROTO_UDP))
    {
        info = makeipv4info (SOCK_DGRAM, IPPROTO_UDP, ip, port, name);
        if (info == NULL)
        {
            errno = ENOMEM;
            return EAI_SYSTEM;
        }
        if (flags & AI_PASSIVE)
            info->ai_flags |= AI_PASSIVE;
        *res = info;
    }
    if ((!protocol) || (protocol == IPPROTO_TCP))
    {
        info = makeipv4info (SOCK_STREAM, IPPROTO_TCP, ip, port, name);
        if (info == NULL)
        {
            errno = ENOMEM;
            return EAI_SYSTEM;
        }
        info->ai_next = *res;
        if (flags & AI_PASSIVE)
            info->ai_flags |= AI_PASSIVE;
        *res = info;
    }

    return 0;
}
