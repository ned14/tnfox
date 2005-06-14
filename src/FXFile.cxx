/********************************************************************************
*                                                                               *
*      F i l e   I n f o r m a t i o n   a n d   M a n i p u l a t i o n        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* Contributed by: Sean Hubbell & Niall Douglas                                  *
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
* $Id: FXFile.cpp,v 1.182 2004/11/30 03:16:56 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "qcstring.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXFile.h"
#ifdef WIN32
#include <shellapi.h>
#else
#include <utime.h>
#endif
#include "FXException.h"
#include "FXThread.h"
#include "FXTrans.h"
#include "FXPtrHold.h"
#include "FXRollback.h"
#include "FXACL.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#if 0
#ifdef WIN32
#define HAVE_WIDEUNISTD
#define wopen _wopen
#endif
#endif

/*
  Notes:
  - Thanks to Sean Hubbell for the original impetus for these functions.
  - Windows flavors of some of these functions are not perfect yet.
  - Windows 95 and NT:
      -  1 to 255 character name.
      -  Complete path for a file or project name cannot exceed 259
         characters, including the separators.
      -  May not begin or end with a space.
      -  May not begin with a $
      -  May contain 1 or more file extensions (eg. MyFile.Ext1.Ext2.Ext3.Txt).
      -  Legal characters in the range of 32 - 255 but not ?"/\<>*|:
      -  Filenames may be mixed case.
      -  Filename comparisons are case insensitive (eg. ThIs.TXT = this.txt).
  - MS-DOS and Windows 3.1:
      -  1 to 11 characters in the 8.3 naming convention.
      -  Legal characters are A-Z, 0-9, Double Byte Character Set (DBCS)
         characters (128 - 255), and _^$~!#%&-{}@'()
      -  May not contain spaces, 0 - 31, and "/\[]:;|=,
      -  Must not begin with $
      -  Uppercase only filename.
  - Perhaps use GetEnvironmentVariable instead of getenv?
  - FXFile::search() what if some paths are quoted, like

      \this\dir;"\that\dir with a ;";\some\other\dir

  - Need function to contract filenames, e.g. change:

      /home/jeroen/junk
      /home/someoneelse/junk

    to:

      ~/junk
      ~someoneelse/junk

  - Perhaps also taking into account certain environment variables in the
    contraction function?
  - FXFile::copy( "C:\tmp", "c:\tmp\tmp" ) results infinite-loop.

*/


#ifndef TIMEFORMAT
#define TIMEFORMAT "%Y/%m/%d %H:%M:%S"
#endif



#ifdef WIN32
#include "WindowsGubbins.h"
// Replacement ftruncate() (as there's no 64 bit version of _chsize())
extern "C" static int ftruncate(int fh, FX::FXfval _newsize)
{
	long _h=_get_osfhandle(fh);
	if(-1==_h) return _h;
	HANDLE h=(HANDLE) _h;
	LARGE_INTEGER current={0}, newsize;
	current.LowPart=SetFilePointer(h, 0, &current.HighPart, FILE_CURRENT);
	if((DWORD) -1==current.LowPart) goto err;
	newsize.QuadPart=_newsize;
	if((DWORD) -1==SetFilePointer(h, newsize.LowPart, &newsize.HighPart, FILE_BEGIN)) goto err;
	if(0==SetEndOfFile(h)) goto err;
	if((DWORD) -1==SetFilePointer(h, current.LowPart, &current.HighPart, FILE_BEGIN)) goto err;
	return 0;
err:
	errno=EIO;	// Emulate POSIX
	return -1;
}
#endif


/*******************************************************************************/

namespace FX {

class FXDLLLOCAL FXFilePrivate : public FXMutex
{
public:
	FXString filename;
	bool amStdio;
	int handle;
	FXfval size;
	enum LastOp
	{
		NoOp=0,
		Read=1,
		Write=2
	} lastop;
	QByteArray ungetchbuffer;
	FXACL *acl;
	FXFilePrivate(bool _amStdio, bool doacl) : amStdio(_amStdio), handle(0), size(0), lastop(NoOp), acl(0)
	{
		if(doacl)
		{
			FXERRHM(acl=new FXACL(FXACL::default_(FXACL::File)));
		}
	}
	~FXFilePrivate()
	{
		FXDELETE(acl);
	}
};
static FXPtrHold<FXFile> stdiofile;

int FXFile::int_fileDescriptor() const
{
	return p->handle;
}

FXFile::FXFile() : p(0), FXIODevice()
{
	FXERRHM(p=new FXFilePrivate(false, true));
}

FXFile::FXFile(WantStdioType) : p(0), FXIODevice()
{	// Special FXFile talking to stdin/stdout
	FXERRHM(p=new FXFilePrivate(true, true));
	p->handle=fileno(stdout);
	setFlags(IO_ReadWrite|IO_Append|IO_Truncate|IO_Open);
}

FXFile::FXFile(const FXString &name, WantLightFXFile) : p(0), FXIODevice()
{	// Special FXFile for a "light" instance. This instance is much faster to create
	// and destroy but does not implement ACL's and so is private
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXFilePrivate(false, false));
	p->filename=name;
	unconstr.dismiss();
}

FXFile::FXFile(const FXString &name) : p(0), FXIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXFilePrivate(false, true));
	p->filename=name;
	unconstr.dismiss();
}

FXFile::~FXFile()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &FXFile::name() const
{
	// FXMtxHold h(p);	can do without
	return p->filename;
}

void FXFile::setName(const FXString &name)
{
	FXMtxHold h(p);
	if(!p->amStdio)
	{
		if(isOpen()) close();
		p->filename=name;
	}
}

bool FXFile::exists() const
{
	FXMtxHold h(p);
	if(p->amStdio) return true;
	return exists(p->filename)!=0;
}

bool FXFile::remove()
{
	FXMtxHold h(p);
	if(p->amStdio) return false;
	close();
	return remove(p->filename)!=0;
}

FXIODevice &FXFile::stdio(bool applyCRLFTranslation)
{
	if(!stdiofile)
	{
		FXERRHM(stdiofile=new FXFile(WantStdioType()));
	}
	if(applyCRLFTranslation)
		stdiofile->setMode(stdiofile->mode()|IO_Translate);
	else
		stdiofile->setMode(stdiofile->mode() & ~IO_Translate);
	return *stdiofile;
}

bool FXFile::open(FXuint mode)
{
	FXMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(FXIODevice::mode()!=mode) FXERRGIO(FXTrans::tr("FXFile", "Device reopen has different mode"));
	}
	else
	{
		FXThread_DTHold dth;
		int access=0;
		if(IO_ReadWrite==(mode & IO_ReadWrite)) access|=O_RDWR|O_CREAT;
		else
		{
			if(mode & IO_ReadOnly)  access|=O_RDONLY;
			if(mode & IO_WriteOnly) access|=O_WRONLY|O_CREAT|O_TRUNC;
		}
		if(mode & IO_Append) access|=O_APPEND;
		if(mode & IO_Truncate) access|=O_TRUNC;
		if(!(access & O_CREAT) && !exists()) FXERRGNF(FXTrans::tr("FXFile", "File '%1' not found").arg(p->filename), 0);
#ifdef WIN32
		access|=O_BINARY;
		// Annoyingly, you must create a file handle with WRITE_DAC permission to
		// be able to write the ACL so a bit of a workaround here
		HANDLE h;
		SECURITY_ATTRIBUTES sa={ sizeof(SECURITY_ATTRIBUTES) };
		SECURITY_ATTRIBUTES *psa=&sa;
		DWORD accessw=STANDARD_RIGHTS_ALL, creation=0;
		if((O_RDONLY & access)==O_RDONLY) accessw|=GENERIC_READ;
		if((O_WRONLY & access)==O_WRONLY) accessw|=GENERIC_WRITE;
		if((O_RDWR & access)==O_RDWR) accessw|=GENERIC_READ|GENERIC_WRITE;
		if(p->acl)
			sa.lpSecurityDescriptor=(SECURITY_DESCRIPTOR *) p->acl->int_toWin32SecurityDescriptor();
		else
			psa=0;
		switch((O_CREAT|O_TRUNC) & access)
		{
		case O_CREAT:
            creation|=OPEN_ALWAYS; break;
		case O_TRUNC:
			creation|=TRUNCATE_EXISTING; break;
		case O_CREAT|O_TRUNC:
			creation|=CREATE_ALWAYS; break;
		default:
			creation|=OPEN_EXISTING; break;
		}
		FXERRHWINFN(INVALID_HANDLE_VALUE!=(h=CreateFile(p->filename.text(), accessw, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, psa,
			creation, FILE_ATTRIBUTE_NORMAL, NULL)), p->filename);
		FXERRHIO(p->handle=_open_osfhandle((intptr_t) h, access));
#endif
#ifdef USE_POSIX
#ifdef HAVE_WIDEUNISTD
		FXERRHIOFN(p->handle=::wopen(p->filename.utext(), access, S_IREAD|S_IWRITE), p->filename);
#else
		FXERRHIOFN(p->handle=::open(p->filename.text(), access, S_IREAD|S_IWRITE), p->filename);
#endif
#endif
		if(access & O_CREAT)
		{	// Set the perms
			if(p->acl) p->acl->writeTo(p->handle);
		}
		if(p->acl) *p->acl=FXACL(p->handle, FXACL::File);
		setFlags((mode & IO_ModeMask)|IO_Open);
		p->lastop=FXFilePrivate::NoOp;
		p->size=size(p->filename);
		ioIndex=(mode & IO_Append) ? p->size : 0;
	}
	return true;
}

void FXFile::close()
{
	FXMtxHold h(p);
	if(isOpen() && !p->amStdio)
	{
		FXThread_DTHold dth;
		FXERRHIO(::close(p->handle));
		p->handle=0;
		p->size=0;
		if(p->acl)
		{	// Reset to default ACL
			FXDELETE(p->acl);
			FXERRHM(p->acl=new FXACL(FXACL::default_(FXACL::File)));
		}
		ioIndex=0;
		setFlags(0);
	}
}

void FXFile::flush()
{
	FXMtxHold h(p);
	if(isOpen() && isWriteable())
	{
		FXThread_DTHold dth;
#ifdef WIN32
		FXERRHWIN(FlushFileBuffers((HANDLE) _get_osfhandle(p->handle)));
		//FXERRHIO(::_commit(p->handle));
#endif
#ifdef USE_POSIX
		FXERRHIO(::fsync(p->handle));
#endif
		p->lastop=FXFilePrivate::NoOp;
	}
}

FXfval FXFile::size() const
{
	// FXMtxHold h(p); can do without
	if(isOpen() && !p->amStdio)
		return p->size;
	else
		return 0;
}

void FXFile::truncate(FXfval size)
{
	FXMtxHold h(p);
	if(!isWriteable()) FXERRGIO(FXTrans::tr("FXFile", "Not open for writing"));
	if(isOpen() && !p->amStdio)
	{
		FXThread_DTHold dth;
		if((mode() & IO_ShredTruncate) && size<p->size)
			shredData(size);
		if(ioIndex>size) at(size);
		FXERRHIO(::ftruncate(p->handle, size));
		p->size=size;
	}
}

