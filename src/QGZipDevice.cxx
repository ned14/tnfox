/********************************************************************************
*                                                                               *
*                          Filter device applying .gz                           *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
*       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
* Parts blatently hacked in from zlib Copyright (C) 1995-1998 Jean-loup Gailly  *
*********************************************************************************
* This code is free software; you can redistribute it and/or modify it under    *
* the terms of the GNU Library General Public License v2.1 as published by the  *
* Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
* "upgrade" this code to the GPL without my prior written permission.           *
* Please consult the file "License_Addendum2.txt" accompanying this file.       *
*                                                                               *
* This code is distributed in the hope that it will be useful,                  *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
*********************************************************************************
* $Id:                                                                          *
********************************************************************************/

#include "QGZipDevice.h"
#include "FXException.h"
#include "QBuffer.h"
#include "QThread.h"
#include "QTrans.h"
#include <qcstring.h>
#include "FXErrCodes.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#ifdef HAVE_ZLIB_H
// ******** Here follows the guts of gzio.c *********
#include "zlib.h"

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
/* default memLevel */

#ifdef WIN32 /* Window 95 & Windows NT */
#  define OS_CODE  0x0b
#endif
#ifndef OS_CODE
#  define OS_CODE  0x03  /* assume Unix */
#endif

namespace ZLib {

#define Z_BUFSIZE 16384

#define ALLOC(size) malloc(size)
#define TRYFREE(p) {if (p) free(p);}

static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

typedef struct gz_stream {
    z_stream stream;
    int      z_err;   /* error code for last stream operation */
    int      z_eof;   /* set if end of input file */
	FX::QIODevice *file; /* .gz file */
    Byte     *inbuf;  /* input buffer */
    Byte     *outbuf; /* output buffer */
    uLong    crc;     /* crc32 of uncompressed data */
    char     *msg;    /* error message */
    int      transparent; /* 1 if input file is not a .gz file */
	bool     write;
    long     startpos; /* start of compressed data in file (header skipped) */
} gz_stream;

static int get_byte(gz_stream *s)
{
    if (s->z_eof) return EOF;
    if (s->stream.avail_in == 0) {
	s->stream.avail_in = s->file->readBlock((char *) s->inbuf, Z_BUFSIZE);
	if (s->stream.avail_in == 0) {
	    s->z_eof = 1;
	    return EOF;
	}
	s->stream.next_in = s->inbuf;
    }
    s->stream.avail_in--;
    return *(s->stream.next_in)++;
}

static void check_header(gz_stream *s)
{
    int method; /* method byte */
    int flags;  /* flags byte */
    uInt len;
    int c;

    /* Check the gzip magic header */
    for (len = 0; len < 2; len++) {
	c = get_byte(s);
	if (c != gz_magic[len]) {
	    if (len != 0) s->stream.avail_in++, s->stream.next_in--;
	    if (c != EOF) {
		s->stream.avail_in++, s->stream.next_in--;
		s->transparent = 1;
	    }
	    s->z_err = s->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
	    return;
	}
    }
    method = get_byte(s);
    flags = get_byte(s);
    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
	s->z_err = Z_DATA_ERROR;
	return;
    }

    /* Discard time, xflags and OS code: */
    for (len = 0; len < 6; len++) (void)get_byte(s);

    if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
	len  =  (uInt)get_byte(s);
	len += ((uInt)get_byte(s))<<8;
	/* len is garbage if EOF but the loop below will quit anyway */
	while (len-- != 0 && get_byte(s) != EOF) ;
    }
    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
	while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
	while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
	for (len = 0; len < 2; len++) (void)get_byte(s);
    }
    s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

static int destroy (gz_stream *s)
{
    int err = Z_OK;

    if (!s) return Z_STREAM_ERROR;

    TRYFREE(s->msg);

    if (s->stream.state != NULL) {
	if (s->write) {
	    err = deflateEnd(&(s->stream));
	} else {
	    err = inflateEnd(&(s->stream));
	}
    }
    if (s->file != NULL) {
	    err = Z_ERRNO;
    }
    if (s->z_err < 0) err = s->z_err;

    TRYFREE(s->inbuf);
    TRYFREE(s->outbuf);
    TRYFREE(s);
    return err;
}

