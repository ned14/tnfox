/********************************************************************************
*                                                                               *
*                           An Access Control List                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
*       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
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

#ifndef FXACL_H
#define FXACL_H

#include "FXString.h"

namespace FX {

/*! \file FXACL.h
\brief Defines classes used in providing Access Control Lists
*/

/*! \class FXACLEntity
\ingroup security
\brief Something which can be permitted or denied to do something within
this operating system

As under most operating systems this is no more than a container for
a very long integer, most of these operations are fast. The exceptions
is lookupUser() as well as the asString() method which
may need to query a remote machine.

Someday the functionality to obtain a list of all entities on a local
or remote machine may be added.

\warning I've made this mistake myself, so I'll warn of it here - when
browsing FXACL's, you \b must check each to see if it is owner() and if
it is then \b you must indirect to the item's owner. Failing to do this
causes different (usually breaking) behaviour on Windows and POSIX, but
also is subtly broken on Windows where owner() appears if it's in the ACL
instead of the owner id itself (this is rare post-NT4 as inheritability
causes the dereferencing long before you usually see it).

\note User accounts on NT can belong to many groups and there is no primary
one. However, NTFS does maintain a primary group as well as user and so
where possible, FXACLEntity's static methods try to find a suitable primary
group. Where this isn't possible, CREATOR_GROUP is used.

\sa FX::FXACL
*/
struct FXACLEntityPrivate;
class FXAPIR FXACLEntity
{
	friend class FXACL;
	friend struct FXACLPrivate;
	FXACLEntityPrivate *p;
public:
	FXACLEntity();
	FXACLEntity(const FXACLEntity &o);
	FXACLEntity &operator=(const FXACLEntity &o);
	~FXACLEntity();
	bool operator==(const FXACLEntity &o) const;
	bool operator!=(const FXACLEntity &o) const { return !(*this==o); }
	//! Returns true if this entity is a group
	bool isGroup() const;
	//! Returns the primary group this entity belongs to
	FXACLEntity group() const;
	/*! Returns the entity as a string, localised to the system locale.
	If \em withId is true, appends the system-dependent underlying user id
	in curly brackets. If \em withMachine is true, on supported platforms this
	prepends the machine where the account lives - this is separated from
	the username with a / rather than a \\ */
	FXString asString(bool withId=true, bool withMachine=true) const;
	/*! Returns true if the specified password is the password to the entity's account.
	You should allocate the password in the secure heap. Due to implementation constraints
	on Windows, SSPI is used which requires the local machine to permit network logons.
	On POSIX, PAM is asked to perform the authentication - this requires root privileges
	to succeed (an exception is thrown if the calling process does not have them)
	\note When successful, this entity becomes authenticated and can perform extra roles
	eg; on NT you can set it as owner of anything
	*/
	bool isLoginPassword(const FXchar *password) const;
	/*! Returns the home directory of the entity if it has one, if not a blank string
	is returned rather than an error. If \em filesdir is true, returns the path where
	the entity stores its files (this is the same as the home directory on POSIX, but
	on Windows it is "My Documents" in the system locale). Note that the entity must
	be authenticated before you can retrieve its home directory */
	FXString homeDirectory(bool filesdir=false) const;
	/*! Returns an entity representing the current user (ie; the user who created this process)
	This entity is already authenticated, so you don't need to call isLoginPassword() */
	static const FXACLEntity &currentUser();
	//! Returns the special entity representing everything (ie; the public)
	static const FXACLEntity &everything();
	//! Returns the special entity representing the highest privileged user (eg; KATE\\Administrator)
	static const FXACLEntity &root();
	//! Returns the special entity representing the owner of something
	static const FXACLEntity &owner();
	/*! Returns an entity representing the specified user on the specified machine. Note that
	on Windows, this may be a user on another machine */
	static FXACLEntity lookupUser(const FXString &username, const FXString &machine=FXString::nullStr());
	/*! Returns a list of entities available on the local machine */
	//static QValueList<FXACLEntity> localEntities();
};

class FXACLIterator;
class FXACLPrivate;