FXfval FXFile::at() const
{
	FXMtxHold h(p);
	return p->amStdio ? 0 : ioIndex;
}

bool FXFile::at(FXfval newpos)
{
	FXMtxHold h(p);
	if(isOpen() && ioIndex!=newpos && !p->amStdio)
	{
		FXThread_DTHold dth;
#ifdef WIN32
		LARGE_INTEGER _newpos; _newpos.QuadPart=newpos;
		FXERRHWIN(SetFilePointerEx((HANDLE) _get_osfhandle(p->handle), _newpos, NULL, FILE_BEGIN));
#else
		FXERRHIO(::lseek(p->handle, newpos, SEEK_SET));
#endif
		ioIndex=newpos;
		if(ioIndex>p->size) p->size=ioIndex;
		p->ungetchbuffer.resize(0);
		return true;
	}
	return false;
}

bool FXFile::atEnd() const
{
	FXMtxHold h(p);
	if(!isOpen()) return true;
	// Unfortunately GNU/Linux doesn't have eof() and MSVC6's _eof() doesn't like file pointers > 4Gb!
	// You also need to stay compatible with when the input is stdin
	FXFile *me=(FXFile *) this;
	int c=me->getch();
	if(-1==c) return true;
	if(p->amStdio)
		me->ungetch(c);
	else
		me->at(ioIndex-1);
	return false;
}

const FXACL &FXFile::permissions() const
{
	assert(p->acl);
	return *p->acl;
}
void FXFile::setPermissions(const FXACL &perms)
{
	assert(p->acl);
	if(isOpen()) perms.writeTo(p->handle);
	*p->acl=perms;
}
FXACL FXFile::permissions(const FXString &path)
{
	return FXACL(path, isFile(path) ? FXACL::File : FXACL::Directory);
}
void FXFile::setPermissions(const FXString &path, const FXACL &perms)
{
	perms.writeTo(path);
}

FXuval FXFile::readBlock(char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!FXIODevice::isReadable()) FXERRGIO(FXTrans::tr("FXFile", "Not open for reading"));
	if(isOpen() && maxlen)
	{
		FXThread_DTHold dth;
		FXuval readed=0;
#ifdef USE_POSIX
		if(FXFilePrivate::Write==p->lastop && !p->amStdio)
		{
			FXERRHIO(::lseek(p->handle, 0, SEEK_CUR));
		}
#endif
		if(!p->ungetchbuffer.isEmpty())
		{
			FXuchar *ungetchdata=p->ungetchbuffer.data();
			FXuval ungetchlen=p->ungetchbuffer.size();
			FXuval copylen=FXMIN(ungetchlen, maxlen);
			memcpy(data, ungetchdata, copylen);
			readed+=copylen; ioIndex+=copylen;
			if(copylen<ungetchlen) memmove(ungetchdata, ungetchdata+copylen, ungetchlen-copylen);
			p->ungetchbuffer.resize(ungetchlen-copylen);
		}
		int ioreaded;
#ifdef WIN32
		FXERRHWIN(ReadFile((HANDLE) _get_osfhandle(p->handle), data+readed, maxlen-readed, (LPDWORD) &ioreaded, NULL));
#else
		FXERRHIO(ioreaded=::read((p->amStdio) ? fileno(stdin) : p->handle, data+readed, maxlen-readed));
#endif
		ioIndex+=ioreaded; readed+=ioreaded;
		p->lastop=FXFilePrivate::Read;
		if(isTranslated())
		{
			bool midNL;
			readed=removeCRLF(midNL, (FXuchar *) data, (FXuchar *) data, readed);
			if(midNL)
			{	// Ok, see what the next is
				char buff=0;
				if(readBlock(&buff, 1))
				{
					if(10!=buff) ungetch(buff);
				}
				data[readed]=10;
				return readed+1;
			}
		}
		return readed;
	}
	return 0;
}

FXuval FXFile::writeBlock(const char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!isWriteable()) FXERRGIO(FXTrans::tr("FXFile", "Not open for writing"));
	if(isOpen())
	{
		FXThread_DTHold dth;
		FXuval written;
#ifdef USE_POSIX
		if(FXFilePrivate::Read==p->lastop && !p->amStdio)
		{
			FXERRHIO(::lseek(p->handle, 0, SEEK_CUR));
			p->ungetchbuffer.resize(0);
		}
#endif
		if(isTranslated())
		{	// GNU/Linux open() doesn't support text translation
			FXuchar buffer[16384];
			FXuval writeidx;
			for(writeidx=0; writeidx<maxlen;)
			{
				bool midNL;
				FXuval inputlen=maxlen-writeidx;
#ifdef WIN32
				FXERRHWIN(WriteFile((HANDLE) _get_osfhandle(p->handle), buffer, applyCRLF(midNL, buffer, (FXuchar *) &data[writeidx], sizeof(buffer), inputlen), (LPDWORD) &written, NULL));
#else
				FXERRHIO(written=::write(p->handle, buffer, applyCRLF(midNL, buffer, (FXuchar *) &data[writeidx], sizeof(buffer), inputlen)));
#endif
				writeidx+=inputlen;
				ioIndex+=written;
			}
			written=writeidx;
		}
		else
		{
#ifdef WIN32
			FXERRHWIN(WriteFile((HANDLE) _get_osfhandle(p->handle), data, maxlen, (LPDWORD) &written, NULL));
#else
			FXERRHIO(written=::write(p->handle, data, maxlen));
#endif
			ioIndex+=written;
		}
		if(ioIndex>p->size) p->size=ioIndex;
		p->lastop=FXFilePrivate::Write;
		if(isRaw()) flush();
		return written;
	}
	return 0;
}

FXuval FXFile::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
	FXMtxHold h(p);
	at(pos);
	return readBlock(data, maxlen);
}
FXuval FXFile::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	at(pos);
	return writeBlock(data, maxlen);
}

int FXFile::ungetch(int c)
{
	FXMtxHold h(p);
	if(isOpen())
	{
		uint size=p->ungetchbuffer.size();
		p->ungetchbuffer.resize(size+1);
		p->ungetchbuffer[size]=(char) c;
		ioIndex-=1;
		return c;
	}
	return -1;
}

void FXFile::stampCreationMetadata(const FXString &path, FXTime creationdate)
{
#ifndef WIN32
	// If modification is older than access time, sets creation time to modification
	struct ::utimbuf times;
	times.actime=creationdate+1;		// accessed
	times.modtime=creationdate;			// modified
	FXERRHOS(::utime(path.text(), &times));		// Sets created
	times.actime=creationdate;			// accessed
	FXERRHOS(::utime(path.text(), &times));		// Sets modified and accessed
#else
	FXFile fh(path);
	fh.open(IO_ReadWrite);
	FILETIME ft;
	*(FXulong *)(&ft)=((FXulong) creationdate*10000000)+116444736000000000ULL;
	FXERRHWIN(SetFileTime((HANDLE) _get_osfhandle(fh.p->handle), &ft, &ft, &ft));
#endif
}

//*****************************************************************************

// Return value of environment variable name
FXString FXFile::getEnvironment(const FXString& name){
  return FXString(getenv(name.text()));
  }