static gzFile gz_open (FX::QIODevice *fd, bool write)
{
    int err;
    int level = Z_DEFAULT_COMPRESSION; /* compression level */
    int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */
    gz_stream *s;

    s = (gz_stream *)ALLOC(sizeof(gz_stream));
    if (!s) return Z_NULL;

    s->stream.zalloc = (alloc_func)0;
    s->stream.zfree = (free_func)0;
    s->stream.opaque = (voidpf)0;
    s->stream.next_in = s->inbuf = Z_NULL;
    s->stream.next_out = s->outbuf = Z_NULL;
    s->stream.avail_in = s->stream.avail_out = 0;
    s->file = fd;
    s->z_err = Z_OK;
    s->z_eof = 0;
    s->crc = crc32(0L, Z_NULL, 0);
    s->msg = NULL;
    s->transparent = 0;
	s->write=write;
    
    if (s->write) {
        err = deflateInit2(&(s->stream), level,
                           Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, strategy);
        /* windowBits is passed < 0 to suppress zlib header */

        s->stream.next_out = s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
        if (err != Z_OK || s->outbuf == Z_NULL) {
            return destroy(s), (gzFile)Z_NULL;
        }
    } else {
        s->stream.next_in  = s->inbuf = (Byte*)ALLOC(Z_BUFSIZE);

        err = inflateInit2(&(s->stream), -MAX_WBITS);
        /* windowBits is passed < 0 to tell that there is no zlib header.
         * Note that in this case inflate *requires* an extra "dummy" byte
         * after the compressed stream in order to complete decompression and
         * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
         * present after the compressed stream.
         */
        if (err != Z_OK || s->inbuf == Z_NULL) {
            return destroy(s), (gzFile)Z_NULL;
        }
    }
    s->stream.avail_out = Z_BUFSIZE;

    if (s->write) {
        /* Write a very simple .gz header:
         */
		char buff[32];
        sprintf(buff, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
             Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, OS_CODE);
		s->file->writeBlock(buff, 10);
	s->startpos = 10L;
	/* We use 10L instead of ftell(s->file) to because ftell causes an
         * fflush on some systems. This version of the library doesn't use
         * startpos anyway in write mode, so this initialization is not
         * necessary.
         */
    } else {
	check_header(s); /* skip the .gz header */
	s->startpos = ((long)s->file->at() - s->stream.avail_in);
    }
    
    return (gzFile)s;
}

static uLong getLong (gz_stream *s)
{
    uLong x = (uLong)get_byte(s);
    int c;

    x += ((uLong)get_byte(s))<<8;
    x += ((uLong)get_byte(s))<<16;
    c = get_byte(s);
    if (c == EOF) s->z_err = Z_DATA_ERROR;
    x += ((uLong)c)<<24;
    return x;
}

static int gz_read (gzFile file, void *buf, unsigned len)
{
    gz_stream *s = (gz_stream*)file;
    Bytef *start = (Bytef*)buf; /* starting point for crc computation */
    Byte  *next_out; /* == stream.next_out but not forced far (for MSDOS) */

    if (s == NULL || s->write) return Z_STREAM_ERROR;

    if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return -1;
    if (s->z_err == Z_STREAM_END) return 0;  /* EOF */

    next_out = (Byte*)buf;
    s->stream.next_out = (Bytef*)buf;
    s->stream.avail_out = len;

    while (s->stream.avail_out != 0) {

	if (s->transparent) {
	    /* Copy first the lookahead bytes: */
	    uInt n = s->stream.avail_in;
	    if (n > s->stream.avail_out) n = s->stream.avail_out;
	    if (n > 0) {
		memcpy(s->stream.next_out, s->stream.next_in, n);
		next_out += n;
		s->stream.next_out = next_out;
		s->stream.next_in   += n;
		s->stream.avail_out -= n;
		s->stream.avail_in  -= n;
	    }
	    if (s->stream.avail_out > 0) {
		s->stream.avail_out -= s->file->readBlock((char *) next_out, s->stream.avail_out);
	    }
	    len -= s->stream.avail_out;
	    s->stream.total_in  += (uLong)len;
	    s->stream.total_out += (uLong)len;
            if (len == 0) s->z_eof = 1;
	    return (int)len;
	}
        if (s->stream.avail_in == 0 && !s->z_eof) {

            errno = 0;
            s->stream.avail_in = s->file->readBlock((char *) s->inbuf, Z_BUFSIZE);
            if (s->stream.avail_in == 0) {
                s->z_eof = 1;
            }
            s->stream.next_in = s->inbuf;
        }
        s->z_err = inflate(&(s->stream), Z_NO_FLUSH);

	if (s->z_err == Z_STREAM_END) {
	    /* Check CRC and original size */
	    s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));
	    start = s->stream.next_out;

	    if (getLong(s) != s->crc) {
		s->z_err = Z_DATA_ERROR;
	    } else {
	        (void)getLong(s);
                /* The uncompressed length returned by above getlong() may
                 * be different from s->stream.total_out) in case of
		 * concatenated .gz files. Check for such files:
		 */
		check_header(s);
		if (s->z_err == Z_OK) {
		    uLong total_in = s->stream.total_in;
		    uLong total_out = s->stream.total_out;

		    inflateReset(&(s->stream));
		    s->stream.total_in = total_in;
		    s->stream.total_out = total_out;
		    s->crc = crc32(0L, Z_NULL, 0);
		}
	    }
	}
	if (s->z_err != Z_OK || s->z_eof) break;
    }
    s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));

    return (int)(len - s->stream.avail_out);
}