/*! \class FXACL
\ingroup security
\brief Provides a portable Access Control List

Microsoft Windows NT provides an excellent security system based upon
Access Control Lists as indeed does SE-Linux, the C3 secure form of
Linux. While ACL based security is not inherently secure (ie;
mathematically proveable as secure) it's still not bad at all.
This class permits portable control of system ACL's and on
systems without them (POSIX), it provides a reasonable emulation.

Unfortunately, Win32 code rarely bothers with NT security and so if
you do try to use it extensively, quite a lot of other code just falls over.
This is almost certainly resulting from the amazingly obtuse, overly
low-level Win32 security API which because it's so hard to understand
really is badly designed. There should at least have been an extra
higher-level API for it but I guess security isn't so important at
Microsoft :(

The major difficulty limiting the usefulness of this class is that
underlying most ACL based systems is a very long integer which
uniquely identifies an entity (FX::FXACLEntity). The big problem is
manipulating portably this long integer - what FXACLEntity provides is
about as much as is common across all platforms.

Where possible, for construction and writing use the methods in all
FX::QIODevice derived classes - these are called permissions() and
setPermissions(). Usually a static method is provided as well to aid
accessing entities not belonging to the local process.

Note that denies are processed before grants, though in the order
of the entries within the ACL.

<h3>Usage:</h3>
For POSIX systems, they provide a minimal user, group & public (other)
read, write & execute bits per thing plus the concept of an owner.
This is represented by FXACL as those three things in that particular
order using the special entities FX::FXACLEntity::owner() and
FX::FXACLEntity::everything() if they have something enabled -
there are never any deny entries.
FXACL will complain if you try setting permissions on anything
other than the ACL's owner or group. You can set permissions for public
(FX::FXACLEntity::everything()) on all systems.

The POSIX permissions map onto FX::FXACL::Permissions as follows for
directories:
\li Read is list, readattr, readperms
\li Write is createfiles, createdirs, deletefiles, deletedirs, writeattr, writeperms
\li Execute is traverse

The POSIX permissions map onto FX::FXACL::Permissions as follows for
files:
\li Read is read, readattr, readperms
\li Write is write, append, writeattr, writeperms
\li Execute is execute

takeownership is always set on POSIX systems if the user is \c root.
If not, it's always unset.

On NT, the situation is quite a lot more complex. Different entities
have different mapping of permission bits - for files, the permissions
just happen to map nicely onto NT ones except
that readattr and writeattr also specify NT extended attributes as well
as normal ones. I think the nice mapping happened because I thought
of every possible operation you'd like to prevent and obviously so did
the designers of NT (ie; Dave Cutler). One caveat on NT is that file
and directory permissions share the same space, so if \em list is true then
you get read file access. There is a slight difference though as read-via-list
does not apply SYNCHRONIZE permission which will cause a problem if you
ever try to wait on a file handle. Best use file permissions for files
and directory permissions for directories.

For pipes, read and write do the obvious. append or createdirs permit
another instance to be created. execute
applies the SYNCHRONIZE permission so it's possible to create a synchronise-only
named pipe (useful).

For memory mapped sections, read and write are again obvious. append
permits extending of the mapped section. deletefile enables deletion
of the mapped area. copyonwrite is specially for mapped sections and
enables copy-on-write access.

For all entities on NT, readattrs, writeattrs, readperms, writeperms and
takeownership are available.

When specifying permission bits, you can either use the setX() methods
in the FX::FXACL::Permissions structure (these can be chained together)
or the more C-style macros prefixed with \c FXACL_ - which you choose
depends on the moment. In particular you may find the FXACL_GENREAD,
FXACL_GENWRITE and FXACL_GENEXECUTE macros of interest.

On NT particularly, watch out for forgetting to set the execute (traverse)
permission for directories - NT ignores this permission when the process'
owner is the same as the directory's but doesn't if they're different.

On NT, the system does not permit you to set ownership of something to
other than the current user. You can work around this by authenticating
the desired owner entity using FXACLEntity::isLoginPassword() first and
then setting it as owner of the ACL before applying.

<h3>Writing portable code:</h3>
If you're writing FXACL using code which must run on POSIX and NT, firstly
consider using hostOSACLSupport() to use better facilities when available.
Future versions of TnFOX will almost certainly support POSIX capabilities
or other enhanced security facilities and your code can become magically
enabled via a simple recompile with a newer version.

Past that, as NT was originally written to be POSIX compliant, through
only using FXACLEntity::owner() and FXACLEntity::everything() in entries
and only using FX::FXACL::Permissions::setGenRead(), FX::FXACL::Permissions::setGenWrite()
and FX::FXACL::Permissions::setGenExecute() member functions (or their
equivalent macros) you can have one code which works with identical
semantics everywhere. Note however that NT is not POSIX Unix and you can
introduce subtle security holes if you're not careful - for example, the
inheritability of ACL's in NT can cause any permissions you set to be
totally ignored.
*/
class FXAPIR FXACL
{
	friend class FXACLIterator;
	FXACLPrivate *p;
public:
	//! Types of thing for which an ACL can enumerate security
	enum EntityType
	{
		Unknown=0,
		File,			//!< This ACL describes a file (FX::FXFile)
		Directory,		//!< This ACL describes a directory
		Pipe,			//!< This ACL describes a pipe (FX::QPipe)
		MemMap			//!< This ACL describes memory mapped data (FX::QMemMap)
	};
	//! Permissions as an integer
	typedef FXuint Perms;
	//! Contains the permissions of an entity
	struct FXAPI Permissions
	{
		Perms read:1;				//!< Reading of data
		Perms write:1;				//!< Writing of data
		Perms execute:1;			//!< Executing of data
		Perms append:1;				//!< Appending of data
		Perms copyonwrite:1;		//!< Copy on write of data (mapped sections only)
		Perms reserved2:3;

