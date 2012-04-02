/*	$Id: error.c,v 1.8 2008/05/10 07:53:41 ragge Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>

#include "defines.h"
#include "defs.h"

void
warn(char *s, ...)
{
	va_list ap;

	if(nowarnflag)
		return;

	va_start(ap, s);
	fprintf(diagfile, "Warning on line %d of %s: ", lineno, infname);
	vfprintf(diagfile, s, ap);
	fprintf(diagfile, "\n");
	va_end(ap);
	++nwarn;
}

void
err(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	fprintf(diagfile, "Error on line %d of %s: ", lineno, infname);
	vfprintf(diagfile, s, ap);
	fprintf(diagfile, "\n");
	va_end(ap);
	++nerr;
}

void
yyerror(s)
char *s;
{ err(s); }


void
dclerr(s, v)
	char *s;
	struct bigblock *v;
{
	char buff[100];

	if(v) {
		sprintf(buff, "Declaration error for %s: %s",
		    varstr(VL, v->b_name.varname), s);
		err( buff);
	} else
		err1("Declaration error %s", s);
}


void
execerr(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	fprintf(diagfile, "Error on line %d of %s: Execution error ",
	    lineno, infname);
	vfprintf(diagfile, s, ap);
	fprintf(diagfile, "\n");
	va_end(ap);
	++nerr;
}

void
fatal(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	fprintf(diagfile, "Compiler error line %d of %s: ", lineno, infname);
	vfprintf(diagfile, s, ap);
	fprintf(diagfile, "\n");
	va_end(ap);

	if(debugflag)
		abort();
	done(3);
	exit(3);
}
