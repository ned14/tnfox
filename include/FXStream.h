/********************************************************************************
*                                                                               *
*       P e r s i s t e n t   S t o r a g e   S t r e a m   C l a s s e s       *
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
* $Id: FXStream.h,v 1.37 2005/01/16 16:06:06 fox Exp $                          *
********************************************************************************/
#ifndef FXSTREAM_H
#define FXSTREAM_H

#include "fxdefs.h"
#include "fxver.h"

namespace FX {

/*! \file FXStream.h
\brief Defines classes used in formatting data for i/o
*/


/// Stream data flow direction
enum FXStreamDirection {
  FXStreamDead=0,               /// Unopened stream
  FXStreamSave=1,               /// Saving stuff to stream
  FXStreamLoad=2                /// Loading stuff from stream
  };


/// Stream status codes
enum FXStreamStatus {
  FXStreamOK=0,                 /// OK
  FXStreamEnd=1,                /// Try read past end of stream
  FXStreamFull=2,               /// Filled up stream buffer or disk full
  FXStreamNoWrite=3,            /// Unable to open for write
  FXStreamNoRead=4,             /// Unable to open for read
  FXStreamFormat=5,             /// Stream format error
  FXStreamUnknown=6,            /// Trying to read unknown class
  FXStreamAlloc=7,              /// Alloc failed
  FXStreamFailure=8             /// General failure
  };


/// Stream seeking
enum FXWhence {
  FXFromStart=0,                /// Seek from start position
  FXFromCurrent=1,              /// Seek from current position
  FXFromEnd=2                   /// Seek from end position
  };


class FXHash;
class FXString;
class FXIODevice;

/************************  Persistent Store Definition  *************************/

/*! \class FXStream
\brief File format helper (Qt compatible)

FXStream is a modified version of FOX's FXStream - it is mostly API compatible
but admittedly reflects the biggest divergence of API between FOX and TnFOX at
the time of writing.
Both share the purpose of serialising and deserialising data to and from a
stored format and indeed via loadObject() and saveObject() can transfer entire
window trees to and from storage.

Most use is simple - set which FXIODevice it should use, and then use the << and
>> operators to store and load respectively. position() lets you set or read
the current offset into the file. Most classes in TnFOX (as well as FOX) provide
overloads for these operators, so your << and >> overloads merely need to call
<< and >> on each of your data members you wish to save. Use load() and save()
for blocks of individual units for efficiency.

Previous to FOX v1.1.31, FOX used FXStream
mostly for dumping application state and its FXStream was not designed for general
purpose file i/o. TnFOX uses FXStream for IPC which can traverse networks and
CPU types, so it needs FXStream to be fast and always generate data in a
universally understood format. You can of course use swapBytes() based on
a flag within your data structure to avoid unnecessary endian conversions
(and indeed this is what TnFOX's IPC classes do based on endian types on each
end of the communications channel).

Since FOX v1.1.31, FXStream incorporates buffering and more general purpose
facilities. The buffering in particular is left out because that's for the
i/o device classes to implement if they want to. More importantly, IPC just
would not work if a write to a pipe was coalesced into a larger packet sent
sometime later.

<h3>Difference from FOX's FXStream & Qt's QDataStream</h3>
Endian translation is performed for you on big endian machines by default
(simply because numerically more computers in the world are little endian,
therefore we want to cost the least performance for the most people).
Normal FOX always saves in native format and converts on load - and Qt is
big endian by default.

Lastly, QDataStream's printable data format is unsupported. You don't need
it with a decent debugger anyway (and it's ridiculously slow).

<h3>Useful tip:</h3>
Look into FXBuffer's ability to dump itself onto a stream. This lets you
very conveniently prepare parts of an overall image into separate sections.
I've personally found that if you keep your end file image in a set of
ordered nested QPtrList's & FXBuffer's, writing it out is as simple as:
\code
FXStream s;
QValueList<QPtrListOrBufferHolder> filedata;
s << filedata;
\endcode
QValueList invokes the << operator on each of its members and so it recurses
down calling each in turn. All QTL thunks provided in TnFOX provide default
stream operators which require a null constructor in the type to compile.
*/
class FXAPI FXStream {
protected:
  FXHash            *hash;      // Hash table
  const FXObject    *parent;    // Parent object
  FXuchar           *begptr;    // Begin of buffer
  FXuchar           *endptr;    // End of buffer
  FXuchar           *wrptr;     // Write pointer
  FXuchar           *rdptr;     // Read pointer
  FXlong             pos;       // Position
  FXStreamDirection  dir;       // Direction of current transfer
  FXStreamStatus     code;      // Status code
  FXuint             seq;       // Sequence number
  FXbool             owns;      // Stream owns buffer
  FXbool             swap;      // Swap bytes on readin

protected: // TnFOX stuff
  FXIODevice        *dev;       // i/o device
public:

  //! Constructs an instance using device \em dev. \em cont is for FOX FXStream emulation only.
  FXStream(FXIODevice *dev=0, const FXObject* cont=NULL);

  //! Returns the i/o device this stream is using
  FXIODevice *device() const { return dev; }
  //! Sets the i/o device this stream is using
  void setDevice(FXIODevice *dev);
  //! \deprecated For Qt compatibility only
  void unsetDevice() { setDevice(0); }
  //! Returns true if there is no more data to be read
  bool atEnd() const;
  enum ByteOrder
  {
	  BigEndian=0,
	  LittleEndian
  };
  //! Returns the byte order the data on the i/o device shall be interpreted in
  int byteOrder() const { return ((swap!=0)==!FOX_BIGENDIAN) ? BigEndian : LittleEndian; }
  //! Sets the byte order the data on the i/o device shall be interpreted in
  void setByteOrder(int b) { swap=(b==BigEndian) ? !FOX_BIGENDIAN : FOX_BIGENDIAN; }

  //! \deprecated For Qt compatibility only
  FXStream &readBytes(char *&s, FXuint &l);
  //! Reads preformatted byte data into the specified buffer
  FXStream &readRawBytes(char *buffer, FXuval len);
  //! \overload
  FXStream &readRawBytes(FXuchar *buffer, FXuval len) { return readRawBytes((char *) buffer, len); }
  //! \deprecated For Qt compatibility only
  FXStream &writeBytes(const char *s, FXuint l);
  //! Writes preformatted byte data from the specified buffer
  FXStream &writeRawBytes(const char *buffer, FXuval len);
  //! \overload
  FXStream &writeRawBytes(const FXuchar *buffer, FXuval len) { return writeRawBytes((char *) buffer, len); }

  //! Moves the file pointer backwards by the specified amount of bytes, returning the new file pointer
  FXfval rewind(FXint amount);

public:

  /** \deprecated For FOX compatibility only

  * Construct stream with given container object.  The container object
  * is an object that will itself not be saved to or loaded from the stream,
  * but which may be referenced by other objects.  These references will be
  * properly saved and restored.
  */
  FXStream(const FXObject* cont);

  /** \deprecated For FOX compatibility only

  * Open stream for reading (FXStreamLoad) or for writing (FXStreamSave).
  * An initial buffer size may be given, which must be at least 16 bytes.
  * If data is not NULL, it is expected to point to an external data buffer
  * of length size; otherwise stream will use an internally managed buffer.
  */
  FXbool open(FXStreamDirection save_or_load,unsigned long size=8192,FXuchar* data=NULL);

  /** \deprecated For FOX compatibility only

  Close; return TRUE if OK
  */
  virtual FXbool close();

  /** \deprecated For FOX compatibility only
  
  Flush buffer
  */
  virtual FXbool flush();

  /** \deprecated For FOX compatibility only

  Get available buffer space
  */
  unsigned long getSpace() const;

  /** \deprecated For FOX compatibility only

  Set available buffer space
  */
  void setSpace(unsigned long sp);

  /** \deprecated For FOX compatibility only
  
  Get status code
  */
  FXStreamStatus status() const { return code; }

  /// Return TRUE if at end of file or error
  FXbool eof() const { return atEnd(); }

  /** \deprecated For FOX compatibility only
  
  Set status code
  */
  void setError(FXStreamStatus err);

  /** \deprecated For FOX compatibility only
  
  Obtain stream direction
  */
  FXStreamDirection direction() const { return dir; }

  /** \deprecated For FOX compatibility only
  
  Get parent object
  */
  const FXObject* container() const { return parent; }

  /// Get position
  FXfval position() const;

  /// Move to position relative to head, tail, or current location
  virtual FXbool position(FXfval offset,FXWhence whence=FXFromStart);