		Perms list:1;				//!< Listing of directories
		Perms createfiles:1;		//!< Creation of files within directories
		Perms createdirs:1;			//!< Creation of directories within directories
		Perms traverse:1;			//!< Traversal of directories
		Perms deletefiles:1;		//!< Deletion of files within directories
		Perms deletedirs:1;			//!< Deletion of directories within directories
		Perms reserved1:2;

		Perms readattrs:1;			//!< Reading of attributes
		Perms writeattrs:1;			//!< Writing of attributes
		Perms readperms:1;			//!< Reading of permissions
		Perms writeperms:1;			//!< Writing of permissions
		Perms takeownership:1;		//!< Changing of ownership
		Perms reserved3:3;

		Perms amTn:1;				//!< Reserved for use by Tn
		Perms custom:7;				//!< Custom bits (used by Tn)

		Permissions(Perms v=0) { *((Perms *) this)=v; }
		//bool operator==(const Permissions &o) const { return *((Perms *) this)==*((Perms *) &o); }
		//bool operator!=(const Permissions &o) const { return *((Perms *) this)!=*((Perms *) &o); }
		operator Perms() const { return *((Perms *) this); }
		Perms asUInt() const { return *((Perms *) this); }
		Permissions &operator=(Perms v) { *((Perms *) this)=v; return *this; }
		Permissions &setRead   (bool v=true) { read=v;    return *this; }
		Permissions &setWrite  (bool v=true) { write=v;   return *this; }
		Permissions &setExecute(bool v=true) { execute=v; return *this; }
		Permissions &setAppend (bool v=true) { append=v;  return *this; }
		Permissions &setCopyOnWrite(bool v=true) { copyonwrite=v;  return *this; }
		Permissions &setList       (bool v=true) { list=v;        return *this; }
		Permissions &setCreateFiles(bool v=true) { createfiles=v; return *this; }
		Permissions &setCreateDirs (bool v=true) { createdirs=v;  return *this; }
		Permissions &setTraverse   (bool v=true) { traverse=v;    return *this; }
		Permissions &setDeleteFiles(bool v=true) { deletefiles=v; return *this; }
		Permissions &setDeleteDirs (bool v=true) { deletedirs=v;  return *this; }
		Permissions &setReadAttrs (bool v=true) { readattrs=v;   return *this; }
		Permissions &setWriteAttrs(bool v=true) { writeattrs=v;  return *this; }
		Permissions &setReadPerms (bool v=true) { readperms=v;  return *this; }
		Permissions &setWritePerms(bool v=true) { writeperms=v; return *this; }
		Permissions &setTakeOwnership(bool v=true) { takeownership=v;  return *this; }
		//! Sets generic read
		Permissions &setGenRead(bool v=true)
		{
			read=list=readattrs=readperms=v;
			return *this;
		}
		//! Sets generic write
		Permissions &setGenWrite(bool v=true)
		{
			write=append=createfiles=createdirs=deletefiles=deletedirs=writeattrs=writeperms=v;
			return *this;
		}
		//! Sets generic execute
		Permissions &setGenExecute(bool v=true)
		{
			execute=traverse=v;
			return *this;
		}
		//! Sets all possible, including copy on write and take ownership
		Permissions &setAll(bool plusExecute=false)
		{
			setGenRead().setGenWrite(); if(plusExecute) setGenExecute();
			setCopyOnWrite().setTakeOwnership();
			return *this;
		}
		/*! Returns a string of the form "RWXA RWRWO" or "LTCDCD RWRWO" for
		files and directories respectively. The letter is in lower case if not present */
		FXString asString(EntityType type) const;
	};
	//! An entry within the ACL
	struct Entry
	{
		bool inherited;				//! True when the entry came by inheritance
		bool inheritable;			//! Set to true to cause children to inherit this entry
		FXACLEntity entity;			//! The entity being denied or granted access
		Permissions deny, grant;	//! The deny and grant permissions
		Entry(const FXACLEntity &_entity, Perms _deny, Perms _grant, bool _inheritable=false) : inherited(false), inheritable(_inheritable), entity(_entity), deny(_deny), grant(_grant) { }
		bool operator==(const Entry &o) const { return entity==o.entity && deny==o.deny && grant==o.grant; } 
	};
private:
	FXDLLLOCAL void init(void *, EntityType);
public:
	//! Constructs an empty ACL of type \em type owned by \em owner
	FXACL(EntityType type=Unknown, const FXACLEntity &owner=FXACLEntity::currentUser());
	/*! Construct an ACL based on an item in the filing system. You
	should tend to use the permissions() method in the relevent classes
	rather than this directly (as NT and POSIX have different conventions) */
	FXACL(const FXString &path, EntityType type);
	//! Constructs an ACL based on a NT kernel handle. Avoid using directly
	FXACL(void *h, EntityType type);
	//! Constructs an ACL based on a POSIX file handle. Avoid using directly
	FXACL(int fd, EntityType type);
	FXACL(const FXACL &o);
	FXACL &operator=(const FXACL &o);
	~FXACL();
	bool operator==(const FXACL &o) const;
	bool operator!=(const FXACL &o) const;
	//! Returns the type of thing described by this ACL
	EntityType type() const;
	//! Sets the type of thing described by this ACL
	void setType(EntityType type);
	//! Returns if this ACL inherits from a parent ACL
	bool hasInherited() const;
	//! Sets if this ACL inherits from a parent ACL
	void setHasInherited(bool newval);
	//! Returns the number of entities listed within this ACL
	FXuint count() const;
	//! Inserts an entry into this ACL
	void insert(const FXACLIterator &it, const Entry &entry);
	//! Removes an entry from this ACL
	void remove(const FXACLIterator &it);
	//! Returns an iterator pointing to the start of the ACL
	FXACLIterator begin() const;
	//! Returns an iterator pointing to the end of the ACL
	FXACLIterator end() const;
	//! Prepends an entry to this ACL
	void prepend(const Entry &entry);
	//! Appends an entry to this ACL
	void append(const Entry &entry);
	//! Returns the entity owning the thing this ACL describes
	const FXACLEntity &owner() const;
	//! Sets the entity owning the thing this ACL describes
	void setOwner(const FXACLEntity &entity);
	//! Checks the ACL if \em what is permitted by this process
	bool check(Perms what) const;
	//! Checks the ACL if \em what is permitted by this process, throwing a FX::FXNoPermissionException if not
	void checkE(Perms what) const;
	//! Returns the ACL as a string
	FXString report() const;
	//! \overload
	FXString asString() const { return report(); }

