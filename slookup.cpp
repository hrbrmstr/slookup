
/*
 *	slookup.cpp - dns lookup stream utility
 *
 *	Heikki Hannikainen <hessu@hes.iki.fi> 1998, 2004
 *  Bob Rudis <bob@rud.is> (2016)
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	
 */


#define VERSION "1.3"
#define VERSTR  "slookup version " VERSION " by Heikki Hannikainen <hessu@hes.iki.fi> 1998, 2004 and Bob Rudis <bob@rud.is> (2016)\n\tThis is free software, redistributable under the GNU GPL v2.\n"
#define HELPS	"Usage: slookup [-f <children>] [-i file] [-p] [-t A|PTR|MX]|NS|TXT|CNAME|SOA]\n\t-f\tNumber of children to Fork for Fast parallel lookups\n\t-i\tFilename to read from instead of stdin\n\t-p\tUse persistent TCP connection(s) to DNS server\n\t-t\tSpecify query type\n"
#define MAXLEN 8192
#define MAXCHILDREN 128

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <ctype.h>
#include <db.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>

#ifdef __APPLE__

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long s64;
typedef unsigned long u64;

#include "nameser_compat.h"

#endif

#include <string>
using namespace std;

// QUERY TYPES
#define Q_A	1
#define Q_PTR	2
#define Q_MX	3
#define Q_NS	4
#define Q_TXT 5
#define Q_CNAME 6
#define Q_SOA 7

int verbose = 0;
int children = 0;
int qtype = Q_A;
int persistent = 0;

int infd[MAXCHILDREN];
pid_t pid[MAXCHILDREN];

FILE *input;

// upcase
char *strupr(char *s) {
	for (char *p = s; (*p); p++) *p = toupper(*p);
	return s;
}

// Convert return values of gethostbyname() to a string
char *h_strerror(int i) {

	static char host_not_found[] = "Host not found";
	static char no_address[] = "No IP address found for name";
	static char no_recovery[] = "A non-recovable name server error occurred";
	static char try_again[] = "A temporary error on an authoritative name server";
	static char unknown_error[] = "Unknown error result code from resolver";
	
	switch (i) {
		case HOST_NOT_FOUND:
			return host_not_found;
		case NO_ADDRESS:
			return no_address;
		case NO_RECOVERY:
			return no_recovery;
		case TRY_AGAIN:
			return try_again;
		case -1:
			return strerror(errno);
		default:
			return unknown_error;
	}

}

// Parse arguments
void parse_args(int argc, char *argv[]) {

	int s;

	input = stdin;
	
	while ((s = getopt(argc, argv, "f:t:i:pv?h")) != -1) {
		switch(s) {
			case 'f':
				children = atoi(optarg);
				break;
			case 'i':
			  input = fopen(optarg, "r");
			  if (input == NULL) {
			  	fprintf(stderr, "Error opening file: %s\n", optarg);
			  	exit(1);
			  }
			  break;
			case 't':
				strupr(optarg);
				if (!strcmp(optarg, "A")) {
					qtype = Q_A;
				} else if (!strcmp(optarg, "PTR")) {
					qtype = Q_PTR;
				} else if (!strcmp(optarg, "MX")) {
					qtype = Q_MX;
				} else if (!strcmp(optarg, "NS")) {
					qtype = Q_NS;
				} else if (!strcmp(optarg, "TXT")) {
					qtype = Q_TXT;
				} else if (!strcmp(optarg, "CNAME")) {
					qtype = Q_CNAME;
				} else if (!strcmp(optarg, "SOA")) {
					qtype = Q_SOA;
				} else {
					fprintf(stderr, "Unknown query type \"%s\", only can do A, PTR, NS, MX, TXT or CNAME\n", optarg);
					exit(1);
				}
				break;
			case 'v':
				verbose = 1;
				break;
			case 'p':
				persistent = 1;
				break;
			case '?':
			case 'h':
			default :
				fprintf(stderr, "%s%s", VERSTR, HELPS);
				exit(1);
		}
	}
	
	if (qtype == Q_MX || qtype == Q_NS || qtype == Q_TXT)
		res_init();

}

// Close pipes
void close_all(void) {

	int i;
	if (children) {
		signal(SIGCHLD, SIG_IGN);
		for (i = 0; i < children; i++)
			close(infd[i]);
		while (wait(NULL) > 0);
	}

	int ret = fclose(input);
	
	exit((ret==0) ? 0 : 1);

}