int gz_write (gzFile file, const void *buf, unsigned len)
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || !s->write) return Z_STREAM_ERROR;

    s->stream.next_in = (Bytef*)buf;
    s->stream.avail_in = len;

    while (s->stream.avail_in != 0) {

        if (s->stream.avail_out == 0) {

            s->stream.next_out = s->outbuf;
            if (s->file->writeBlock((char *) s->outbuf, Z_BUFSIZE) != Z_BUFSIZE) {
                s->z_err = Z_ERRNO;
                break;
            }
            s->stream.avail_out = Z_BUFSIZE;
        }
        s->z_err = deflate(&(s->stream), Z_NO_FLUSH);
        if (s->z_err != Z_OK) break;
    }
    s->crc = crc32(s->crc, (const Bytef *)buf, len);

    return (int)(len - s->stream.avail_in);
}

static int do_flush (gzFile file, int flush)
{
    uInt len;
    int done = 0;
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || !s->write) return Z_STREAM_ERROR;

    s->stream.avail_in = 0; /* should be zero already anyway */

    for (;;) {
        len = Z_BUFSIZE - s->stream.avail_out;

        if (len != 0) {
            if ((uInt)s->file->writeBlock((char *) s->outbuf, len) != len) {
                s->z_err = Z_ERRNO;
                return Z_ERRNO;
            }
            s->stream.next_out = s->outbuf;
            s->stream.avail_out = Z_BUFSIZE;
        }
        if (done) break;
        s->z_err = deflate(&(s->stream), flush);

	/* Ignore the second of two consecutive flushes: */
	if (len == 0 && s->z_err == Z_BUF_ERROR) s->z_err = Z_OK;

        /* deflate has finished flushing only when it hasn't used up
         * all the available space in the output buffer: 
         */
        done = (s->stream.avail_out != 0 || s->z_err == Z_STREAM_END);
 
        if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) break;
    }
    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}

static void putLong (FX::QIODevice *file, uLong x)
{
    int n;
    for (n = 0; n < 4; n++) {
        file->putch((int)(x & 0xff));
        x >>= 8;
    }
}

static int gz_close (gzFile file)
{
    int err;
    gz_stream *s = (gz_stream*)file;

    if (s == NULL) return Z_STREAM_ERROR;

    if (s->write) {
        err = do_flush (file, Z_FINISH);
        if (err != Z_OK) return destroy((gz_stream*)file);

        putLong (s->file, s->crc);
        putLong (s->file, s->stream.total_in);
    }
    return destroy((gz_stream*)file);
}
} // extern "C"
#endif

namespace FX {


struct FXDLLLOCAL QGZipDevicePrivate : public QMutex
{
	QIODevice *src;
	QBuffer uncomp;
#ifdef HAVE_ZLIB_H
	gzFile inh, outh;
#endif
	QGZipDevicePrivate(QIODevice *_src) : src(_src), QMutex() { }
};

QGZipDevice::QGZipDevice(QIODevice *src) : p(0), QIODevice()
{
	FXERRHM(p=new QGZipDevicePrivate(src));
}

QGZipDevice::~QGZipDevice()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		close();
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }

QIODevice *QGZipDevice::GZData() const
{
	return p->src;
}

void QGZipDevice::setGZData(QIODevice *src)
{
	p->src=src;
}