	//! Writes the ACL to the specified path in the filing system
	void writeTo(const FXString &path) const;
	//! Writes the ACL to the specified NT kernel handle. Avoid using directly.
	void writeTo(void *h) const;
	//! Writes the ACL to the specified POSIX file handle. Avoid using directly.
	void writeTo(int fd) const;

	/*! Returns a default ACL with the owner set to the current user and
	one entry specifying that only the current user has full access except
	for execute. For \em flags bit 0 specifies that the user's group has
	read access, bit 1 specifies write access and bit 2 has the execute
	bits set too */
	static FXACL default_(EntityType type, int flags=0);
	//! Denotes how much security support the host OS provides
	struct ACLSupport
	{
		FXuint perOwnerGroup	: 1; //! True if per owner/owner group permissions can be set
		FXuint perEntity		: 1; //! True if per arbitrary entity permissions can be set
		FXuint hasInheritance	: 1; //! True if ACL's can inherit entries
		ACLSupport() { *((FXuint *) this)=0; }
		FXuint asUInt() const { return *((FXuint *) this); }
	};
	//! Returns what ACL features the host OS supports
	static ACLSupport hostOSACLSupport();
	/*! Resets a tree on the filing system to have specific permissions, making
	use of inheritance when available */
	static void resetPath(const FXString &path, const FXACL &dirs, const FXACL &files);
public:
	FXDLLLOCAL void *int_toWin32SecurityDescriptor() const;
};