// Terminate child processes
void hunt(void){
	for (int i = 0; i < children; i++) kill(pid[i], 15);
}

// Finish
void finish(void) {
	if (children) hunt();
	exit(0);
}

// Fork children
void rabbit(void) {

	int i;
	int fd[2];
	
	for (i = 0; i < children; i++) {
		if (pipe(fd)) {
			fprintf(stderr, "rabbit: pipe failed: %s\n", strerror(errno));
			exit(1);
		}
		if ((pid[i] = fork()) == -1) {
			fprintf(stderr, "rabbit: fork failed: %s\n", strerror(errno));
			exit(1);
		}
		if (pid[i] == 0) {
			/* child */
			close(0);
			int ret = dup(fd[0]);
			if (ret == -1) fprintf(stderr, "error in file descriptor: %s\n", strerror(errno));
			close(fd[0]);
			close(fd[1]);
			children = 0;
			return;
		} else {
			infd[i] = fd[1];
			close(fd[0]);
		}
	}
	
	signal(SIGTERM, (void (*)(int))&finish);
	signal(SIGHUP,  (void (*)(int))&finish);
	signal(SIGCHLD, (void (*)(int))&finish);
	signal(SIGPIPE, (void (*)(int))&finish);
	signal(SIGSEGV, (void (*)(int))&finish);

}

// skip a name in the response
int skipname(u_char *start, u_char *p, u_char *eom) {

	u_char buf[MAXDNAME];
	int n;
	
	if ((n = dn_expand(start, eom, p, (char *)buf, MAXDNAME)) < 0) {
		fprintf(stderr, "skipname: dn_expand failed\n");
		exit(2);
	}
	
	return n;

}

// skip to start of rr data portion
int skiptodata(u_char *start, u_char *cp, u_short *type, u_short *dclass,
	             uint32_t *ttl, u_short *dlen, u_char *eom) {

	u_char *tmp_cp = cp;
	
	tmp_cp += skipname(start, tmp_cp, eom);
	GETSHORT(*type, tmp_cp);
	GETSHORT(*dclass, tmp_cp);
	GETLONG(*ttl, tmp_cp);
	GETSHORT(*dlen, tmp_cp);
	
	return (tmp_cp - cp);

}

// Issue A queries
// 
// @param qtype type of query Q_MX, Q_NS
// @param s,qs original query string (host or IP)
// @param rs string to output
// @param rslen max storage space for rs
// @return result of query (char *) 
char *lookup_a(u_short qtype, char *s, char *qs, char *rs, int rslen) {

	struct hostent *he;
	struct in_addr *sinp;
	char **p;

	if (!(he = gethostbyname(s))) {
		snprintf(rs, MAXLEN, "%s - %s\n", qs, h_strerror(h_errno));
		return(rs);
	}
	
	if (he->h_addrtype != AF_INET) {
		snprintf(rs, MAXLEN, "%s - Returned unknown address type %d\n", qs, he->h_addrtype);
		return(rs);
	}
	
	snprintf(rs, MAXLEN, "%s +", qs);
	for (p = he->h_addr_list; *p; p++) {
		strncat(rs, " A ", MAXLEN);
		sinp = (struct in_addr *)*p;
		strncat(rs, inet_ntoa(*sinp), MAXLEN);
	}
	strncat(rs, "\n", MAXLEN);
	return(rs);

}

// Issue PTR queries
// 
// @param qtype type of query Q_MX, Q_NS
// @param s,qs original query string (host or IP)
// @param rs string to output
// @param rslen max storage space for rs
// @return result of query (char *) 
char *lookup_ptr(u_short qtype, char *s, char *qs, char *rs, int rslen) {

	struct hostent *he;
	struct in_addr sin;

	if (!inet_aton(s, &sin)) {
		snprintf(rs, MAXLEN, "%s - Invalid address format\n", qs);
		return(rs);
	}
	
	if (!(he = gethostbyaddr((char *)&sin, sizeof(struct in_addr), AF_INET))) {
		snprintf(rs, MAXLEN, "%s - %s\n", qs, h_strerror(h_errno));
		return(rs);
	}
	
	snprintf(rs, MAXLEN, "%s + PTR %s\n", qs, he->h_name);
	return(rs);

}