  /// Change swap bytes flag. -1 sets machine default (ie; do swap on big endian machines)
  void swapBytes(FXint s){ swap=(s<0) ? !FOX_BIGENDIAN : (s!=0); }

  /// Get swap bytes flag
  FXbool swapBytes() const { return swap; }

  /**
  * Set stream to big endian mode if TRUE.  Byte swapping will
  * be enabled if the machine native byte order is not equal to
  * the desired byte order.
  */
  void setBigEndian(FXbool big){ swap=(big^FOX_BIGENDIAN); }

  /**
  * Return TRUE if big endian mode.
  */
  FXbool isBigEndian() const { return (swap^FOX_BIGENDIAN); }

  /// Save single items to stream
  FXStream& operator<<(const FXuchar& v);
  FXStream& operator<<(const FXchar& v){ return *this << reinterpret_cast<const FXuchar&>(v); }
  FXStream& operator<<(const FXushort& v);
  FXStream& operator<<(const FXshort& v){ return *this << reinterpret_cast<const FXushort&>(v); }
  FXStream& operator<<(const FXuint& v);
  FXStream& operator<<(const FXint& v){ return *this << reinterpret_cast<const FXuint&>(v); }
  FXStream& operator<<(const FXfloat& v);
  FXStream& operator<<(const FXdouble& v);
  FXStream& operator<<(const FXlong& v){ return *this << reinterpret_cast<const FXulong&>(v); }
  FXStream& operator<<(const FXulong& v);
  FXStream& operator<<(const char *v);
  FXStream& operator<<(const bool& v);

  /// Save arrays of items to stream
  FXStream& save(const FXuchar* p,unsigned long n);
  FXStream& save(const FXchar* p,unsigned long n){ return save(reinterpret_cast<const FXuchar*>(p),n); }
  FXStream& save(const FXushort* p,unsigned long n);
  FXStream& save(const FXshort* p,unsigned long n){ return save(reinterpret_cast<const FXushort*>(p),n); }
  FXStream& save(const FXuint* p,unsigned long n);
  FXStream& save(const FXint* p,unsigned long n){ return save(reinterpret_cast<const FXuint*>(p),n); }
  FXStream& save(const FXfloat* p,unsigned long n);
  FXStream& save(const FXdouble* p,unsigned long n);
  FXStream& save(const FXlong* p,unsigned long n){ return save(reinterpret_cast<const FXulong*>(p),n); }
  FXStream& save(const FXulong* p,unsigned long n);

  /// Load single items from stream
  FXStream& operator>>(FXuchar& v);
  FXStream& operator>>(FXchar& v){ return *this >> reinterpret_cast<FXuchar&>(v); }
  FXStream& operator>>(FXushort& v);
  FXStream& operator>>(FXshort& v){ return *this >> reinterpret_cast<FXushort&>(v); }
  FXStream& operator>>(FXuint& v);
  FXStream& operator>>(FXint& v){ return *this >> reinterpret_cast<FXuint&>(v); }
  FXStream& operator>>(FXfloat& v);
  FXStream& operator>>(FXdouble& v);
  FXStream& operator>>(FXlong& v){ return *this >> reinterpret_cast<FXulong&>(v); }
  FXStream& operator>>(FXulong& v);
  FXStream& operator>>(bool& v);

  /// Load arrays of items from stream
  FXStream& load(FXuchar* p,unsigned long n);
  FXStream& load(FXchar* p,unsigned long n){ return load(reinterpret_cast<FXuchar*>(p),n); }
  FXStream& load(FXushort* p,unsigned long n);
  FXStream& load(FXshort* p,unsigned long n){ return load(reinterpret_cast<FXushort*>(p),n); }
  FXStream& load(FXuint* p,unsigned long n);
  FXStream& load(FXint* p,unsigned long n){ return load(reinterpret_cast<FXuint*>(p),n); }
  FXStream& load(FXfloat* p,unsigned long n);
  FXStream& load(FXdouble* p,unsigned long n);
  FXStream& load(FXlong* p,unsigned long n){ return load(reinterpret_cast<FXulong*>(p),n); }
  FXStream& load(FXulong* p,unsigned long n);

  /// Save object
  FXStream& saveObject(const FXObject* v);

  /// Load object
  FXStream& loadObject(FXObject*& v);

  /// Add object without saving or loading
  FXStream& addObject(const FXObject* v);

  /// Destructor
  virtual ~FXStream();
  };

}

#endif
