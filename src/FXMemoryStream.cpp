/********************************************************************************
*                                                                               *
*                   M e m o r y   S t r e a m   C l a s s e s                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
* TnFOX Extensions (C) 2003 Niall Douglas                                       *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXMemoryStream.cpp,v 1.5 2004/02/08 17:29:06 fox Exp $                   *
********************************************************************************/
#include "fxdefs.h"
#include "FXMemoryStream.h"
#include "FXException.h"
#include "QBuffer.h"
#include "qcstring.h"



/*******************************************************************************/

namespace FX {

// Initialize memory stream
FXMemoryStream::FXMemoryStream(const FXObject* cont) : buffer(0), owns(FALSE), FXStream(0, cont)
{
}


// Open a stream, possibly with an initial data array
FXbool FXMemoryStream::open(FXStreamDirection save_or_load, FXuchar* data)
{
	FXERRHM(buffer=new QBuffer);
	setDevice(buffer);
	if(data)
	{
		buffer->buffer().setRawData(data, 4294967295U);
		owns=FALSE;
	}
	else
	{
		buffer->buffer().resize(1);
		owns=TRUE;
	}
	if(save_or_load==FXStreamLoad)
	{   // Open for read
		buffer->open(IO_ReadOnly);
	}
	else
	{   // Open for write
		buffer->open(IO_WriteOnly);
	}
	return FXStream::open(save_or_load);
}


// Open a stream, possibly with initial data array of certain size
FXbool FXMemoryStream::open(FXStreamDirection save_or_load,unsigned long size,FXuchar* data)
{
	FXERRHM(buffer=new QBuffer);
	setDevice(buffer);
	if(data)
	{
		buffer->buffer().setRawData(data, size);
		owns=FALSE;
	}
	else
	{
		if(size==0) size=1;
		buffer->buffer().resize(size);
		owns=TRUE;
	}
	if(save_or_load==FXStreamLoad)
	{   // Open for read
		buffer->open(IO_ReadOnly);
	}
	else
	{   // Open for write
		buffer->open(IO_WriteOnly);
	}
	return FXStream::open(save_or_load);
}


// Take buffer away from stream
void FXMemoryStream::takeBuffer(FXuchar *&dest,unsigned long& size)
{
	if(owns)
	{	// Make a copy
		size=buffer->buffer().size();
		FXMALLOC(&dest, FXuchar, size);
		memcpy(dest, buffer->buffer().data(), size);
	}
	else
	{
		size=buffer->buffer().size();
		dest=(FXuchar *) buffer->buffer().data();
		buffer->buffer().resetRawData(dest, size);
	}
	buffer->close();
	buffer->buffer().resize(0);
	owns=FALSE;
}


// Give buffer to stream
void FXMemoryStream::giveBuffer(FXuchar *src,unsigned long size)
{
	if(src==NULL){ fxerror("FXMemoryStream::giveBuffer: NULL buffer argument.\n"); }
	if(owns) buffer->buffer().resize(0);
	buffer->buffer().setRawData(src, size);
	owns=TRUE;
}


// Close the stream
FXbool FXMemoryStream::close()
{
	if(owns)
		buffer->buffer().resize(0);
	else
		buffer->buffer().resetRawData(buffer->buffer().data(), buffer->buffer().size());
	owns=FALSE;
	FXDELETE(buffer);
	return FXStream::close();
}

}