char *lookup_soa(u_short qtype, char *s, char *qs, char *rs, int rslen) {

	union {
		HEADER hdr;
		u_char buf[PACKETSZ];
	} res;
	u_char buf[MAXLEN];
	u_char buf1[MAXLEN];
	int reslen;
	u_char *p, *eom;
	int nmx, n;
	int maxmx = 30;
	u_short type, dclass, dlen;
	uint32_t ttl;
	
	if ((reslen = res_query(s, C_IN, T_SOA, (unsigned char *)&res, sizeof(res))) < 0) {
		snprintf(rs, rslen, "%s - %s\n", qs, h_strerror(h_errno));
		return(rs);
	}
	
	eom = res.buf + reslen;
	p = res.buf + sizeof(HEADER);
	
	int ancount = ntohs(res.hdr.ancount);
	// int qdcount = ntohs(res.hdr.qdcount);
	
	if (res.hdr.rcode != NOERROR || ancount == 0) {
		snprintf(rs, rslen, "%s - trouble: %s\n", qs, h_strerror(h_errno));
		return rs;
	}
	
	snprintf(rs, rslen, "%s +", qs);
	
	p += skipname(res.buf, p, eom) + QFIXEDSZ;
	
	nmx = 0;
	while ((--ancount >= 0) && (p < eom) && (nmx < maxmx-1)) {
		p += skiptodata(res.buf, p, &type, &dclass, &ttl, &dlen, eom);
		
		if (type != qtype) {
			p += dlen;
			continue;
		}
			
		if ((n = dn_expand(res.buf, eom, p, (char *)buf, MAXDNAME)) < 0) {
			snprintf(rs, rslen, "%s - dn_expand failed\n", qs);
			return rs;
		}
		p += n;

		if ((n = dn_expand(res.buf, eom, p, (char *)buf1, MAXDNAME)) < 0) {
			snprintf(rs, rslen, "%s - dn_expand failed\n", qs);
			return rs;
		}
		p += n;

    u_long serial, refresh, retry, expire, minimum;

    GETLONG(serial, p);
    GETLONG(refresh, p);
    GETLONG(retry, p);
    GETLONG(expire, p);
    GETLONG(minimum, p);

  	snprintf(rs + strlen(rs), rslen - strlen(rs), " SOA %s %s %ld %ld %ld %ld %ld", 
  		buf, buf1, serial, refresh, retry, expire, minimum);
			
		nmx++;

	}
	
	strncat(rs, "\n", rslen);
	return rs;

}

// Issue TXT queries
// 
// @param qtype type of query Q_MX, Q_NS
// @param s,qs original query string (host or IP)
// @param rs string to output
// @param rslen max storage space for rs
// @return result of query (char *) 
char *lookup_txt(u_short qtype, char *s, char *qs, char *rs, int rslen) {

	unsigned char query_buffer[MAXLEN];
	int reslen;
	ns_msg handle;
	ns_rr rr;

	if ((reslen = res_query(s, C_IN, T_TXT, (unsigned char *)query_buffer, MAXLEN)) < 0) {
  	snprintf(rs, reslen, "%s - dn_expand failed\n", qs);
		return(rs);
  }

  string answer;

  reslen = ns_initparse(query_buffer, reslen, &handle);
  int i;
  for (i=0; i < ns_msg_count(handle, ns_s_an); i++) {
    reslen = ns_parserr(&handle, ns_s_an, i, &rr);
    u_char const *rdata = (u_char*)(ns_rr_rdata(rr));
    string txt(rdata+1, rdata+1+(int)rdata[0]);
    answer += txt;
  }

  snprintf(rs, MAXLEN, "%s + TXT %s\n", qs, answer.c_str());
  return(rs);

}