// Get current user name
FXString FXFile::getCurrentUserName(){
#ifndef WIN32
#ifdef FOX_THREAD_SAFE
  struct passwd pwdresult,*pwd;
  char buffer[1024];
  if(getpwuid_r(geteuid(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd) return pwd->pw_name;
#else
  struct passwd *pwd=getpwuid(geteuid());
  if(pwd) return pwd->pw_name;
#endif
#else
  char buffer[1024];
  DWORD size=sizeof(buffer);
  if(GetUserName(buffer,&size)) return buffer;
#endif
  return FXString::null;
  }


// Get current working directory
FXString FXFile::getCurrentDirectory(){
  FXchar buffer[MAXPATHLEN];
#ifndef WIN32
  if(getcwd(buffer,MAXPATHLEN)) return FXString(buffer);
#else
  if(GetCurrentDirectory(MAXPATHLEN,buffer)) return FXString(buffer);
#endif
  return FXString::null;
  }


// Change current directory
FXbool FXFile::setCurrentDirectory(const FXString& path){
#ifdef WIN32
  return !path.empty() && SetCurrentDirectory(path.text());
#else
  return !path.empty() && chdir(path.text())==0;
#endif
  }


// Get current drive prefix "a:", if any
// This is the same method as used in VC++ CRT.
FXString FXFile::getCurrentDrive(){
#ifdef WIN32
  FXchar buffer[MAXPATHLEN];
  if(GetCurrentDirectory(MAXPATHLEN,buffer) && isalpha((FXuchar)buffer[0]) && buffer[1]==':') return FXString(buffer,2);
#endif
  return FXString::null;
  }


#ifdef WIN32

// Change current drive prefix "a:"
// This is the same method as used in VC++ CRT.
FXbool FXFile::setCurrentDrive(const FXString& prefix){
  FXchar buffer[3];
  if(!prefix.empty() && isalpha((FXuchar)prefix[0]) && prefix[1]==':'){
    buffer[0]=prefix[0];
    buffer[1]=':';
    buffer[2]='\0';
    return SetCurrentDirectory(buffer);
    }
  return FALSE;
  }

#else

// Change current drive prefix "a:"
FXbool FXFile::setCurrentDrive(const FXString&){
  return TRUE;
  }

#endif


// Get home directory for a given user
FXString FXFile::getUserDirectory(const FXString& user){
#ifndef WIN32
#ifdef FOX_THREAD_SAFE
  struct passwd pwdresult,*pwd;
  char buffer[1024];
  if(user.empty()){
    register const FXchar* str;
    if((str=getenv("HOME"))!=NULL) return str;
    if((str=getenv("USER"))!=NULL || (str=getenv("LOGNAME"))!=NULL){
      if(getpwnam_r(str,&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd) return pwd->pw_dir;
      }
    if(getpwuid_r(getuid(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd) return pwd->pw_dir;
    return PATHSEPSTRING;
    }
  if(getpwnam_r(user.text(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd) return pwd->pw_dir;
  return PATHSEPSTRING;
#else
  register struct passwd *pwd;
  if(user.empty()){
    register const FXchar* str;
    if((str=getenv("HOME"))!=NULL) return str;
    if((str=getenv("USER"))!=NULL || (str=getenv("LOGNAME"))!=NULL){
      if((pwd=getpwnam(str))!=NULL) return pwd->pw_dir;
      }
    if((pwd=getpwuid(getuid()))!=NULL) return pwd->pw_dir;
    return PATHSEPSTRING;
    }
  if((pwd=getpwnam(user.text()))!=NULL) return pwd->pw_dir;
  return PATHSEPSTRING;
#endif
#else
  if(user.empty()){
    register const FXchar *str1,*str2;
    if((str1=getenv("USERPROFILE"))!=NULL) return str1; // Daniël Hörchner <dbjh@gmx.net>
    if((str1=getenv("HOME"))!=NULL) return str1;
    if((str2=getenv("HOMEPATH"))!=NULL){      // This should be good for WinNT, Win2K according to MSDN
      if((str1=getenv("HOMEDRIVE"))==NULL) str1="c:";
      return FXString(str1,str2);
      }
//  FXchar buffer[MAX_PATH]
//  if(SHGetFolderPath(NULL,CSIDL_PERSONAL|CSIDL_FLAG_CREATE,NULL,O,buffer)==S_OK){
//    return buffer;
//    }
    HKEY hKey;
    if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,KEY_READ,&hKey)==ERROR_SUCCESS){
      FXchar home[MAXPATHLEN];
      DWORD size=MAXPATHLEN;
      LONG result=RegQueryValueEx(hKey,"Personal",NULL,NULL,(LPBYTE)home,&size);  // Change "Personal" to "Desktop" if you want...
      RegCloseKey(hKey);
      if(result==ERROR_SUCCESS) return home;
      }
    return "c:" PATHSEPSTRING;
    }
  return "c:" PATHSEPSTRING;
#endif
  }


// Return the home directory for the current user.
FXString FXFile::getHomeDirectory(){
  return getUserDirectory(FXString::null);
  }


// Get executable path
FXString FXFile::getExecPath(){
  return FXString(getenv("PATH"));
  }


// Return temporary directory.
FXString FXFile::getTempDirectory(){
#ifndef WIN32
  // Conform Linux File Hierarchy standard; this should be
  // good for SUN, SGI, HP-UX, AIX, and OSF1 also.
  return FXString("/tmp",5);
#else
  FXchar buffer[MAXPATHLEN];
  FXuint len=GetTempPath(MAXPATHLEN,buffer);
  if(1<len && ISPATHSEP(buffer[len-1]) && !ISPATHSEP(buffer[len-2])) len--;
  return FXString(buffer,len);
#endif
  }


// Return directory part of pathname, assuming full pathname.
// Note that directory("/bla/bla/") is "/bla/bla" and NOT "/bla".
// However, directory("/bla/bla") is "/bla" as we expect!
FXString FXFile::directory(const FXString& file){
  register FXint n,i;
  if(!file.empty()){
    i=0;
#ifdef WIN32
    if(isalpha((FXuchar)file[0]) && file[1]==':') i=2;
#endif
    if(ISPATHSEP(file[i])) i++;
    n=i;
    while(file[i]){
      if(ISPATHSEP(file[i])) n=i;
      i++;
      }
    return FXString(file.text(),n);
    }
  return FXString::null;
  }


// Return name and extension part of pathname.
// Note that name("/bla/bla/") is "" and NOT "bla".
// However, name("/bla/bla") is "bla" as we expect!
FXString FXFile::name(const FXString& file){
  register FXint f,n;
  if(!file.empty()){
    n=0;
#ifdef WIN32
    if(isalpha((FXuchar)file[0]) && file[1]==':') n=2;
#endif
    f=n;
    while(file[n]){
      if(ISPATHSEP(file[n])) f=n+1;
      n++;
      }
    return FXString(file.text()+f,n-f);
    }
  return FXString::null;
  }


// Return file title, i.e. document name only:
//
//  /path/aa        -> aa
//  /path/aa.bb     -> aa
//  /path/aa.bb.cc  -> aa.bb
//  /path/.aa       -> .aa
FXString FXFile::title(const FXString& file){
  register FXint f,e,b,i;
  if(!file.empty()){
    i=0;
#ifdef WIN32
    if(isalpha((FXuchar)file[0]) && file[1]==':') i=2;
#endif
    f=i;
    while(file[i]){
      if(ISPATHSEP(file[i])) f=i+1;
      i++;
      }
    b=f;
    if(file[b]=='.') b++;     // Leading '.'
    e=i;
    while(b<i){
      if(file[--i]=='.'){ e=i; break; }
      }
    return FXString(file.text()+f,e-f);
    }
  return FXString::null;
  }


// Return extension, if there is one:
//
//  /path/aa        -> ""
//  /path/aa.bb     -> bb
//  /path/aa.bb.cc  -> cc
//  /path/.aa       -> ""
FXString FXFile::extension(const FXString& file){
  register FXint f,e,i,n;
  if(!file.empty()){
    n=0;
#ifdef WIN32
    if(isalpha((FXuchar)file[0]) && file[1]==':') n=2;
#endif
    f=n;
    while(file[n]){
      if(ISPATHSEP(file[n])) f=n+1;
      n++;
      }
    if(file[f]=='.') f++;     // Leading '.'
    e=i=n;
    while(f<i){
      if(file[--i]=='.'){ e=i+1; break; }
      }
    return FXString(file.text()+e,n-e);
    }
  return FXString::null;
  }


// Return file name less the extension
//
//  /path/aa        -> /path/aa
//  /path/aa.bb     -> /path/aa
//  /path/aa.bb.cc  -> /path/aa.bb
//  /path/.aa       -> /path/.aa
FXString FXFile::stripExtension(const FXString& file){
  register FXint f,e,n;
  if(!file.empty()){
    n=0;
#ifdef WIN32
    if(isalpha((FXuchar)file[0]) && file[1]==':') n=2;
#endif
    f=n;
    while(file[n]){
      if(ISPATHSEP(file[n])) f=n+1;
      n++;
      }
    if(file[f]=='.') f++;     // Leading '.'
    e=n;
    while(f<n){
      if(file[--n]=='.'){ e=n; break; }
      }
    return FXString(file.text(),e);
    }
  return FXString::null;
  }


#ifdef WIN32

// Return drive letter prefix "c:"
FXString FXFile::drive(const FXString& file){
  FXchar buffer[3];
  if(isalpha((FXuchar)file[0]) && file[1]==':'){
    buffer[0]=tolower((FXuchar)file[0]);
    buffer[1]=':';
    buffer[2]='\0';
    return FXString(buffer,2);
    }
  return FXString::null;
  }

#else

// Return drive letter prefix "c:"
FXString FXFile::drive(const FXString&){
  return FXString::null;
  }

#endif

// Perform tilde or environment variable expansion
FXString FXFile::expand(const FXString& file){
#ifndef WIN32
  if(!file.empty()){
    register FXint b,e,n;
    FXString result;

    // Expand leading tilde of the form ~/filename or ~user/filename
    n=0;
    if(file[n]=='~'){
      n++;
      b=n;
      while(file[n] && !ISPATHSEP(file[n])) n++;
      e=n;
      result.append(getUserDirectory(file.mid(b,e-b)));
      }

    // Expand environment variables of the form $HOME, ${HOME}, or $(HOME)
    while(file[n]){
      if(file[n]=='$'){
        n++;
        if(file[n]=='{' || file[n]=='(') n++;
        b=n;
        while(isalnum((FXuchar)file[n]) || file[n]=='_') n++;
        e=n;
        if(file[n]=='}' || file[n]==')') n++;
        result.append(getEnvironment(file.mid(b,e-b)));
        continue;
        }
      result.append(file[n]);
      n++;
      }
    return result;
    }
  return FXString::null;
#else
  if(!file.empty()){
    FXchar buffer[2048];

    // Expand environment variables of the form %HOMEPATH%
    if(ExpandEnvironmentStrings(file.text(),buffer,sizeof(buffer))){
      return buffer;
      }
    return file;
    }
  return FXString::null;
#endif
  }



// Simplify a file path; the path will remain relative if it was relative,
// or absolute if it was absolute.  Also, a trailing "/" will be preserved
// as this is important in other functions.
//
// Examples:
//
//  /aa/bb/../cc    -> /aa/cc
//  /aa/bb/../cc/   -> /aa/cc/
//  /aa/bb/../..    -> /
//  ../../bb        -> ../../bb
//  ../../bb/       -> ../../bb/
//  /../            -> /
//  ./aa/bb/../../  -> ./
//  a/..            -> .
//  a/../           -> ./
//  ./a             -> ./a
//  /////./././     -> /
//  c:/../          -> c:/
//  c:a/..          -> c:
//  /.              -> /
FXString FXFile::simplify(const FXString& file){
  if(!file.empty()){
    FXString result=file;
    register FXint p,q,s;
    p=q=0;
#ifndef WIN32
    if(ISPATHSEP(result[q])){
      result[p++]=PATHSEP;
      while(ISPATHSEP(result[q])) q++;
      }
#else
    if(ISPATHSEP(result[q])){         // UNC
      result[p++]=PATHSEP;
      q++;
      if(ISPATHSEP(result[q])){
        result[p++]=PATHSEP;
        while(ISPATHSEP(result[q])) q++;
        }
      }
    else if(isalpha((FXuchar)result[q]) && result[q+1]==':'){
      result[p++]=result[q++];
      result[p++]=':';
      q++;
      if(ISPATHSEP(result[q])){
        result[p++]=PATHSEP;
        while(ISPATHSEP(result[q])) q++;
        }
      }
#endif
    s=p;
    while(result[q]){
      while(result[q] && !ISPATHSEP(result[q])){
        result[p++]=result[q++];
        }
      if(2<=p && result[p-1]=='.' && ISPATHSEP(result[p-2]) && result[q]==0){
        p-=1;
        }
      else if(2<=p && result[p-1]=='.' && ISPATHSEP(result[p-2]) && ISPATHSEP(result[q])){
        p-=2;
        }
      else if(3<=p && result[p-1]=='.' && result[p-2]=='.' && ISPATHSEP(result[p-3]) && !(5<=p && result[p-4]=='.' && result[p-5]=='.')){
        p-=2;
        if(s+2<=p){
          p-=2;
          while(s<p && !ISPATHSEP(result[p])) p--;
          if(p==0) result[p++]='.';
          }
        }
      if(ISPATHSEP(result[q])){
        while(ISPATHSEP(result[q])) q++;
        if(!ISPATHSEP(result[p-1])) result[p++]=PATHSEP;
        }
      }
    return result.trunc(p);
    }
  return FXString::null;
  }


// Build absolute pathname
FXString FXFile::absolute(const FXString& file){
  if(file.empty()) return FXFile::getCurrentDirectory();
#ifndef WIN32
  if(ISPATHSEP(file[0])) return FXFile::simplify(file);
#else
  if(ISPATHSEP(file[0])){
    if(ISPATHSEP(file[1])) return FXFile::simplify(file);   // UNC
    return FXFile::simplify(FXFile::getCurrentDrive()+file);
    }
  if(isalpha((FXuchar)file[0]) && file[1]==':'){
    if(ISPATHSEP(file[2])) return FXFile::simplify(file);
    return FXFile::simplify(file.mid(0,2)+PATHSEPSTRING+file.mid(2,2147483647));
    }
#endif
  return FXFile::simplify(FXFile::getCurrentDirectory()+PATHSEPSTRING+file);
  }


// Build absolute pathname from parts
FXString FXFile::absolute(const FXString& base,const FXString& file){
  if(file.empty()) return FXFile::absolute(base);
#ifndef WIN32
  if(ISPATHSEP(file[0])) return FXFile::simplify(file);
#else
  if(ISPATHSEP(file[0])){
    if(ISPATHSEP(file[1])) return FXFile::simplify(file);   // UNC
    return FXFile::simplify(FXFile::getCurrentDrive()+file);
    }
  if(isalpha((FXuchar)file[0]) && file[1]==':'){
    if(ISPATHSEP(file[2])) return FXFile::simplify(file);
    return FXFile::simplify(file.mid(0,2)+PATHSEPSTRING+file.mid(2,2147483647));
    }
#endif
  return FXFile::simplify(FXFile::absolute(base)+PATHSEPSTRING+file);
  }


#ifndef WIN32

// Return root of given path; this is just "/" or "" if not absolute
FXString FXFile::root(const FXString& file){
  if(ISPATHSEP(file[0])){
    return PATHSEPSTRING;
    }
  return FXString::null;
  }

#else

// Return root of given path; this may be "\\" or "C:\" or "" if not absolute
FXString FXFile::root(const FXString& file){
  if(ISPATHSEP(file[0])){
    if(ISPATHSEP(file[1])) return PATHSEPSTRING PATHSEPSTRING;   // UNC
    return FXFile::getCurrentDrive()+PATHSEPSTRING;
    }
  if(isalpha((FXuchar)file[0]) && file[1]==':'){
    if(ISPATHSEP(file[2])) return file.left(3);
    return file.left(2)+PATHSEPSTRING;
    }
  return FXString::null;
  }

#endif


// Return relative path of file to given base directory
//
// Examples:
//
//  Base       File         Result
//  /a/b/c     /a/b/c/d     d
//  /a/b/c/    /a/b/c/d     d
//  /a/b/c/d   /a/b/c       ../
//  ../a/b/c   ../a/b/c/d   d
//  /a/b/c/d   /a/b/q       ../../q
//  /a/b/c     /a/b/c       .
//  /a/b/c/    /a/b/c/      .
//  ./a        ./b          ../b
//  a          b            ../b
FXString FXFile::relative(const FXString& base,const FXString& file){
  register FXint p,q,b;
  FXString result;

  // Find branch point
#ifndef WIN32
  for(p=b=0; base[p] && base[p]==file[p]; p++){
    if(ISPATHSEP(file[p])) b=p;
    }
#else
  for(p=b=0; base[p] && tolower((FXuchar)base[p])==tolower((FXuchar)file[p]); p++){
    if(ISPATHSEP(file[p])) b=p;
    }
#endif

  // Paths are equal
  if((base[p]=='\0' || (ISPATHSEP(base[p]) && base[p+1]=='\0')) && (file[p]=='\0' || (ISPATHSEP(file[p]) && file[p+1]=='\0'))){
    return ".";
    }

  // Directory base is prefix of file
  if((base[p]=='\0' && ISPATHSEP(file[p])) || (file[p]=='\0' && ISPATHSEP(base[p]))){
    b=p;
    }

  // Up to branch point
  for(p=q=b; base[p]; p=q){
    while(base[q] && !ISPATHSEP(base[q])) q++;
    if(q>p) result.append(".." PATHSEPSTRING);
    while(base[q] && ISPATHSEP(base[q])) q++;
    }

  // Strip leading path character off, if any
  while(ISPATHSEP(file[b])) b++;

  // Append tail end
  result.append(&file[b]);

  return result;
  }


// Return relative path of file to the current directory
FXString FXFile::relative(const FXString& file){
  return FXFile::relative(getCurrentDirectory(),file);
  }


// Generate unique filename of the form pathnameXXX.ext, where
// pathname.ext is the original input file, and XXX is a number,
// possibly empty, that makes the file unique.
// (From: Mathew Robertson <mathew.robertson@mi-services.com>)
FXString FXFile::unique(const FXString& file){
  if(!exists(file)) return file;
  FXString ext=extension(file);
  FXString path=stripExtension(file);           // Use the new API (Jeroen)
  FXString filename;
  register FXint count=0;
  if(!ext.empty()) ext.prepend('.');            // Only add period when non-empty extension
  while(count<1000){
    filename.format("%s%i%s",path.text(),count,ext.text());
    if(!exists(filename)) return filename;      // Return result here (Jeroen)
    count++;
    }
  return FXString::null;
  }


// Search pathlist for file
FXString FXFile::search(const FXString& pathlist,const FXString& file){
  if(!file.empty()){
    FXString path;
    FXint beg,end;
#ifndef WIN32
    if(ISPATHSEP(file[0])){
      if(exists(file)) return file;
      return FXString::null;
      }
#else
    if(ISPATHSEP(file[0])){
      if(ISPATHSEP(file[1])){
        if(exists(file)) return file;   // UNC
        return FXString::null;
        }
      path=FXFile::getCurrentDrive()+file;
      if(exists(path)) return path;
      return FXString::null;
      }
    if(isalpha((FXuchar)file[0]) && file[1]==':'){
      if(exists(file)) return file;
      return FXString::null;
      }
#endif
    for(beg=0; pathlist[beg]; beg=end){
      while(pathlist[beg]==PATHLISTSEP) beg++;
      for(end=beg; pathlist[end] && pathlist[end]!=PATHLISTSEP; end++);
      if(beg==end) break;
      path=absolute(pathlist.mid(beg,end-beg),file);
      if(exists(path)) return path;
      }
    }
  return FXString::null;
  }


// Up one level, given absolute path
FXString FXFile::upLevel(const FXString& file){
  if(!file.empty()){
    FXint beg=0;
    FXint end=file.length();
#ifndef WIN32
    if(ISPATHSEP(file[0])) beg++;
#else
    if(ISPATHSEP(file[0])){
      beg++;
      if(ISPATHSEP(file[1])) beg++;     // UNC
      }
    else if(isalpha((FXuchar)file[0]) && file[1]==':'){
      beg+=2;
      if(ISPATHSEP(file[2])) beg++;
      }
#endif
    if(beg<end && ISPATHSEP(file[end-1])) end--;
    while(beg<end){ --end; if(ISPATHSEP(file[end])) break; }
    return file.left(end);
    }
  return PATHSEPSTRING;
  }


// Check if file represents absolute pathname
FXbool FXFile::isAbsolute(const FXString& file){
#ifndef WIN32
  return !file.empty() && ISPATHSEP(file[0]);
#else
  return !file.empty() && (ISPATHSEP(file[0]) || (isalpha((FXuchar)file[0]) && file[1]==':'));
#endif
  }


// Does file represent topmost directory
FXbool FXFile::isTopDirectory(const FXString& file){
#ifndef WIN32
  return !file.empty() && ISPATHSEP(file[0]) && file[1]=='\0';
#else
  return !file.empty() && ((ISPATHSEP(file[0]) && (file[1]=='\0' || (ISPATHSEP(file[1]) && file[2]=='\0'))) || (isalpha((FXuchar)file[0]) && file[1]==':' && (file[2]=='\0' || (ISPATHSEP(file[2]) && file[3]=='\0'))));
#endif
  }


// Check if file represents a file
FXbool FXFile::isFile(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && S_ISREG(status.st_mode);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF) && !(atts&FILE_ATTRIBUTE_DIRECTORY);
#endif
  }


// Check if file represents a link
FXbool FXFile::isLink(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::lstat(file.text(),&status)==0) && S_ISLNK(status.st_mode);
#else
  return FALSE;
#endif
  }


// Check if file represents a file share
FXbool FXFile::isShare(const FXString& file){
#ifndef WIN32
  return FALSE;
#else
  return ISPATHSEP(file[0]) && ISPATHSEP(file[1]) && file.find(PATHSEP,2)<0;
#endif
  }


/*


// Return true if input path represents a file share of the form "\\" or "\\server"
FXbool FXFile::isShare(const FXString& file){
#ifndef WIN32
  return FALSE;
#else
  if(ISPATHSEP(file[0]) && ISPATHSEP(file[1]) && file.find(PATHSEP,2)<0){
    HANDLE hEnum;
    NETRESOURCE host;
    host.dwScope=RESOURCE_GLOBALNET;
    host.dwType=RESOURCETYPE_DISK;
    host.dwDisplayType=RESOURCEDISPLAYTYPE_GENERIC;
    host.dwUsage=RESOURCEUSAGE_CONTAINER;
    host.lpLocalName=NULL;
    host.lpRemoteName=(char*)file.text();
    host.lpComment=NULL;
    host.lpProvider=NULL;

    // This shit thows "First-chance exception in blabla.exe (KERNEL32.DLL): 0x000006BA: (no name)"
    // when non-existing server name is passed in.  Don't know if this is dangerous...
    if(WNetOpenEnum((file[2]?RESOURCE_GLOBALNET:RESOURCE_CONTEXT),RESOURCETYPE_DISK,0,(file[2]?&host:NULL),&hEnum)==NO_ERROR){
      WNetCloseEnum(hEnum);
      return TRUE;
      }
    }
  return FALSE;
#endif
  }
*/


// Check if file represents a directory
FXbool FXFile::isDirectory(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && S_ISDIR(status.st_mode);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF) && (atts&FILE_ATTRIBUTE_DIRECTORY);
#endif
  }


// Return true if file is readable (thanks to gehriger@linkcad.com)
FXbool FXFile::isReadable(const FXString& file){
  return !file.empty() && access(file.text(),R_OK)==0;
  }


// Return true if file is writable (thanks to gehriger@linkcad.com)
FXbool FXFile::isWritable(const FXString& file){
  return !file.empty() && access(file.text(),W_OK)==0;
  }


// Return true if file is executable (thanks to gehriger@linkcad.com)
FXbool FXFile::isExecutable(const FXString& file){
#ifndef WIN32
  return !file.empty() && access(file.text(),X_OK)==0;
#else
  SHFILEINFO sfi;
  return !file.empty() && SHGetFileInfo(file.text(),0,&sfi,sizeof(SHFILEINFO),SHGFI_EXETYPE)!=0;
#endif
  }


// Check if owner has full permissions
FXbool FXFile::isOwnerReadWriteExecute(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IRUSR) && (status.st_mode&S_IWUSR) && (status.st_mode&S_IXUSR);
#else
  return TRUE;
#endif
  }


// Check if owner can read
FXbool FXFile::isOwnerReadable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IRUSR);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF);
#endif
  }