#define FXACL_READ (1<<0)
#define FXACL_WRITE (1<<1)
#define FXACL_EXECUTE (1<<2)
#define FXACL_APPEND (1<<3)
#define FXACL_COPYONWRITE (1<<4)
#define FXACL_LIST (1<<8)
#define FXACL_CREATEFILES (1<<9)
#define FXACL_CREATEDIRS (1<<10)
#define FXACL_TRAVERSE (1<<11)
#define FXACL_DELETEFILES (1<<12)
#define FXACL_DELETEDIRS (1<<13)
#define FXACL_READATTRS (1<<16)
#define FXACL_WRITEATTRS (1<<17)
#define FXACL_READPERMS (1<<18)
#define FXACL_WRITEPERMS (1<<19)
#define FXACL_TAKEOWNERSHIP (1<<20)
//! Expands to generic read permissions for FX::FXACL::Permissions
#define FXACL_GENREAD ((0x1<<0)|(0x1<<8)|(0x5<<16))
//! Expands to generic write permissions for FX::FXACL::Permissions
#define FXACL_GENWRITE ((0xa<<0)|(0x36<<8)|(0xa<<16))
//! Expands to generic execute permissions for FX::FXACL::Permissions
#define FXACL_GENEXECUTE ((1<<2)|(1<<11))


/*! \class FXACLIterator
\ingroup security
\brief Allows indexing of an ACL

\sa FX::FXACL
*/
struct FXACLIteratorPrivate;
class FXACL;
class FXAPIR FXACLIterator
{
	friend class FXACL;
	FXACLIteratorPrivate *p;
public:
	FXACLIterator(const FXACL &acl, bool end=false);
	FXACLIterator(const FXACLIterator &o);
	~FXACLIterator();
	FXACLIterator &operator=(const FXACLIterator &o);
	bool operator==(const FXACLIterator &o) const;
	bool operator!=(const FXACLIterator &o) const;
	bool atEnd() const;
	const FXACL::Entry &operator *() const;
	const FXACL::Entry *operator->() const;
	FXACLIterator &operator++();
	FXACLIterator &operator+=(FXuint i);
	FXACLIterator &operator--();
	FXACLIterator &operator-=(FXuint i);
};

} // namespace

#endif