// Issue MX, NS or CNAME queries
//
// Lumped together as their response formats are similar
// 
// @param qtype type of query Q_MX, Q_NS
// @param s,qs original query string (host or IP)
// @param rs string to output
// @param rslen max storage space for rs
// @return result of query (char *) 
char *lookup_mxnscn(u_short qtype, char *s, char *qs, char *rs, int rslen) {

	union {
		HEADER hdr;
		u_char buf[PACKETSZ];
	} res;
	u_char buf[MAXLEN];
	int reslen;
	u_char *p, *eom;
	int nmx, n;
	int maxmx = 30;
	u_short type, dclass, dlen, pref;
	uint32_t ttl;
	
	if ((reslen = res_query(s, C_IN, qtype, (unsigned char *)&res, sizeof(res))) < 0) {
		snprintf(rs, rslen, "%s - %s\n", qs, h_strerror(h_errno));
		return(rs);
	}
	
	eom = res.buf + reslen;
	p = res.buf + sizeof(HEADER);
	
	int ancount = ntohs(res.hdr.ancount);
	// int qdcount = ntohs(res.hdr.qdcount);
	
	if (res.hdr.rcode != NOERROR || ancount == 0) {
		snprintf(rs, rslen, "%s - trouble: %s\n", qs, h_strerror(h_errno));
		return rs;
	}
	
	snprintf(rs, rslen, "%s +", qs);
	
	p += skipname(res.buf, p, eom) + QFIXEDSZ;
	
	nmx = 0;
	while ((--ancount >= 0) && (p < eom) && (nmx < maxmx-1)) {
		p += skiptodata(res.buf, p, &type, &dclass, &ttl, &dlen, eom);
		
		if (type != qtype) {
			p += dlen;
			continue;
		}
		
		if (type == T_MX) GETSHORT(pref, p);
			
		if ((n = dn_expand(res.buf, eom, p, (char *)buf, MAXDNAME)) < 0) {
			snprintf(rs, rslen, "%s - dn_expand failed\n", qs);
			return rs;
		}
		p += n;
		
		if (type == T_MX)
			snprintf(rs + strlen(rs), rslen - strlen(rs), " MX %d %s", pref, buf);
		if (type == T_CNAME)
			snprintf(rs + strlen(rs), rslen - strlen(rs), " CNAME %s", buf);
		else
			snprintf(rs + strlen(rs), rslen - strlen(rs), " NS %s", buf);
			
		nmx++;
	}
	
	strncat(rs, "\n", rslen);
	return rs;

}

// Query router
//
// Depending on the qtype of query, have one of the lookup_ functions
// issue the request.
// 
// @param qtype type of query Q_A, Q_PTR, Q_MX, Q_NS, Q_TXT
// @param qs query string 
// @return result of query (char *)
char *lookup(int qtype, char *qs) {

	static char s[MAXLEN];
	static char rs[MAXLEN];
	char *sp;
	
	rs[0] = '\0';
	
	strncpy(s, qs, MAXLEN);
	for (sp = s; (*sp); sp++)
		if (isspace(*sp))
			*sp = '\0';
	
	switch (qtype) {
	case Q_A:
		lookup_a(T_A, s, qs, rs, MAXLEN);
		return(rs);
		
	case Q_PTR:
		lookup_ptr(T_PTR, s, qs, rs, MAXLEN);
		return(rs);
		
	case Q_MX:
		lookup_mxnscn(T_MX, s, qs, rs, MAXLEN);
		return(rs);
		
	case Q_NS:
		lookup_mxnscn(T_NS, s, qs, rs, MAXLEN);
		return(rs);

	case Q_TXT:
	  lookup_txt(T_TXT, s, qs, rs, MAXLEN);
	  return(rs);

	case Q_CNAME:
	  lookup_mxnscn(T_CNAME, s, qs, rs, MAXLEN);
	  return(rs);

	case Q_SOA:
	  lookup_soa(T_SOA, s, qs, rs, MAXLEN);
	  return(rs);
		
	default:
		return(rs);
	}

}

// Main
int main(int argc, char **argv) {

	char s[MAXLEN];
	char *p;
	int robin = -1;
	
	parse_args(argc, argv);
	
	/* if we want to fork, then fork */
	if (children) rabbit();
	if (!children && persistent) sethostent(1);
	
	while (fgets(s, MAXLEN-1, input)) {

		if (children) {

			/* if we have children, write to them in a round-robin fashion */
			robin++;
			if (robin == children) robin = 0;
			ssize_t ret = write(infd[robin], s, strlen(s));
			if (ret == -1) fprintf(stderr, "error writing to file: %s\n", strerror(errno));

		} else {

			/* if not (or if i'm a child) resolve here */
			while ((p = strchr(s, '\n'))) *p = '\0';
			fputs(lookup(qtype, s), stdout);
			fflush(stdout);

		}

	}
	
	close_all(); /* End of file... */
	
	return(0); /* Never will... */

}