// Check if owner can write
FXbool FXFile::isOwnerWritable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IWUSR);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF) && !(atts&FILE_ATTRIBUTE_READONLY);
#endif
  }


// Check if owner can execute
FXbool FXFile::isOwnerExecutable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IXUSR);
#else
  SHFILEINFO sfi;
  return !file.empty() && SHGetFileInfo(file.text(),0,&sfi,sizeof(SHFILEINFO),SHGFI_EXETYPE)!=0;
#endif
  }


// Check if group has full permissions
FXbool FXFile::isGroupReadWriteExecute(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IRGRP) && (status.st_mode&S_IWGRP) && (status.st_mode&S_IXGRP);
#else
  return TRUE;
#endif
  }


// Check if group can read
FXbool FXFile::isGroupReadable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IRGRP);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF);
#endif
  }


// Check if group can write
FXbool FXFile::isGroupWritable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IWGRP);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF) && !(atts&FILE_ATTRIBUTE_READONLY);
#endif
  }


// Check if group can execute
FXbool FXFile::isGroupExecutable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IXGRP);
#else
  SHFILEINFO sfi;
  return !file.empty() && SHGetFileInfo(file.text(),0,&sfi,sizeof(SHFILEINFO),SHGFI_EXETYPE)!=0;
#endif
  }


