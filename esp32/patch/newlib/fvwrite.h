/*
 * Copyright (c) 1990, 2007 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * and/or other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* %W% (Berkeley) %G% */
#include <_ansi.h>

/*
 * I/O descriptors for __sfvwrite_r().
 */
struct __siov {
	const void *iov_base;
	size_t	iov_len;
};
struct __suio {
	struct	__siov *uio_iov;
	int	uio_iovcnt;
	size_t	uio_resid;
};


extern int __sfvwrite_r (struct _reent *, FILE *, struct __suio *);
extern int __swsetup_r (struct _reent *, FILE *);