bool QGZipDevice::open(FXuint mode)
{
	FXMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QGZipDevice", "Device reopen has different mode"));
	}
	else
	{
#ifndef HAVE_ZLIB_H
		FXERRG("This TnFOX was built without zlib support", FXGZIPDEVICE_NOZLIB, FXERRH_ISDEBUG);
#else
		//if(mode & IO_Truncate) FXERRG("Cannot truncate with this device", FXGZIPDEVICE_CANTTRUNCATE, FXERRH_ISDEBUG);
		FXERRH(p->src, "Need to set a source device before opening", FXGZIPDEVICE_MISSINGSOURCE, FXERRH_ISDEBUG);
		if(p->src->isClosed()) p->src->open(mode & ~IO_Translate);
		p->uncomp.open(IO_ReadWrite);
		if(mode & IO_ReadOnly)
		{
			if(!p->src->atEnd())
			{
				p->inh=ZLib::gz_open(p->src, false);
				int len=Z_BUFSIZE, read, offset=0;
				do
				{
					p->uncomp.buffer().resize(len);
					FXuchar *data=p->uncomp.buffer().data()+offset;
					read=ZLib::gz_read(p->inh, data, Z_BUFSIZE);
					len+=read; offset+=read;
				} while(read);
				p->uncomp.buffer().resize(len-Z_BUFSIZE);
				ZLib::gz_close(p->inh);
				p->inh=0;
				if(mode & IO_Translate)
				{
					FXuchar *data=p->uncomp.buffer().data();
					FXuval datasize=p->uncomp.buffer().size();
					bool midNL;
					p->uncomp.buffer().resize(removeCRLF(midNL, data, data, datasize));
				}
			}
		}
		p->outh=0;
		setFlags((mode & IO_ModeMask)|IO_Open);
#endif
		ioIndex=0;
	}
	return true;
}

void QGZipDevice::close()
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	if(isOpen())
	{
		flush();
		p->uncomp.truncate(0);
		p->uncomp.close();
		setFlags(0);
	}
#endif
}

void QGZipDevice::flush()
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	if(isOpen() && isWriteable())
	{
		FXuchar *data=p->uncomp.buffer().data();
		FXuval datalen=p->uncomp.buffer().size();
		p->src->at(0);
		p->outh=ZLib::gz_open(p->src, true);
		if(isTranslated())
		{
			FXuchar buffer[4096];
			FXuval out=0, in=0, idx;
			for(idx=0; idx<datalen; idx+=in)
			{
				in=sizeof(buffer);
				bool midNL;
				out=applyCRLF(midNL, buffer, data+idx, sizeof(buffer), in);
				ZLib::gz_write(p->outh, buffer, out);
			}
		}
		else ZLib::gz_write(p->outh, data, datalen);
		ZLib::gz_close(p->outh);
		p->outh=0;
		p->src->truncate(p->src->at());
		//p->src->flush();
	}
#endif
}

FXfval QGZipDevice::size() const
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.size();
#else
	return 0;
#endif
}

void QGZipDevice::truncate(FXfval size)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	if((mode() & IO_ShredTruncate) && size<p->uncomp.size())
		shredData(size);
	p->uncomp.truncate(size);
#endif
}

FXfval QGZipDevice::at() const
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.at();
#else
	return 0;
#endif
}

bool QGZipDevice::at(FXfval newpos)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.at(newpos);
#else
	return false;
#endif
}

bool QGZipDevice::atEnd() const
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.atEnd();
#else
	return true;
#endif
}

FXuval QGZipDevice::readBlock(char *data, FXuval maxlen)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.readBlock(data, maxlen);
#else
	return 0;
#endif
}

FXuval QGZipDevice::writeBlock(const char *data, FXuval maxlen)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.writeBlock(data, maxlen);
#else
	return 0;
#endif
}

FXuval QGZipDevice::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	p->uncomp.at(pos);
	return p->uncomp.readBlock(data, maxlen);
#else
	return 0;
#endif
}
FXuval QGZipDevice::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	p->uncomp.at(pos);
	return p->uncomp.writeBlock(data, maxlen);
#else
	return 0;
#endif
}

int QGZipDevice::getch()
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.getch();
#else
	return -1;
#endif
}

int QGZipDevice::putch(int c)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.putch(c);
#else
	return -1;
#endif
}

int QGZipDevice::ungetch(int c)
{
#ifdef HAVE_ZLIB_H
	FXMtxHold h(p);
	return p->uncomp.ungetch(c);
#else
	return -1;
#endif
}

} // namespace