// Check if everybody has full permissions
FXbool FXFile::isOtherReadWriteExecute(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IROTH) && (status.st_mode&S_IWOTH) && (status.st_mode&S_IXOTH);
#else
  return TRUE;
#endif
  }


// Check if everybody can read
FXbool FXFile::isOtherReadable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IROTH);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF);
#endif
  }


// Check if everybody can write
FXbool FXFile::isOtherWritable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IWOTH);
#else
  DWORD atts;
  return !file.empty() && ((atts=GetFileAttributes(file.text()))!=0xFFFFFFFF) && !(atts&FILE_ATTRIBUTE_READONLY);
#endif
  }


// Check if everybody can execute
FXbool FXFile::isOtherExecutable(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_IXOTH);
#else
  SHFILEINFO sfi;
  return !file.empty() && SHGetFileInfo(file.text(),0,&sfi,sizeof(SHFILEINFO),SHGFI_EXETYPE)!=0;
#endif
  }


// These 5 functions below contributed by calvin@users.sourceforge.net


// Test if suid bit set
FXbool FXFile::isSetUid(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_ISUID);
#else
  return FALSE;
#endif
  }


// Test if sgid bit set
FXbool FXFile::isSetGid(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_ISGID);
#else
  return FALSE;
#endif
  }


// Test if sticky bit set
FXbool FXFile::isSetSticky(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) && (status.st_mode&S_ISVTX);
#else
  return FALSE;
#endif
  }


// Return owner name from uid
FXString FXFile::owner(FXuint uid){
  FXchar result[64];
#ifndef WIN32
#ifdef FOX_THREAD_SAFE
  struct passwd pwdresult,*pwd;
  char buffer[1024];
  if(getpwuid_r(uid,&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd) return pwd->pw_name;
#else
  struct passwd *pwd=getpwuid(uid);
  if(pwd) return pwd->pw_name;
#endif
#endif
  sprintf(result,"%u",uid);
  return result;
  }


// Return group name from gid
FXString FXFile::group(FXuint gid){
  FXchar result[64];
#ifndef WIN32
#ifdef FOX_THREAD_SAFE
  ::group grpresult;
  ::group *grp;
  char buffer[1024];
  if(getgrgid_r(gid,&grpresult,buffer,sizeof(buffer),&grp)==0 && grp) return grp->gr_name;
#else
  ::group *grp=getgrgid(gid);
  if(grp) return grp->gr_name;
#endif
#endif
  sprintf(result,"%u",gid);
  return result;
  }


// Return owner name of file
FXString FXFile::owner(const FXString& file){
  struct stat status;
  if(!file.empty() && ::stat(file.text(),&status)==0){
    return FXFile::owner(status.st_uid);
    }
  return FXString::null;
  }


// Return group name of file
FXString FXFile::group(const FXString& file){
  struct stat status;
  if(!file.empty() && ::stat(file.text(),&status)==0){
    return FXFile::group(status.st_gid);
    }
  return FXString::null;
  }


/// Return permissions string
FXString FXFile::permissions(FXuint mode){
  FXchar result[11];
#ifndef WIN32
  result[0]=S_ISLNK(mode) ? 'l' : S_ISREG(mode) ? '-' : S_ISDIR(mode) ? 'd' : S_ISCHR(mode) ? 'c' : S_ISBLK(mode) ? 'b' : S_ISFIFO(mode) ? 'p' : S_ISSOCK(mode) ? 's' : '?';
  result[1]=(mode&S_IRUSR) ? 'r' : '-';
  result[2]=(mode&S_IWUSR) ? 'w' : '-';
  result[3]=(mode&S_ISUID) ? 's' : (mode&S_IXUSR) ? 'x' : '-';
  result[4]=(mode&S_IRGRP) ? 'r' : '-';
  result[5]=(mode&S_IWGRP) ? 'w' : '-';
  result[6]=(mode&S_ISGID) ? 's' : (mode&S_IXGRP) ? 'x' : '-';
  result[7]=(mode&S_IROTH) ? 'r' : '-';
  result[8]=(mode&S_IWOTH) ? 'w' : '-';
  result[9]=(mode&S_ISVTX) ? 't' : (mode&S_IXOTH) ? 'x' : '-';
  result[10]=0;
#else
  result[0]='-';
#ifdef _S_IFDIR
  if(mode&_S_IFDIR) result[0]='d';
#endif
#ifdef _S_IFCHR
  if(mode&_S_IFCHR) result[0]='c';
#endif
#ifdef _S_IFIFO
  if(mode&_S_IFIFO) result[0]='p';
#endif
  result[1]='r';
  result[2]='w';
  result[3]='x';
  result[4]='r';
  result[5]='w';
  result[6]='x';
  result[7]='r';
  result[8]='w';
  result[9]='x';
  result[10]=0;
#endif
  return result;
  }

/*
// Convert FILETIME (# 100ns since 01/01/1601) to time_t (# s since 01/01/1970)
static time_t fxfiletime(const FILETIME& ft){
  FXlong ll=(((FXlong)ft.dwHighDateTime)<<32) | (FXlong)ft.dwLowDateTime;
#if defined(__CYGWIN__) || defined(__MINGW32__)
  ll=ll-116444736000000000LL;
#else
  ll=ll-116444736000000000L;    // 0x19DB1DED53E8000
#endif
  ll=ll/10000000;
  if(ll<0) ll=0;
  return (time_t)ll;
  }
*/

//    hFile=CreateFile(pathname,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
//    if(hFile!=INVALID_HANDLE_VALUE){
//      GetFileTime(hFile,NULL,NULL,&ftLastWriteTime);
//      CloseHandle(hFile);

//       filetime=fxfiletime(ftLastWriteTime);

// Return time file was last modified
FXTime FXFile::modified(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXTime)status.st_mtime : 0L;
#else
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXTime)status.st_mtime : 0L;
#endif
  }


// Return time file was last accessed
FXTime FXFile::accessed(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXTime)status.st_atime : 0L;
#else
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXTime)status.st_atime : 0L;
#endif
  }


// Return time when created
FXTime FXFile::created(const FXString& file){
#ifndef WIN32
  return 0L;
#else
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXTime)status.st_ctime : 0L;
#endif
  }


// Return time when "touched"
FXTime FXFile::touched(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXTime)FXMAX(status.st_ctime,status.st_mtime) : 0L;
#else
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXTime)FXMAX(status.st_ctime,status.st_mtime) : 0L;
#endif
  }



#ifndef WIN32                 // UNIX


// List all the files in directory
FXint FXFile::listFiles(FXString*& filelist,const FXString& path,const FXString& pattern,FXuint flags){
  FXuint matchmode=FILEMATCH_FILE_NAME|FILEMATCH_NOESCAPE;
  FXString pathname;
  FXString name;
  struct dirent *dp;
  FXString *newlist;
  FXint count=0;
  FXint size=0;
  DIR *dirp;
  struct stat inf;

  // Initialize to empty
  filelist=NULL;
/*
  // One single root under Unix
  if(path.empty()){
    filelist=new FXString[2];
    list[count++]=PATHSEPSTRING;
    return count;
    }
*/
  // Folding case
  if(flags&LIST_CASEFOLD) matchmode|=FILEMATCH_CASEFOLD;

  // Get directory stream pointer
  dirp=opendir(path.text());
  if(dirp){

    // Loop over directory entries
#ifdef FOX_THREAD_SAFE
    struct fxdirent dirresult;
    while(!readdir_r(dirp,&dirresult,&dp) && dp){
#else
    while((dp=readdir(dirp))!=NULL){
#endif

      // Get name
      name=dp->d_name;

      // Build full pathname
      pathname=path;
      if(!ISPATHSEP(pathname[pathname.length()-1])) pathname+=PATHSEPSTRING;
      pathname+=name;

      // Get info on file
      if(!info(pathname,inf)) continue;

      // Filter out files; a bit tricky...
      if(!S_ISDIR(inf.st_mode) && ((flags&LIST_NO_FILES) || (name[0]=='.' && !(flags&LIST_HIDDEN_FILES)) || (!(flags&LIST_ALL_FILES) && !match(pattern,name,matchmode)))) continue;

      // Filter out directories; even more tricky!
      if(S_ISDIR(inf.st_mode) && ((flags&LIST_NO_DIRS) || (name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0 && (flags&LIST_NO_PARENT)) || (name[1]!='.' && !(flags&LIST_HIDDEN_DIRS)))) || (!(flags&LIST_ALL_DIRS) && !match(pattern,name,matchmode)))) continue;

      // Grow list
      if(count+1>=size){
        size=size?(size<<1):256;
        newlist=new FXString [size];
        for(int i=0; i<count; i++) newlist[i]=filelist[i];
        delete [] filelist;
        filelist=newlist;
        }

      // Add to list
      filelist[count++]=name;
      }
    closedir(dirp);
    }
  return count;
  }


#else                         // WINDOWS


// List all the files in directory
FXint FXFile::listFiles(FXString*& filelist,const FXString& path,const FXString& pattern,FXuint flags){
  FXuint matchmode=FILEMATCH_FILE_NAME|FILEMATCH_NOESCAPE;
  FXString pathname;
  FXString name;
  FXString *newlist;
  FXint count=0;
  FXint size=0;
  WIN32_FIND_DATA ffData;
  DWORD nCount,nSize,i,j;
  HANDLE hFindFile,hEnum;
  FXchar server[200];

  // Initialize to empty
  filelist=NULL;

/*
  // Each drive is a root on windows
  if(path.empty()){
    FXchar letter[4];
    letter[0]='a';
    letter[1]=':';
    letter[2]=PATHSEP;
    letter[3]='\0';
    filelist=new FXString[28];
    for(DWORD mask=GetLogicalDrives(); mask; mask>>=1,letter[0]++){
      if(mask&1) list[count++]=letter;
      }
    filelist[count++]=PATHSEPSTRING PATHSEPSTRING;    // UNC for file shares
    return count;
    }
*/
/*
  // A UNC name was given of the form "\\" or "\\server"
  if(ISPATHSEP(path[0]) && ISPATHSEP(path[1]) && path.find(PATHSEP,2)<0){
    NETRESOURCE host;

    // Fill in
    host.dwScope=RESOURCE_GLOBALNET;
    host.dwType=RESOURCETYPE_DISK;
    host.dwDisplayType=RESOURCEDISPLAYTYPE_GENERIC;
    host.dwUsage=RESOURCEUSAGE_CONTAINER;
    host.lpLocalName=NULL;
    host.lpRemoteName=(char*)path.text();
    host.lpComment=NULL;
    host.lpProvider=NULL;

    // Open network enumeration
    if(WNetOpenEnum((path[2]?RESOURCE_GLOBALNET:RESOURCE_CONTEXT),RESOURCETYPE_DISK,0,(path[2]?&host:NULL),&hEnum)==NO_ERROR){
      NETRESOURCE resource[16384/sizeof(NETRESOURCE)];
      FXTRACE((1,"Enumerating=%s\n",path.text()));
      while(1){
        nCount=-1;    // Read as many as will fit
        nSize=sizeof(resource);
        if(WNetEnumResource(hEnum,&nCount,resource,&nSize)!=NO_ERROR) break;
        for(i=0; i<nCount; i++){

          // Dump what we found
          FXTRACE((1,"dwScope=%s\n",resource[i].dwScope==RESOURCE_CONNECTED?"RESOURCE_CONNECTED":resource[i].dwScope==RESOURCE_GLOBALNET?"RESOURCE_GLOBALNET":resource[i].dwScope==RESOURCE_REMEMBERED?"RESOURCE_REMEMBERED":"?"));
          FXTRACE((1,"dwType=%s\n",resource[i].dwType==RESOURCETYPE_ANY?"RESOURCETYPE_ANY":resource[i].dwType==RESOURCETYPE_DISK?"RESOURCETYPE_DISK":resource[i].dwType==RESOURCETYPE_PRINT?"RESOURCETYPE_PRINT":"?"));
          FXTRACE((1,"dwDisplayType=%s\n",resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_DOMAIN?"RESOURCEDISPLAYTYPE_DOMAIN":resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_SERVER?"RESOURCEDISPLAYTYPE_SERVER":resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_SHARE?"RESOURCEDISPLAYTYPE_SHARE":resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_GENERIC?"RESOURCEDISPLAYTYPE_GENERIC":resource[i].dwDisplayType==6?"RESOURCEDISPLAYTYPE_NETWORK":resource[i].dwDisplayType==7?"RESOURCEDISPLAYTYPE_ROOT":resource[i].dwDisplayType==8?"RESOURCEDISPLAYTYPE_SHAREADMIN":resource[i].dwDisplayType==9?"RESOURCEDISPLAYTYPE_DIRECTORY":resource[i].dwDisplayType==10?"RESOURCEDISPLAYTYPE_TREE":resource[i].dwDisplayType==11?"RESOURCEDISPLAYTYPE_NDSCONTAINER":"?"));
          FXTRACE((1,"dwUsage=%s\n",resource[i].dwUsage==RESOURCEUSAGE_CONNECTABLE?"RESOURCEUSAGE_CONNECTABLE":resource[i].dwUsage==RESOURCEUSAGE_CONTAINER?"RESOURCEUSAGE_CONTAINER":"?"));
          FXTRACE((1,"lpLocalName=%s\n",resource[i].lpLocalName));
          FXTRACE((1,"lpRemoteName=%s\n",resource[i].lpRemoteName));
          FXTRACE((1,"lpComment=%s\n",resource[i].lpComment));
          FXTRACE((1,"lpProvider=%s\n\n",resource[i].lpProvider));

          // Grow list
          if(count+1>=size){
            size=size?(size<<1):256;
            newlist=new FXString[size];
            for(j=0; j<count; j++) newlist[j]=list[j];
            delete [] filelist;
            filelist=newlist;
            }

          // Add remote name to list
          filelist[count]=resource[i].lpRemoteName;
          count++;
          }
        }
      WNetCloseEnum(hEnum);
      }
    return count;
    }
*/
  // Folding case
  if(flags&LIST_CASEFOLD) matchmode|=FILEMATCH_CASEFOLD;

  // Copy directory name
  pathname=path;
  if(!ISPATHSEP(pathname[pathname.length()-1])) pathname+=PATHSEPSTRING;
  pathname+="*";

  // Open directory
  hFindFile=FindFirstFile(pathname.text(),&ffData);
  if(hFindFile!=INVALID_HANDLE_VALUE){

    // Loop over directory entries
    do{

      // Get name
      name=ffData.cFileName;

      // Filter out files; a bit tricky...
      if(!(ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && ((flags&LIST_NO_FILES) || ((ffData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) && !(flags&LIST_HIDDEN_FILES)) || (!(flags&LIST_ALL_FILES) && !match(pattern,name,matchmode)))) continue;

      // Filter out directories; even more tricky!
      if((ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && ((flags&LIST_NO_DIRS) || ((ffData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) && !(flags&LIST_HIDDEN_DIRS)) || (name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0 && (flags&LIST_NO_PARENT)))) || (!(flags&LIST_ALL_DIRS) && !match(pattern,name,matchmode)))) continue;

      // Grow list
      if(count+1>=size){
        size=size?(size<<1):256;
        newlist=new FXString[size];
        for(int f=0; f<count; f++) newlist[f]=filelist[f];
        delete [] filelist;
        filelist=newlist;
        }

      // Add to list
      filelist[count++]=name;
      }
    while(FindNextFile(hFindFile,&ffData));
    FindClose(hFindFile);
    }
  return count;
  }

#endif


// Convert file time to string as per strftime format
FXString FXFile::time(const FXchar *format,FXTime filetime){
#ifndef WIN32
#ifdef FOX_THREAD_SAFE
  time_t tmp=(time_t)FXMAX(filetime,0);
  struct tm tmresult;
  FXchar buffer[512];
  FXint len=strftime(buffer,sizeof(buffer),format,localtime_r(&tmp,&tmresult));
  return FXString(buffer,len);
#else
  time_t tmp=(time_t)FXMAX(filetime,0);
  FXchar buffer[512];
  FXint len=strftime(buffer,sizeof(buffer),format,localtime(&tmp));
  return FXString(buffer,len);
#endif
#else
  time_t tmp=(time_t)FXMAX(filetime,0);
  FXchar buffer[512];
  FXint len=(FXint) strftime(buffer,sizeof(buffer),format,localtime(&tmp));
  return FXString(buffer,len);
#endif
  }



// Convert file time to string
FXString FXFile::time(FXTime filetime){
  return FXFile::time(TIMEFORMAT,filetime);
  }


// Return current time
FXTime FXFile::now(){
  return (FXTime)::time(NULL);
  }


// Get file info
FXbool FXFile::info(const FXString& file,struct stat& inf){
#ifndef WIN32
  return !file.empty() && (::stat(file.text(),&inf)==0);
#else
  return !file.empty() && (::stat(file.text(),&inf)==0);
#endif
  }


// Get file info
FXbool FXFile::linkinfo(const FXString& file,struct stat& inf){
#ifndef WIN32
  return !file.empty() && (::lstat(file.text(),&inf)==0);
#else
  return !file.empty() && (::stat(file.text(),&inf)==0);
#endif
  }



// Get file size
FXfval FXFile::size(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXfval) status.st_size : 0L;
#else
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? (FXfval) status.st_size : 0L;
#endif
  }


FXbool FXFile::exists(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0);
#else
  return !file.empty() && (GetFileAttributes(file.text())!=0xFFFFFFFF);
#endif
  }



#ifndef WIN32                 // UNIX

// Enquote filename to make safe for shell
FXString FXFile::enquote(const FXString& file,FXbool forcequotes){
  FXString result;
  register FXint i,c;
  for(i=0; (c=file[i])!='\0'; i++){
    switch(c){
      case '\'':              // Quote needs to be escaped
        result+="\\\'";
        break;
      case '\\':              // Backspace needs to be escaped, of course
        result+="\\\\";
        break;
      case '#':
      case '~':
        if(i) goto noquote;   // Only quote if at begin of filename
      case '!':               // Special in csh
      case '"':
      case '$':               // Variable substitution
      case '&':
      case '(':
      case ')':
      case ';':
      case '<':               // Redirections, pipe
      case '>':
      case '|':
      case '`':               // Command substitution
      case '^':               // Special in sh
      case '*':               // Wildcard characters
      case '?':
      case '[':
      case ']':
      case '\t':              // White space
      case '\n':
      case ' ':
        forcequotes=TRUE;
      default:                // Normal characters just added
noquote:result+=c;
        break;
      }
    }
  if(forcequotes) return "'"+result+"'";
  return result;
  }


// Decode filename to get original again
FXString FXFile::dequote(const FXString& file){
  FXString result;
  register FXint i,c;
  i=0;
  while((c=file[i])!='\0' && isspace((FXuchar)c)) i++;
  if(file[i]=='\''){
    i++;
    while((c=file[i])!='\0' && c!='\''){
      if(c=='\\' && file[i+1]!='\0') c=file[++i];
      result+=c;
      i++;
      }
    }
  else{
    while((c=file[i])!='\0' && !isspace((FXuchar)c)){
      if(c=='\\' && file[i+1]!='\0') c=file[++i];
      result+=c;
      i++;
      }
    }
  return result;
  }



#else                         // WINDOWS

// Enquote filename to make safe for shell
FXString FXFile::enquote(const FXString& file,FXbool forcequotes){
  FXString result;
  register FXint i,c;
  for(i=0; (c=file[i])!='\0'; i++){
    switch(c){
      case '<':               // Redirections
      case '>':
      case '|':
      case '$':
      case ':':
      case '*':               // Wildcards
      case '?':
      case ' ':               // White space
        forcequotes=TRUE;
      default:                // Normal characters just added
        result+=c;
        break;
      }
    }
  if(forcequotes) return "\""+result+"\"";
  return result;
  }


// Decode filename to get original again
FXString FXFile::dequote(const FXString& file){
  register FXint i,c;
  FXString result;
  i=0;
  while((c=file[i])!='\0' && isspace((FXuchar)c)) i++;
  if(file[i]=='"'){
    i++;
    while((c=file[i])!='\0' && c!='"'){
      result+=c;
      i++;
      }
    }
  else{
    while((c=file[i])!='\0' && !isspace((FXuchar)c)){
      result+=c;
      i++;
      }
    }
  return result;
  }

#endif


// Match filenames using *, ?, [^a-z], and so on
FXbool FXFile::match(const FXString& pattern,const FXString& file,FXuint flags){
  return fxfilematch(pattern.text(),file.text(),flags);
  }


// Return true if files are identical
FXbool FXFile::identical(const FXString& file1,const FXString& file2){
  if(file1!=file2){
#ifndef WIN32
    struct stat stat1,stat2;
    return !::lstat(file1.text(),&stat1) && !::lstat(file2.text(),&stat2) && stat1.st_ino==stat2.st_ino && stat1.st_dev==stat2.st_dev;
#else
    FXbool same=FALSE;
    HANDLE hFile1;
    HANDLE hFile2;
    hFile1=CreateFile(file1.text(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile1!=INVALID_HANDLE_VALUE){
      hFile2=CreateFile(file2.text(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
      if(hFile2!=INVALID_HANDLE_VALUE){
        BY_HANDLE_FILE_INFORMATION info1;
        BY_HANDLE_FILE_INFORMATION info2;
        if(GetFileInformationByHandle(hFile1,&info1) && GetFileInformationByHandle(hFile2,&info2)){
          same=(info1.nFileIndexLow==info2.nFileIndexLow && info1.nFileIndexHigh==info2.nFileIndexHigh && info1.dwVolumeSerialNumber==info2.dwVolumeSerialNumber);
          }
        CloseHandle(hFile2);
        }
      CloseHandle(hFile1);
      }
    return same;
#endif
    }
  return TRUE;
  }


// Return file mode flags
FXuint FXFile::mode(const FXString& file){
#ifndef WIN32
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? status.st_mode : 0;
#else
  struct stat status;
  return !file.empty() && (::stat(file.text(),&status)==0) ? status.st_mode : 0;
#endif
  }


// Change the mode flags for this file
FXbool FXFile::mode(const FXString& file,FXuint mode){
#ifndef WIN32
  return !file.empty() && chmod(file.text(),mode)==0;
#else
  return FALSE; // Unimplemented yet
#endif
  }


// Create new directory
FXbool FXFile::createDirectory(const FXString& path,FXuint mode){
#ifndef WIN32
  return mkdir(path.text(),mode)==0;
#else
  return CreateDirectory(path.text(),NULL)!=0;
#endif
  }


// Create new (empty) file
FXbool FXFile::createFile(const FXString& file,FXuint mode){
#ifndef WIN32
	FXint fd=::open(file.text(),O_CREAT|O_WRONLY|O_TRUNC|O_EXCL,mode);
  if(fd>=0){ ::close(fd); return TRUE; }
  return FALSE;
#else
  HANDLE hFile=CreateFile(file.text(),GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile!=INVALID_HANDLE_VALUE){ CloseHandle(hFile); return TRUE; }
  return FALSE;
#endif
  }


// Hack code below for testing if volume is mounted

// #if defined (HKS_NT)
//
// static int check_nfs (const char* name)
// {
// char drive[8];
//
// char* cp = strchr (name, ':');
// if (cp)
// {
// strncpy (drive, name, cp - name);
// drive[cp - name] = '\0';
// }
// else
// {
// drive[0] = 'A' + _getdrive() - 1;
// drive[1] = '\0';
// }
//
// strcat (drive, ":\\");
//
// return GetDriveType(drive) == DRIVE_REMOTE;
// }
//
// #elif defined(LINUX)
//
// static int check_nfs (int fd)
// {
// struct statfs statbuf;
// if (fstatfs(fd,&statbuf) < 0)
// {
// RFM_RAISE_SYSTEM_ERROR("statfs");
// return 0;
// }
// if (statbuf.f_type == NFS_SUPER_MAGIC)
// return 1;
// else
// return 0;
// }
//
// #else
//
// static int check_nfs (int fd)
// {
//
// struct statvfs statbuf;
//
// if (fstatvfs (fd, &statbuf) < 0)
// {
// RFM_RAISE_SYSTEM_ERROR ("fstatvfs");
// }
// return strncmp (statbuf.f_basetype, "nfs", 3) == 0 || strncmp
// (statbuf.f_basetype, "NFS", 3) == 0;
// }
// #endif






#ifndef WIN32


// Read bytes
static long fullread(int fd,unsigned char *ptr,long len){
  long nread;
#ifdef EINTR
  do{nread=read(fd,ptr,len);}while(nread<0 && errno==EINTR);
#else
  nread=read(fd,ptr,len);
#endif
  return nread;
  }


// Write bytes
static long fullwrite(int fd,const unsigned char *ptr,long len){
  long nwritten,ntotalwritten=0;
  while(len>0){
    nwritten=write(fd,ptr,len);
    if(nwritten<0){
#ifdef EINTR
      if(errno==EINTR) continue;
#endif
      return -1;
      }
    ntotalwritten+=nwritten;
    ptr+=nwritten;
    len-=nwritten;
    }
  return ntotalwritten;
  }


// Concatenate srcfile1 and srcfile2 to a dstfile
FXbool FXFile::concatenate(const FXString& srcfile1,const FXString& srcfile2,const FXString& dstfile,FXbool overwrite){
  unsigned char buffer[4096];
  struct stat status;
  int src1,src2,dst;
  long nread,nwritten;
  FXbool ok=FALSE;
  if(srcfile1==dstfile || srcfile2==dstfile) return FALSE;
  if(::lstat(dstfile.text(),&status)==0){
    if(!overwrite) return FALSE;
    }
  dst=::open(dstfile.text(),O_CREAT|O_WRONLY|O_TRUNC,0777);
  if(0<=dst){
    src1=::open(srcfile1.text(),O_RDONLY);
    if(0<=src1){
      src2=::open(srcfile2.text(),O_RDONLY);
      if(0<=src2){
        while(1){
          nread=fullread(src1,buffer,sizeof(buffer));
          if(nread<0) goto err;
          if(nread==0) break;
          nwritten=fullwrite(dst,buffer,nread);
          if(nwritten<0) goto err;
          }
        while(1){
          nread=fullread(src2,buffer,sizeof(buffer));
          if(nread<0) goto err;
          if(nread==0) break;
          nwritten=fullwrite(dst,buffer,nread);
          if(nwritten<0) goto err;
          }
        ok=TRUE;
err:    ::close(src2);
        }
      ::close(src1);
      }
    ::close(dst);
    }
  return ok;
  }


// Copy ordinary file
static FXbool copyfile(const FXString& oldfile,const FXString& newfile){
  unsigned char buffer[4096];
  struct stat status;
  long nread,nwritten;
  int src,dst;
  FXbool ok=FALSE;
  if((src=open(oldfile.text(),O_RDONLY))>=0){
    if(::stat(oldfile.text(),&status)==0){
      if((dst=::open(newfile.text(),O_WRONLY|O_CREAT|O_TRUNC,status.st_mode))>=0){
        while(1){
          nread=fullread(src,buffer,sizeof(buffer));
          if(nread<0) goto err;
          if(nread==0) break;
          nwritten=fullwrite(dst,buffer,nread);
          if(nwritten<0) goto err;
          }
        ok=TRUE;
err:    ::close(dst);
        }
      }
    ::close(src);
    }
  return ok;
  }


// To search visited inodes
struct inodelist {
  ino_t st_ino;
  inodelist *next;
  };


// Forward declararion
static FXbool copyrec(const FXString& oldfile,const FXString& newfile,FXbool overwrite,inodelist* inodes);


// Copy directory
static FXbool copydir(const FXString& oldfile,const FXString& newfile,FXbool overwrite,struct stat& parentstatus,inodelist* inodes){
  FXString oldchild,newchild;
  struct stat status;
  inodelist *in,inode;
  struct dirent *dp;
  DIR *dirp;

  // See if visited this inode already
  for(in=inodes; in; in=in->next){
    if(in->st_ino==parentstatus.st_ino) return TRUE;
    }

  // Try make directory, if none exists yet
  if(mkdir(newfile.text(),parentstatus.st_mode|S_IWUSR)!=0 && errno!=EEXIST) return FALSE;

  // Can we stat it
  if(::lstat(newfile.text(),&status)!=0 || !S_ISDIR(status.st_mode)) return FALSE;

  // Try open directory to copy
  dirp=opendir(oldfile.text());
  if(!dirp) return FALSE;

  // Add this to the list
  inode.st_ino=status.st_ino;
  inode.next=inodes;

  // Copy stuff
#ifdef FOX_THREAD_SAFE
  struct fxdirent dirresult;
  while(!readdir_r(dirp,&dirresult,&dp) && dp){
#else
  while((dp=readdir(dirp))!=NULL){
#endif
    if(dp->d_name[0]!='.' || (dp->d_name[1]!='\0' && (dp->d_name[1]!='.' || dp->d_name[2]!='\0'))){
      oldchild=oldfile;
      if(!ISPATHSEP(oldchild[oldchild.length()-1])) oldchild.append(PATHSEP);
      oldchild.append(dp->d_name);
      newchild=newfile;
      if(!ISPATHSEP(newchild[newchild.length()-1])) newchild.append(PATHSEP);
      newchild.append(dp->d_name);
      if(!copyrec(oldchild,newchild,overwrite,&inode)){
        closedir(dirp);
        return FALSE;
        }
      }
    }

  // Close directory
  closedir(dirp);

  // Success
  return TRUE;
  }




// Recursive copy
static FXbool copyrec(const FXString& oldfile,const FXString& newfile,FXbool overwrite,inodelist* inodes){
  struct stat status1,status2;

  // Old file or directory does not exist
  if(::lstat(oldfile.text(),&status1)!=0) return FALSE;

  // If target is not a directory, remove it if allowed
  if(::lstat(newfile.text(),&status2)==0){
    if(!S_ISDIR(status2.st_mode)){
      if(!overwrite) return FALSE;
      FXTRACE((100,"unlink(%s)\n",newfile.text()));
      if(::unlink(newfile.text())!=0) return FALSE;
      }
    }

  // Source is direcotory: copy recursively
  if(S_ISDIR(status1.st_mode)){
    return copydir(oldfile,newfile,overwrite,status1,inodes);
    }

  // Source is regular file: copy block by block
  if(S_ISREG(status1.st_mode)){
    FXTRACE((100,"copyfile(%s,%s)\n",oldfile.text(),newfile.text()));
    return copyfile(oldfile,newfile);
    }

  // Source is fifo: make a new one
  if(S_ISFIFO(status1.st_mode)){
    FXTRACE((100,"mkfifo(%s)\n",newfile.text()));
    return ::mkfifo(newfile.text(),status1.st_mode);
    }

  // Source is device: make a new one
  if(S_ISBLK(status1.st_mode) || S_ISCHR(status1.st_mode) || S_ISSOCK(status1.st_mode)){
    FXTRACE((100,"mknod(%s)\n",newfile.text()));
    return ::mknod(newfile.text(),status1.st_mode,status1.st_rdev)==0;
    }

  // Source is symbolic link: make a new one
  if(S_ISLNK(status1.st_mode)){
    FXString lnkfile=FXFile::symlink(oldfile);
    FXTRACE((100,"symlink(%s,%s)\n",lnkfile.text(),newfile.text()));
    return ::symlink(lnkfile.text(),newfile.text())==0;
    }

  // This shouldn't happen
  return FALSE;
  }


#else


// Concatenate srcfile1 and srcfile2 to a dstfile
FXbool FXFile::concatenate(const FXString& srcfile1,const FXString& srcfile2,const FXString& dstfile,FXbool overwrite){
  unsigned char buffer[4096];
  HANDLE src1,src2,dst;
  DWORD nread,nwritten;
  FXbool ok=FALSE;
  if(srcfile1==dstfile || srcfile2==dstfile) return FALSE;
  if(GetFileAttributes(dstfile.text())!=0xFFFFFFFF){
    if(!overwrite) return FALSE;
    }
  dst=CreateFile(dstfile.text(),GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  if(dst!=INVALID_HANDLE_VALUE){
    src1=CreateFile(srcfile1.text(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(src1!=INVALID_HANDLE_VALUE){
      src2=CreateFile(srcfile2.text(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
      if(src2!=INVALID_HANDLE_VALUE){
        while(1){
          if(!ReadFile(src1,buffer,sizeof(buffer),&nread,NULL)) goto err;
          if(nread==0) break;
          if(!WriteFile(dst,buffer,nread,&nwritten,NULL)) goto err;
          }
        while(1){
          if(!ReadFile(src2,buffer,sizeof(buffer),&nread,NULL)) goto err;
          if(nread==0) break;
          if(!WriteFile(dst,buffer,nread,&nwritten,NULL)) goto err;
          }
        ok=TRUE;
err:    CloseHandle(src2);
        }
      CloseHandle(src1);
      }
    CloseHandle(dst);
    }
  return ok;
  }


// Forward declararion
static FXbool copyrec(const FXString& oldfile,const FXString& newfile,FXbool overwrite);


// Copy directory
static FXbool copydir(const FXString& oldfile,const FXString& newfile,FXbool overwrite){
  FXString oldchild,newchild;
  DWORD atts;
  WIN32_FIND_DATA ffData;
  HANDLE hFindFile;

  // Try make directory, if none exists yet
//  if(CreateDirectory(newfile.text(),NULL)==0 && GetLastError()!=ERROR_FILE_EXISTS) return FALSE;
  if(CreateDirectory(newfile.text(),NULL)==0){  // patch from "Malcolm Dane" <danem@talk21.com>
    switch(GetLastError()){
      case ERROR_FILE_EXISTS:
      case ERROR_ALREADY_EXISTS: break;
      default: return FALSE;
      }
    }

  // Can we stat it
  if((atts=GetFileAttributes(newfile.text()))==0xffffffff || !(atts&FILE_ATTRIBUTE_DIRECTORY)) return FALSE;

  // Try open directory to copy
  hFindFile=FindFirstFile((oldfile+PATHSEPSTRING+"*").text(),&ffData);
  if(hFindFile==INVALID_HANDLE_VALUE) return FALSE;

  // Copy stuff
  do{
    if(ffData.cFileName[0]!='.' && (ffData.cFileName[1]!='\0' && (ffData.cFileName[1]!='.' || ffData.cFileName[2]!='\0'))){
      oldchild=oldfile;
      if(!ISPATHSEP(oldchild[oldchild.length()-1])) oldchild.append(PATHSEP);
      oldchild.append(ffData.cFileName);
      newchild=newfile;
      if(!ISPATHSEP(newchild[newchild.length()-1])) newchild.append(PATHSEP);
      newchild.append(ffData.cFileName);
      if(!copyrec(oldchild,newchild,overwrite)){
        FindClose(hFindFile);
        return FALSE;
        }
      }
    }
  while(FindNextFile(hFindFile,&ffData));

  // Close directory
  FindClose(hFindFile);

  // Success
  return TRUE;
  }


// Recursive copy
static FXbool copyrec(const FXString& oldfile,const FXString& newfile,FXbool overwrite){
  DWORD atts1,atts2;

  // Old file or directory does not exist
  if((atts1=GetFileAttributes(oldfile.text()))==0xffffffff) return FALSE;

  // If target is not a directory, remove it if allowed
  if((atts2=GetFileAttributes(newfile.text()))!=0xffffffff){
    if(!(atts2&FILE_ATTRIBUTE_DIRECTORY)){
      if(!overwrite) return FALSE;
      FXTRACE((100,"DeleteFile(%s)\n",newfile.text()));
      if(DeleteFile(newfile.text())==0) return FALSE;
      }
    }

  // Source is direcotory: copy recursively
  if(atts1&FILE_ATTRIBUTE_DIRECTORY){
    return copydir(oldfile,newfile,overwrite);
    }

  // Source is regular file: copy block by block
  if(!(atts1&FILE_ATTRIBUTE_DIRECTORY)){
    FXTRACE((100,"CopyFile(%s,%s)\n",oldfile.text(),newfile.text()));
    return CopyFile(oldfile.text(),newfile.text(),!overwrite);
    }

  // This shouldn't happen
  return FALSE;
  }


#endif


// Copy file
FXbool FXFile::copy(const FXString& oldfile,const FXString& newfile,FXbool overwrite){
  if(newfile!=oldfile){
#ifndef WIN32
    return copyrec(oldfile,newfile,overwrite,NULL);
#else
    return copyrec(oldfile,newfile,overwrite);      // No symlinks, so no need to check if directories are visited already
#endif
    }
  return FALSE;
  }


// Remove file or directory
FXbool FXFile::remove(const FXString& file){
#ifndef WIN32
  struct stat status;
  if(::lstat(file.text(),&status)==0){
    if(S_ISDIR(status.st_mode)){
      DIR *dirp=::opendir(file.text());
      if(dirp){
        FXString child;
        struct dirent *dp;
#ifdef FOX_THREAD_SAFE
        struct fxdirent dirresult;
        while(!readdir_r(dirp,&dirresult,&dp) && dp){
#else
        while((dp=readdir(dirp))!=NULL){
#endif
          if(dp->d_name[0]!='.' || (dp->d_name[1]!='\0' && (dp->d_name[1]!='.' || dp->d_name[2]!='\0'))){
            child=file;
            if(!ISPATHSEP(child[child.length()-1])) child.append(PATHSEP);
            child.append(dp->d_name);
            if(!FXFile::remove(child)){
              ::closedir(dirp);
              return FALSE;
              }
            }
          }
        ::closedir(dirp);
        }
      FXTRACE((100,"rmdir(%s)\n",file.text()));
      return ::rmdir(file.text())==0;
      }
    else{
      FXTRACE((100,"unlink(%s)\n",file.text()));
      return ::unlink(file.text())==0;
      }
    }
  return FALSE;
#else
  DWORD atts;
  if((atts=GetFileAttributes(file.text()))!=0xffffffff){
    if(atts&FILE_ATTRIBUTE_DIRECTORY){
      WIN32_FIND_DATA ffData;
      HANDLE hFindFile;
      hFindFile=FindFirstFile((file+PATHSEPSTRING+"*").text(),&ffData); // FIXME we may want to formalize the "walk over directory" in a few API's here also...
      if(hFindFile!=INVALID_HANDLE_VALUE){
        FXString child;
        do{
          if(ffData.cFileName[0]!='.' && (ffData.cFileName[1]!='\0' && (ffData.cFileName[1]!='.' || ffData.cFileName[2]!='\0'))){
            child=file;
            if(!ISPATHSEP(child[child.length()-1])) child.append(PATHSEP);
            child.append(ffData.cFileName);
            if(!FXFile::remove(child)){
              FindClose(hFindFile);
              return FALSE;
              }
            }
          }
        while(FindNextFile(hFindFile,&ffData));
        FindClose(hFindFile);
        }
      FXTRACE((100,"RemoveDirectory(%s)\n",file.text()));
      return RemoveDirectory(file.text())!=0;
      }
    else{
      FXTRACE((100,"DeleteFile(%s)\n",file.text()));
      return DeleteFile(file.text())!=0;
      }
    }
  return FALSE;
#endif
  }


// Rename or move file, or copy and delete old if different file systems
FXbool FXFile::move(const FXString& oldfile,const FXString& newfile,FXbool overwrite){
  if(newfile!=oldfile){
#ifndef WIN32
    if(!FXFile::exists(oldfile)) return FALSE;
    if(FXFile::exists(newfile)){
      if(!overwrite) return FALSE;
      if(!FXFile::remove(newfile)) return FALSE;
      }
    FXTRACE((100,"rename(%s,%s)\n",oldfile.text(),newfile.text()));
    if(::rename(oldfile.text(),newfile.text())==0) return TRUE;
    if(errno!=EXDEV) return FALSE;
    if(FXFile::copy(oldfile,newfile)){
      return FXFile::remove(oldfile);
      }
#else
    if(!FXFile::exists(oldfile)) return FALSE;
    if(FXFile::exists(newfile)){
      if(!overwrite) return FALSE;
      if(!FXFile::remove(newfile)) return FALSE;
      }
    FXTRACE((100,"MoveFile(%s,%s)\n",oldfile.text(),newfile.text()));
    if(::MoveFile(oldfile.text(),newfile.text())!=0) return TRUE;
    if(GetLastError()!=ERROR_NOT_SAME_DEVICE) return FALSE;
    if(FXFile::copy(oldfile,newfile)){
      return FXFile::remove(oldfile);
      }
#endif
    }
  return FALSE;
  }


// Link file
FXbool FXFile::link(const FXString& oldfile,const FXString& newfile,FXbool overwrite){
  if(newfile!=oldfile){
#ifndef WIN32
    if(!FXFile::exists(oldfile)) return FALSE;
    if(FXFile::exists(newfile)){
      if(!overwrite) return FALSE;
      if(!FXFile::remove(newfile)) return FALSE;
      }
    FXTRACE((100,"link(%s,%s)\n",oldfile.text(),newfile.text()));
    return ::link(oldfile.text(),newfile.text())==0;
#else
    typedef BOOL (WINAPI *PFN_CHL)(LPCTSTR,LPCTSTR,LPSECURITY_ATTRIBUTES);
    static PFN_CHL chl=NULL;
    if(!chl){
      HMODULE hkernel=LoadLibraryA("Kernel32");
      if(!hkernel) return FALSE;
      chl=(PFN_CHL)::GetProcAddress(hkernel,"CreateHardLinkA");
      FreeLibrary(hkernel);
      }
    if(!FXFile::exists(oldfile)) return FALSE;
    if(FXFile::exists(newfile)){
      if(!overwrite) return FALSE;
      if(!FXFile::remove(newfile)) return FALSE;
      }
    FXTRACE((100,"CreateHardLink(%s,%s)\n",oldfile.text(),newfile.text()));
    return chl && (*chl)(newfile.text(),oldfile.text(),NULL)!=0;
#endif
    }
  return FALSE;
  }


// Symbolic Link file
FXbool FXFile::symlink(const FXString& oldfile,const FXString& newfile,FXbool overwrite){
#ifndef WIN32
  if(newfile!=oldfile){
    if(!FXFile::exists(oldfile)) return FALSE;
    if(FXFile::exists(newfile)){
      if(!overwrite) return FALSE;
      if(!FXFile::remove(newfile)) return FALSE;
      }
    FXTRACE((100,"symlink(%s,%s)\n",oldfile.text(),newfile.text()));
    return ::symlink(oldfile.text(),newfile.text())==0;
    }
#endif
  return FALSE;
  }


// Read symbolic link
FXString FXFile::symlink(const FXString& file){
#ifndef WIN32
  FXchar lnk[MAXPATHLEN+1];
  FXint len=::readlink(file.text(),lnk,MAXPATHLEN);
  if(0<=len) return FXString(lnk,len);
#endif
  return FXString::null;
  }

}

