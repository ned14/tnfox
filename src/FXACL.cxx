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

#include "FXACL.h"
#include "FXException.h"
#include "FXRollback.h"
#include "FXThread.h"
#include "FXTrans.h"
#include "FXProcess.h"
#include "FXErrCodes.h"
#include "qvaluelist.h"
#include "qcstring.h"
#include <assert.h>
#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"
#include "Sddl.h"
#include "Aclapi.h"
#include "Lm.h"
#define SECURITY_WIN32
#include "Security.h"
#include <io.h>
#endif
#ifdef USE_POSIX
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#ifdef HAVE_PAM
#include <security/pam_appl.h>
#endif
//#include <shadow.h>
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXDLLLOCAL FXACLEntityPrivate
{
	FXString machine;
#ifdef USE_WINAPI
	bool win32alloc;
	SID *sid, *group;
	HANDLE token;
	void copySid(SID *osid, SID *ogroup, const FXString &_machine)
	{
		assert(IsValidSid(osid)); assert(IsValidSid(ogroup));
		DWORD len;
		win32alloc=false;
		FXERRHM(sid=(SID *) malloc((len=GetLengthSid(osid))));
		FXERRHWIN(CopySid(len, sid, osid));
		FXERRHM(group=(SID *) malloc((len=GetLengthSid(ogroup))));
		FXERRHWIN(CopySid(len, group, ogroup));
		machine=_machine;
	}
	FXACLEntityPrivate(SID *osid, SID *ogroup, const FXString &_machine) : win32alloc(false), sid(0), group(0), token(0)
	{
		FXRBOp unconstr=FXRBConstruct(this);
		copySid(osid, ogroup, _machine);
		unconstr.dismiss();
	}
	FXACLEntityPrivate(bool _win32alloc, SID *osid, SID *ogroup, const FXString &_machine) : win32alloc(_win32alloc), sid(osid), group(ogroup), token(0), machine(_machine)
	{
	}
	FXACLEntityPrivate(const FXACLEntityPrivate &o) : win32alloc(false), sid(0), group(0), token(0)
	{
		FXRBOp unconstr=FXRBConstruct(this);
		copySid(o.sid, o.group, o.machine);
		if(o.token)
		{
			FXERRHWIN(DuplicateTokenEx(o.token, 0, NULL, SecurityImpersonation, TokenImpersonation, &token));
		}
		unconstr.dismiss();
	}
	~FXACLEntityPrivate();
#endif
#ifdef USE_POSIX
	uid_t userId;	// 0=root, -1=public, -2=owner
	gid_t groupId;	// 0=root, -1=public, -2=owner
	bool amGroup, amOwner, amPublic;
	FXACLEntityPrivate(uid_t _userId, gid_t _groupId, bool _amGroup, const FXString &_machine)
		: userId(_userId), groupId(_groupId), amGroup(_amGroup), amOwner(false), amPublic(false), machine(_machine) { }
#endif
	const char *computer() const { return machine.empty() ? 0 : machine.text(); }
private:
	FXACLEntityPrivate &operator=(const FXACLEntityPrivate &);
};
#ifdef USE_WINAPI
FXACLEntityPrivate::~FXACLEntityPrivate()
{ FXEXCEPTIONDESTRUCT1 {
	if(sid)
	{
		if(win32alloc)
		{
			FreeSid(sid);
		}
		else free(sid);
		sid=0;
	}
	if(group)
	{
		if(win32alloc)
		{
			FreeSid(group);
		}
		else free(group);
		group=0;
	}
	if(token)
	{
		FXERRHWIN(CloseHandle(token));
		token=0;
	}
} FXEXCEPTIONDESTRUCT2; }
#endif

FXACLEntity::FXACLEntity() : p(0)
{
}
FXACLEntity::FXACLEntity(const FXACLEntity &o) : p(0)
{
	FXERRHM(p=new FXACLEntityPrivate(*o.p));
}
FXACLEntity::~FXACLEntity()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
FXACLEntity &FXACLEntity::operator=(const FXACLEntity &o)
{
	FXDELETE(p);
	FXERRHM(p=new FXACLEntityPrivate(*o.p));
	return *this;
}
bool FXACLEntity::operator==(const FXACLEntity &o) const
{
	if(!p && !o.p) return true;
	if(!p || !o.p) return false;
#ifdef USE_WINAPI
	return EqualSid(p->sid, o.p->sid) ? true : false;
#endif
#ifdef USE_POSIX
	if(p->amGroup || o.p->amGroup)
	{
		if(!p->amGroup || !o.p->amGroup) return false;
		return p->groupId==o.p->groupId;
	}
	return p->userId==o.p->userId;
#endif
}
bool FXACLEntity::isGroup() const
{
	if(!p) return false;
#ifdef USE_WINAPI
	SID_NAME_USE use;
	char b1[512], b2[512];
	DWORD accsize=sizeof(b1), compsize=sizeof(b2);
	FXERRHWIN(LookupAccountSid(p->computer(), p->sid, b1, &accsize, b2, &compsize, &use));
	return SidTypeGroup==use || SidTypeWellKnownGroup==use;
#endif
#ifdef USE_POSIX
	return p->amGroup;
#endif
}
FXACLEntity FXACLEntity::group() const
{
	if(!p) return FXACLEntity();
	FXACLEntity ret;
#ifdef USE_WINAPI
	FXERRHM(ret.p=new FXACLEntityPrivate(p->group, p->group, p->machine));
#endif
#ifdef USE_POSIX
	FXERRHM(ret.p=new FXACLEntityPrivate(p->groupId, p->groupId, true, p->machine));
#endif
	return ret;
}
FXString FXACLEntity::asString(bool full) const
{
	if(!p) return FXString();
#ifdef USE_WINAPI
	TCHAR account[1024], computer[1024];
	DWORD accsize=sizeof(account), compsize=sizeof(computer);
	SID_NAME_USE use;
	FXERRHWIN(LookupAccountSid(p->computer(), p->sid, account, &accsize, computer, &compsize, &use));
	FXString ret=(computer[0]) ? "%1/%2" : "%1%2";
	if(full)
	{
		FXString sidstr;
		LPTSTR _sidstr;
		FXERRHWIN(ConvertSidToStringSid(p->sid, &_sidstr));
		sidstr=_sidstr;
		FXERRHWIN(!LocalFree(_sidstr));
		return (ret+" {%3}").arg(computer).arg(account).arg(sidstr);
	}
	else return ret.arg(computer).arg(account);
#endif
#ifdef USE_POSIX
	if(p->amOwner)
	{
		return p->amGroup ? FXTrans::tr("FXACLEntity", "Owner group")
			: FXTrans::tr("FXACLEntity", "Owner");
	}
	if(p->amPublic) return FXTrans::tr("FXACLEntity", "Everything");
	int ret;
	QByteArray store(1024);
	struct passwd *_userinfo=0, userinfo={0};
	struct group *_groupinfo=0, groupinfo={0};
	if(!p->amGroup)
	{
		for(;;)
		{
			if(!(ret=getpwuid_r(p->userId, &userinfo, (char *) store.data(), store.size(), &_userinfo))) break;
			if(ret && ERANGE!=ret) FXERRHOS(-1);
			store.resize(store.size()+1024);
		}
		if(!_userinfo) memset(&userinfo, 0, sizeof(userinfo));
	}
	else
	{
		for(;;)
		{
			if(!(ret=getgrgid_r(p->groupId, &groupinfo, (char *) store.data(), store.size(), &_groupinfo))) break;
			if(ret && ERANGE!=ret) FXERRHOS(-1);
			store.resize(store.size()+1024);
		}
		if(!_groupinfo) memset(&groupinfo, 0, sizeof(groupinfo));
	}
	FXString retstr("%1");
	if(!p->amGroup) retstr.arg(FXString(userinfo.pw_name));
	else retstr.arg(FXString(groupinfo.gr_name));
	if(full)
	{
		retstr+=" {%2}";
		if(!p->amGroup) retstr.arg(userinfo.pw_uid); else retstr.arg(groupinfo.gr_gid);
	}
	return retstr;
#endif
}
bool FXACLEntity::isLoginPassword(const FXchar *password) const
{
	if(!p) return false;
#ifdef USE_WINAPI
	TCHAR account[1024], computer[1024];
	DWORD accsize=sizeof(account), compsize=sizeof(computer);
	SID_NAME_USE use;
	FXERRHWIN(LookupAccountSid(p->computer(), p->sid, account, &accsize, computer, &compsize, &use));
	if(p->token)
	{
		FXERRHWIN(CloseHandle(p->token));
		p->token=0;
	}
	/* Needs SE_TCB_NAME privs on Win2k :(
	HANDLE usertoken;
	if(LogonUser(account, computer, password, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &usertoken))
	{
		FXERRHWIN(CloseHandle(usertoken));
		return true;
	}
	else
	{
		if(GetLastError()!=ERROR_LOGON_FAILURE) FXERRHWIN(0);
	}
	return false;*/

	/* Microsoft are donkey fuckers for making me have to go through this.
	Why the fuck is it so fucking hard just to check if a password is right? :(
	*/
	SEC_WINNT_AUTH_IDENTITY_EX swai={ SEC_WINNT_AUTH_IDENTITY_VERSION, sizeof(swai) };
	swai.User    =(FXuchar *) account;  swai.UserLength=accsize;
	swai.Domain  =(FXuchar *) computer; swai.DomainLength=compsize;
	swai.Password=(FXuchar *) password; swai.PasswordLength=strlen(password);
	swai.Flags=SEC_WINNT_AUTH_IDENTITY_ANSI;
	// Setup client & server
	CredHandle clientcred, servercred; TimeStamp expiry;
	FXERRHWIN(SEC_E_OK==AcquireCredentialsHandle(0, "NTLM",
		SECPKG_CRED_OUTBOUND, NULL, &swai, NULL, NULL,
		&clientcred, &expiry));
	FXRBOp unclient=FXRBFunc(FreeCredentialsHandle, &clientcred);
	FXERRHWIN(SEC_E_OK==AcquireCredentialsHandle(0, "NTLM",
		SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL,
		&servercred, &expiry));
	FXRBOp unserver=FXRBFunc(FreeCredentialsHandle, &servercred);
	BYTE buffA_[8192], buffB_[8192];
	SecBuffer buffA={ sizeof(buffA_), SECBUFFER_TOKEN, buffA_ };
	SecBuffer buffB={ sizeof(buffB_), SECBUFFER_TOKEN, buffB_ };
	SecBufferDesc buffAdesc={ SECBUFFER_VERSION, 1, &buffA };
	SecBufferDesc buffBdesc={ SECBUFFER_VERSION, 1, &buffB };
	SecBufferDesc *clientin=0, *clientout=&buffAdesc, *serverin=&buffAdesc, *serverout=&buffBdesc;
	struct Handles {
		CtxtHandle clienthout, serverhout;
		static void del(CtxtHandle *clienthout, CtxtHandle*serverhout) { DeleteSecurityContext(clienthout); DeleteSecurityContext(serverhout); }
	} handles;
	CtxtHandle *clienthin=0, *clienthout=&handles.clienthout, *serverhin=0, *serverhout=&handles.serverhout;
	FXRBOp unhandles=FXRBFunc(&Handles::del, &handles.clienthout, &handles.serverhout);
	bool doClient=true, doServer=true;
	SEC_CHAR target[32]="";
	// Perform challenge acting both as server and client
	while(doClient || doServer)
	{
		if(doClient)
		{
			buffA.cbBuffer=sizeof(buffA_);
			DWORD flags=0;
			SECURITY_STATUS ret=InitializeSecurityContext(&clientcred, clienthin, target,
				0, 0, SECURITY_NATIVE_DREP, clientin, 0, clienthout,
				clientout, &flags, &expiry);
			if(SEC_E_OK==ret) doClient=false;
			else if(SEC_I_CONTINUE_NEEDED==ret)
			{
				clienthin=clienthout;
				clientin=serverout;
			}
			else if(SEC_E_LOGON_DENIED==ret) return false;
			else { FXERRHWIN(0); }
		}
		if(doServer)
		{
			buffB.cbBuffer=sizeof(buffB_);
			DWORD flags=0;
			SECURITY_STATUS ret=AcceptSecurityContext(&servercred, serverhin, serverin,
				0, SECURITY_NATIVE_DREP, serverhout, serverout, &flags,
				&expiry);
			if(SEC_E_OK==ret) doServer=false;
			else if(SEC_I_CONTINUE_NEEDED==ret)
			{
				serverhin=serverhout;
			}
			else if(SEC_E_LOGON_DENIED==ret) return false;
			else { FXERRHWIN(0); }
		}
	}
	// Fetch the impersonation token
	HANDLE temp;
	/*{
		FXERRHWIN(SEC_E_OK==ImpersonateSecurityContext(serverhout));
		FXRBOp unimpers=FXRBFunc(RevertSecurityContext, serverhout);
		FXERRHWIN(OpenThreadToken(GetCurrentThread(), TOKEN_READ|TOKEN_DUPLICATE|TOKEN_IMPERSONATE,
			TRUE, &temp));
	}*/
	FXERRHWIN(SEC_E_OK==QuerySecurityContextToken(serverhout, &temp));
	FXRBOp untemp=FXRBFunc(CloseHandle, temp);
	// Take a copy with reduced permissions
	FXERRHWIN(DuplicateTokenEx(temp, TOKEN_DUPLICATE|TOKEN_IMPERSONATE, NULL, SecurityImpersonation, TokenImpersonation, &p->token));
	return true;
#endif
#ifdef USE_POSIX
	int ret;
	QByteArray store(1024);
	struct passwd *_userinfo=0, userinfo={0};
	for(;;)
	{
		if(!(ret=getpwuid_r(p->userId, &userinfo, (char *) store.data(), store.size(), &_userinfo))) break;
		if(ret && ERANGE!=ret) FXERRHOS(-1);
		store.resize(store.size()+1024);
	}
	// Might as well try old-style passwords
	FXString epass(crypt(password, password)); // crypt() has no thread safe equivalent :(
	if(epass==userinfo.pw_passwd) return true;
#ifdef HAVE_PAM
	{	// PAM is used on all modern Unices
		struct PamCode {
			static int convfunc(int msgno, const struct pam_message **msgs, struct pam_response **response, void *ptr)
			{
				*response=(struct pam_response *) malloc(msgno*sizeof(struct pam_response));
				if(*response==NULL) return PAM_CONV_ERR;
				//fxmessage("Got %d msgs\n", msgno);
				for(int i = 0; i<msgno; i++)
				{
					//fxmessage("Msg %d is type %d, pass=%s\n", i, (*msgs)[i].msg_style, (char *) ptr);
					(*response)[i].resp=strdup((char *)ptr);
					(*response)[i].resp_retcode=0;
				}
				return PAM_SUCCESS;
			}
			static void faildelay(int, unsigned int micro_sec, void *)
			{	// Two seconds is too much. Let's use a half second
				//fxmessage("Waiting %d ms ...\n", 200+micro_sec/10000);
				FXThread::msleep(200+micro_sec/10000);
			}
		};
		pam_handle_t *pamh;
		int ret=0;
		struct pam_conv conv={ PamCode::convfunc, (void *) password };
		/* Ensure we're root or can act like root. PAM does appear to work without
		being root but only for uid!=0 which is annoying. Best be consistent */
		FXERRH(!getuid() /*|| !geteuid()*/, FXTrans::tr("FXACLEntity", "Must have root privileges for this operation"), FXACLENTITY_NEEDROOTPRIVS, 0);
		if(PAM_SUCCESS==(ret=pam_start("passwd", userinfo.pw_name, &conv, &pamh)))
		{
			FXRBOp unpam=FXRBFunc(pam_end, pamh, ret);
#ifdef PAM_FAIL_DELAY
			if(PAM_SUCCESS!=(ret=pam_set_item(pamh, PAM_FAIL_DELAY, (void *) PamCode::faildelay)))
			{
				FXERRG(pam_strerror(pamh, ret), FXACLENTITY_PAMERROR, 0);
			}
			if(PAM_SUCCESS!=(ret=pam_fail_delay(pamh, 500*1000))) // Want half second delay
			{
				FXERRG(pam_strerror(pamh, ret), FXACLENTITY_PAMERROR, 0);
			}
#endif
			if(PAM_SUCCESS!=(ret=pam_authenticate(pamh, PAM_SILENT)))
			{
				//fxmessage("Failed with %s\n", pam_strerror(pamh, ret));
				if(PAM_AUTH_ERR==ret) return false;
				FXERRG(pam_strerror(pamh, ret), FXACLENTITY_PAMERROR, 0);
			}
			if(PAM_SUCCESS!=(ret=pam_acct_mgmt(pamh, PAM_SILENT)))
			{
				FXERRG(pam_strerror(pamh, ret), FXACLENTITY_PAMERROR, 0);
			}
			else return true;
		}
		else
		{
			FXERRG(pam_strerror(pamh, ret), FXACLENTITY_PAMERROR, 0);
		}
	}
#endif
	/*{	// Try shadow password file
		struct spwd *_suserinfo=0, suserinfo={0};
		for(;;)
		{
			if(!(ret=getspnam_r(userinfo.pw_name, &suserinfo, (char *) store.data(), store.size(), &_suserinfo))) break;
			if(ret && ERANGE!=ret) FXERRHOS(-1);
			store.resize(store.size()+1024);
		}
		FXERRH(_suserinfo && suserinfo.sp_pwdp, FXTrans::tr("FXACLEntity", "Shadow password file unavailable"), FXACLENTITY_NOSHADOWPASSWORDFILE, 0);
		fxmessage("mine is %s his is %s\n", epass.text(), suserinfo.sp_pwdp);
		if(epass==suserinfo.sp_pwdp) return true;
	}*/
	return false;
#endif
}

static FXMutex staticmethodlock;
const FXACLEntity &FXACLEntity::currentUser()
{
	static FXACLEntity ret;
	if(ret.p) return ret;
	FXMtxHold lh(staticmethodlock);
#ifdef USE_WINAPI
	HANDLE myprocessh;
	DWORD userinfolen=0, groupinfolen=0;
    FXERRHWIN(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &myprocessh));
	FXRBOp unprocessh=FXRBFunc(&CloseHandle, myprocessh);
	GetTokenInformation(myprocessh, TokenUser, NULL, userinfolen, &userinfolen);
	GetTokenInformation(myprocessh, TokenPrimaryGroup, NULL, groupinfolen, &groupinfolen);
	TOKEN_USER *userinfo;
	TOKEN_PRIMARY_GROUP *groupinfo;
	FXERRHM(userinfo=(TOKEN_USER *) malloc(userinfolen));
	FXRBOp unalloc1=FXRBAlloc(userinfo);
	FXERRHWIN(GetTokenInformation(myprocessh, TokenUser, userinfo, userinfolen, &userinfolen));
	FXERRHM(groupinfo=(TOKEN_PRIMARY_GROUP *) malloc(groupinfolen));
	FXRBOp unalloc2=FXRBAlloc(groupinfo);
	FXERRHWIN(GetTokenInformation(myprocessh, TokenPrimaryGroup, groupinfo, groupinfolen, &groupinfolen));
	FXERRHM(ret.p=new FXACLEntityPrivate((SID *) userinfo->User.Sid, (SID *) groupinfo->PrimaryGroup, FXString::nullStr()));
#endif
#ifdef USE_POSIX
	FXERRHM(ret.p=new FXACLEntityPrivate(getuid(), getgid(), false, FXString::nullStr()));
#endif
	return ret;
}

const FXACLEntity &FXACLEntity::everything()
{
	static FXACLEntity ret;
	if(ret.p) return ret;
	FXMtxHold lh(staticmethodlock);
#ifdef USE_WINAPI
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld=SECURITY_WORLD_SID_AUTHORITY, CreatorAuth=SECURITY_CREATOR_SID_AUTHORITY;
	SID *world, *_creatorgroup;
	FXERRHM(ret.p=new FXACLEntityPrivate(true, 0, 0, FXString::nullStr()));
	FXERRHWIN(AllocateAndInitializeSid(&SIDAuthWorld, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
        (PSID *) &world));
	FXERRHWIN(AllocateAndInitializeSid(&CreatorAuth, 1,
		SECURITY_CREATOR_GROUP_RID,
		0, 0, 0, 0, 0, 0, 0,
        (PSID *) &_creatorgroup));
	ret.p->sid=world;
	ret.p->group=_creatorgroup;
#endif
#ifdef USE_POSIX
	FXERRHM(ret.p=new FXACLEntityPrivate((uid_t)-1, (gid_t)-1, true, FXString::nullStr()));
	ret.p->amPublic=true;
#endif
	return ret;
}
const FXACLEntity &FXACLEntity::root()
{
	static FXACLEntity ret;
	if(ret.p) return ret;
	FXMtxHold lh(staticmethodlock);
#ifdef USE_WINAPI
	SID_IDENTIFIER_AUTHORITY SIDAuth=SECURITY_NT_AUTHORITY;
	SID *_root, *_rootgroup;
	FXERRHM(ret.p=new FXACLEntityPrivate(true, 0, 0, FXString::nullStr()));
	// Ask local computer for its admin account and transfer (this is way too hard ... :( )
	USER_MODALS_INFO_2 *umi=0;
	FXERRHWIN(NERR_Success==NetUserModalsGet(NULL, 2, (LPBYTE *) &umi));
	FXRBOp unumi=FXRBFunc(NetApiBufferFree, umi);
	assert(IsValidSid(umi->usrmod2_domain_id));
	BYTE subauthorities=*GetSidSubAuthorityCount(umi->usrmod2_domain_id);
	FXERRH(subauthorities<=8, "Subauthorities greater than eight", 0, FXERRH_ISDEBUG);
	FXERRHWIN(AllocateAndInitializeSid(GetSidIdentifierAuthority(umi->usrmod2_domain_id), subauthorities+1,
		0, 0, 0, 0, 0, 0, 0, 0,
        (PSID *) &_root));
	BYTE idx;
	for(idx=0; idx<subauthorities; idx++)
	{
		*GetSidSubAuthority(_root, idx)=*GetSidSubAuthority(umi->usrmod2_domain_id, idx);
	}
	*GetSidSubAuthority(_root, subauthorities)=DOMAIN_USER_RID_ADMIN;
	FXERRHWIN(AllocateAndInitializeSid(&SIDAuth, 2,
		SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
        (PSID *) &_rootgroup));
	ret.p->sid=_root;
	ret.p->group=_rootgroup;	// Note: used also by lookupUser and other things
#endif
#ifdef USE_POSIX
	// Am I safe in assuming root will always be uid 0, gid 0?
	FXERRHM(ret.p=new FXACLEntityPrivate(0, 0, false, FXString::nullStr()));
#endif
	return ret;
}
const FXACLEntity &FXACLEntity::owner()
{
	static FXACLEntity ret;
	if(ret.p) return ret;
	FXMtxHold lh(staticmethodlock);
#ifdef USE_WINAPI
	SID_IDENTIFIER_AUTHORITY CreatorAuth=SECURITY_CREATOR_SID_AUTHORITY;
	SID *_creatorowner, *_creatorgroup;
	FXERRHM(ret.p=new FXACLEntityPrivate(true, 0, 0, FXString::nullStr()));
	FXERRHWIN(AllocateAndInitializeSid(&CreatorAuth, 1,
		SECURITY_CREATOR_OWNER_RID,
		0, 0, 0, 0, 0, 0, 0,
        (PSID *) &_creatorowner));
	FXERRHWIN(AllocateAndInitializeSid(&CreatorAuth, 1,
		SECURITY_CREATOR_GROUP_RID,
		0, 0, 0, 0, 0, 0, 0,
        (PSID *) &_creatorgroup));
	ret.p->sid=_creatorowner;
	ret.p->group=_creatorgroup;
#endif
#ifdef USE_POSIX
	FXERRHM(ret.p=new FXACLEntityPrivate((uid_t)-2, (gid_t)-2, false, FXString::nullStr()));
	ret.p->amOwner=true;
#endif
	return ret;
}
FXACLEntity FXACLEntity::lookupUser(const FXString &username, const FXString &machine)
{
	FXACLEntity ret;
#ifdef USE_WINAPI
	const FXACLEntity &_root=root();
	char sidbuff[64], compbuff[512];
	SID *sid=(SID *) sidbuff;
	DWORD sidlen=sizeof(sidbuff), complen=sizeof(compbuff);
	SID_NAME_USE use;
	FXERRHWIN(LookupAccountName(machine.empty() ? 0 : machine.text(), username.text(),
		sidbuff, &sidlen, compbuff, &complen, &use));
	FXERRHM(ret.p=new FXACLEntityPrivate(sid, _root.p->group, machine));
#endif
#ifdef USE_POSIX
	int _ret;
	QByteArray store(1024);
	struct passwd *_userinfo=0, userinfo={0};
	for(;;)
	{
		if(!(_ret=getpwnam_r(username.text(), &userinfo, (char *) store.data(), store.size(), &_userinfo))) break;
		if(_ret && ERANGE!=_ret) FXERRHOS(-1);
		store.resize(store.size()+1024);
	}
	FXERRH(_userinfo, FXTrans::tr("FXACLEntity", "Unknown user"), FXACLENTITY_UNKNOWNUSER, 0);
	FXERRHM(ret.p=new FXACLEntityPrivate(userinfo.pw_uid, userinfo.pw_gid, false, machine));
#endif
	return ret;
}

// Returns a string of the form "RWXA RWRWO" or "LTCDCD RWRWO"
FXString FXACL::Permissions::asString(EntityType type) const
{
	FXchar buffer[16], *end=0;
	if(File==type || Pipe==type || MemMap==type)
	{
		buffer[0]=read		? 'R' : 'r';
		buffer[1]=write		? 'W' : 'w';
		buffer[2]=execute	? 'X' : 'x';
		buffer[3]=append	? 'A' : 'a';
		if(MemMap==type)
		{
			buffer[4]=copyonwrite ? 'C' : 'c';
			end=buffer+5;
		}
		else end=buffer+4;
	}
	else if(Directory==type)
	{
		buffer[0]=list			? 'L' : 'l';
		buffer[1]=traverse		? 'T' : 't';
		buffer[2]=createfiles	? 'C' : 'c';
		buffer[3]=deletefiles	? 'D' : 'd';
		buffer[4]=createdirs	? 'C' : 'c';
		buffer[5]=deletedirs	? 'D' : 'd';
		end=buffer+6;
	}
    end[0]=' ';
	end[1]=readattrs	? 'R' : 'r';
	end[2]=writeattrs	? 'W' : 'w';
	end[3]=readperms	? 'R' : 'r';
	end[4]=writeperms	? 'W' : 'w';
	end[5]=takeownership ? 'T' : 't';
	end[6]=0;
	return FXString(buffer);
}

#ifdef USE_WINAPI
static DWORD translate(FXACL::Permissions &p, FXACL::EntityType type)
{
	DWORD perms=0;
	//if(p.read)			perms|=GENERIC_READ;
	//if(p.write)			perms|=GENERIC_WRITE;
	//if(p.execute)		perms|=GENERIC_EXECUTE;
	// Common to all
	if(p.readattrs)		perms|=FILE_READ_ATTRIBUTES|FILE_READ_EA;
	if(p.writeattrs)	perms|=FILE_WRITE_ATTRIBUTES|FILE_WRITE_EA;
	if(p.readperms)		perms|=READ_CONTROL;
	if(p.writeperms)	perms|=WRITE_DAC;
	if(p.takeownership) perms|=WRITE_OWNER;
	if(FXACL::File==type || FXACL::Directory==type || FXACL::Pipe==type)
	{
		if(p.read)			perms|=FILE_READ_DATA|SYNCHRONIZE;
		if(p.write)			perms|=FILE_WRITE_DATA|SYNCHRONIZE;
		if(p.append)		perms|=FILE_APPEND_DATA|SYNCHRONIZE;
	}
	switch(type)
	{
	case FXACL::File:
	case FXACL::Directory:
		{
			if(p.execute)		perms|=FILE_EXECUTE|SYNCHRONIZE;

			if(p.list)			perms|=FILE_LIST_DIRECTORY;
			if(p.createfiles)	perms|=FILE_ADD_FILE;
			if(p.createdirs)	perms|=FILE_ADD_SUBDIRECTORY;
			if(p.traverse)		perms|=FILE_TRAVERSE;
			if(p.deletefiles)	perms|=DELETE;
			if(p.deletedirs)	perms|=FILE_DELETE_CHILD;
			break;
		}
	case FXACL::Pipe:
		{
			if(p.execute)		perms|=SYNCHRONIZE;
			if(p.append || p.createdirs) perms|=FILE_CREATE_PIPE_INSTANCE;
			break;
		}
	case FXACL::MemMap:
		{	// I vaguely remember that execute needs to be set for writes?
			if(p.read)			perms|=SECTION_MAP_READ;
			if(p.write)			perms|=SECTION_MAP_WRITE;
			if(p.execute)		perms|=SECTION_MAP_EXECUTE;
			if(p.append)		perms|=SECTION_EXTEND_SIZE;
			if(p.copyonwrite)	perms|=SECTION_QUERY;
			if(p.deletefiles)	perms|=DELETE;
			break;
		}
	}
	return perms;
}
static FXACL::Permissions translateBack(DWORD p, FXACL::EntityType type)
{
	FXACL::Permissions perms;
	perms.readattrs    =(p & (FILE_READ_ATTRIBUTES|FILE_READ_EA))!=0;
	perms.writeattrs   =(p & (FILE_WRITE_ATTRIBUTES|FILE_WRITE_EA))!=0;
	perms.readperms    =(p & READ_CONTROL)!=0;
	perms.writeperms   =(p & WRITE_DAC)!=0;
	perms.takeownership=(p & WRITE_OWNER)!=0;

	if(FXACL::File==type || FXACL::Directory==type || FXACL::Pipe==type)
	{
		perms.read   =(p & (FILE_READ_DATA|SYNCHRONIZE))  ==(FILE_READ_DATA|SYNCHRONIZE);
		perms.write  =(p & (FILE_WRITE_DATA|SYNCHRONIZE)) ==(FILE_WRITE_DATA|SYNCHRONIZE);
		perms.append =(p & (FILE_APPEND_DATA|SYNCHRONIZE))==(FILE_APPEND_DATA|SYNCHRONIZE);
	}
	switch(type)
	{
	case FXACL::File:
	case FXACL::Directory:
		{
			perms.execute=(p & (FILE_EXECUTE|SYNCHRONIZE))    ==(FILE_EXECUTE|SYNCHRONIZE);

			perms.list       =(p & FILE_LIST_DIRECTORY)!=0;
			perms.createfiles=(p & FILE_ADD_FILE)!=0;
			perms.createdirs =(p & FILE_ADD_SUBDIRECTORY)!=0;
			perms.traverse   =(p & FILE_TRAVERSE)!=0;
			perms.deletefiles=(p & DELETE)!=0;
			perms.deletedirs =(p & FILE_DELETE_CHILD)!=0;
			break;
		}
	case FXACL::Pipe:
		{
			perms.execute=(p & SYNCHRONIZE)!=0;
			perms.append=perms.createdirs=(p & FILE_CREATE_PIPE_INSTANCE)!=0;
			break;
		}
	case FXACL::MemMap:
		{
			perms.read		=(p & SECTION_MAP_READ)!=0;
			perms.write		=(p & SECTION_MAP_WRITE)!=0;
			perms.execute	=(p & SECTION_MAP_EXECUTE)!=0;
			perms.append	=(p & SECTION_EXTEND_SIZE)!=0;
			perms.copyonwrite=(p & SECTION_QUERY)!=0;
			perms.deletefiles=(p & DELETE)!=0;
			break;
		}
	}
	return perms;
}
#endif

#ifdef USE_WINAPI
class FXACLInit
{	// Cached here to speed FXACL::check()
public:
	HANDLE myprocessh;
	DWORD pssize;
	PRIVILEGE_SET *ps;
	FXACLInit()
	{
		FXERRHWIN(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_ADJUST_PRIVILEGES, &myprocessh));
		//DWORD privsinfolen=0;
		//FXERRHWIN(GetTokenInformation(myprocessh, TokenPrivileges, NULL, privsinfolen, &privsinfolen));
		//TOKEN_PRIVILEGES *privsinfo;
		//FXERRHM(privsinfo=malloc(privsinfolen+sizeof(DWORD)));
		//FXRBOp unalloc=FXRBAlloc(privsinfo);
		//FXERRHWIN(GetTokenInformation(myprocessh, TokenPrivileges, privsinfo, privsinfolen, &privsinfolen));
		//// Stupid god damn extra DWORD ...
		//PRIVILEGE_SET *ps=(PRIVILEGE_SET *) privsinfo;
		//memmove(&ps->Privileges, &privsinfo->Privileges, privsinfolen-sizeof(DWORD));
		//ps->Control=PRIVILEGE_SET_ALL_NECESSARY;
		pssize=sizeof(PRIVILEGE_SET)+3*sizeof(LUID_AND_ATTRIBUTES);
		FXERRHM(ps=(PRIVILEGE_SET *) malloc(pssize));
		ps->PrivilegeCount=3;
		ps->Control=0;
		// Just some useful privileges
		ps->Privilege[0].Attributes=0;
		FXERRHWIN(LookupPrivilegeValue(0, SE_CHANGE_NOTIFY_NAME, &ps->Privilege[0].Luid));
		ps->Privilege[1].Attributes=0;
		FXERRHWIN(LookupPrivilegeValue(0, SE_TAKE_OWNERSHIP_NAME, &ps->Privilege[1].Luid));
		ps->Privilege[2].Attributes=0;
		FXERRHWIN(LookupPrivilegeValue(0, SE_TCB_NAME, &ps->Privilege[2].Luid));

		/*{	// Try to give ourselves the SE_TCB_NAME privilege
			HANDLE adjh;
			if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &adjh)) return;
			FXRBOp unadjh=FXRBFunc(CloseHandle, adjh);
			TOKEN_PRIVILEGES tp={ 1 };
			tp.Privileges[0].Luid=ps->Privilege[2].Luid;
			tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			DWORD retlen=0;
			BOOL ret=AdjustTokenPrivileges(adjh, FALSE, &tp, 0, NULL, &retlen);
			ret=ret;
		}*/
	}
	~FXACLInit()
	{
		free(ps); ps=0;
		FXERRHWIN(CloseHandle(myprocessh)); myprocessh=0;
	}
};
static FXProcess_StaticInit<FXACLInit> fxaclinit("FXACLInit");
#endif

struct FXDLLLOCAL FXACLPrivate : public QValueList<FXACL::Entry>
{
	FXACL::EntityType type;
	FXACLEntity owner;
	bool dirty, hasInherited;
#ifdef USE_WINAPI
	mutable SECURITY_DESCRIPTOR *sd;
	mutable ACL *acl;
#endif
	FXACLPrivate(FXACL::EntityType _type, const FXACLEntity &_owner) : type(_type), owner(_owner),
#ifdef USE_WINAPI
		dirty(true), hasInherited(true), sd(0), acl(0)
#endif
#ifdef USE_POSIX
		dirty(true), hasInherited(false)
#endif
	{ }
	FXACLPrivate(const FXACLPrivate &o) : type(o.type), owner(o.owner), dirty(true), hasInherited(o.hasInherited),
#ifdef USE_WINAPI
		sd(0), acl(0),
#endif
		QValueList<FXACL::Entry>(o) { }
	FXACLPrivate &operator=(const FXACLPrivate &o)
	{
		type=o.type;
		owner=o.owner;
		dirty=true;
		hasInherited=o.hasInherited;
#ifdef USE_WINAPI
		if(sd)
		{
			free(sd);
			sd=0;
		}
		if(acl)
		{
			free(acl);
			acl=0;
		}
#endif
		QValueList<FXACL::Entry>::operator=(o);
		return *this;
	}
	~FXACLPrivate()
	{
#ifdef USE_WINAPI
		if(sd)
		{
			free(sd);
			sd=0;
		}
		if(acl)
		{
			free(acl);
			acl=0;
		}
#endif
	}
#ifdef USE_WINAPI
	void readACL(PACL _acl);
#endif
#ifdef USE_POSIX
	void buildACL(struct stat &s);
	mode_t fromACL(const FXACLEntity &owner, struct stat &s);
#endif
};
struct FXDLLLOCAL FXACLIteratorPrivate : public QValueList<FXACL::Entry>::iterator
{
	FXACL &list;
	FXACLIteratorPrivate(FXACL &_list, QValueList<FXACL::Entry> &l, bool end)
		: list(_list), QValueList<FXACL::Entry>::iterator(end ? l.end() : l.begin()) { }
	FXACLIteratorPrivate &operator=(const FXACLIteratorPrivate &o)
	{
		list=o.list;
		QValueList<FXACL::Entry>::iterator::operator=(o);
		return *this;
	}
};
#ifdef USE_WINAPI
void FXACLPrivate::readACL(PACL _acl)
{
	assert(IsValidAcl(_acl));
	ACL_SIZE_INFORMATION asi;
	FXERRHWIN(GetAclInformation(_acl, &asi, sizeof(asi), AclSizeInformation));
	for(DWORD idx=0; idx<asi.AceCount; idx++)
	{
		ACE_HEADER *aceh;
		FXERRHWIN(GetAce(_acl, idx, (LPVOID *) &aceh));
		switch(aceh->AceType)
		{
		case ACCESS_DENIED_ACE_TYPE:
			{
				ACCESS_DENIED_ACE *ace=(ACCESS_DENIED_ACE *) aceh;
				FXACLEntity entity;
				FXERRHM(entity.p=new FXACLEntityPrivate((SID *)(&(ace->SidStart)), (SID *)(&(ace->SidStart)), FXString::nullStr()));
				FXACL::Entry entry(entity, translateBack(ace->Mask, type), 0, (aceh->AceFlags & CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE)!=0);
				entry.inherited=(aceh->AceFlags & INHERITED_ACE)!=0;
				push_back(entry);
				break;
			}
		case ACCESS_ALLOWED_ACE_TYPE:
			{
				ACCESS_ALLOWED_ACE *ace=(ACCESS_ALLOWED_ACE *) aceh;
				SID *sid=(SID *)(&(ace->SidStart));
				// Can we scavenge a denied entry?
				QValueList<FXACL::Entry>::iterator it=begin();
				for(; it!=end(); ++it)
				{
					FXACL::Entry &e=*it;
					if(EqualSid(e.entity.p->sid, sid) && 0==e.grant)
					{
						e.grant=translateBack(ace->Mask, type);
						break;
					}
				}
				if(it==end())
				{
					FXACLEntity entity;
					FXERRHM(entity.p=new FXACLEntityPrivate(sid, sid, FXString::nullStr()));
					FXACL::Entry entry(entity, 0, translateBack(ace->Mask, type), (aceh->AceFlags & CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE)!=0);
					entry.inherited=(aceh->AceFlags & INHERITED_ACE)!=0;
					push_back(entry);
				}
				break;
			}
		}
	}
}
static SE_OBJECT_TYPE mapType(FXACL::EntityType type)
{
	switch(type)
	{
	case FXACL::File:
		return SE_FILE_OBJECT;
	case FXACL::Directory:
		return SE_FILE_OBJECT;
	case FXACL::Pipe:
		return SE_KERNEL_OBJECT;
	case FXACL::MemMap:
		return SE_KERNEL_OBJECT;
	default:
		return SE_UNKNOWN_OBJECT_TYPE;
	}
}
#endif
#ifdef USE_POSIX
void FXACLPrivate::buildACL(struct stat &s)
{
	FXACL::Permissions u,g,o;
	u.setGenRead(s.st_mode & S_IRUSR).setGenWrite(s.st_mode & S_IWUSR).setGenExecute(s.st_mode & S_IXUSR);
	u.setTakeOwnership(0==s.st_uid);
	g.setGenRead(s.st_mode & S_IRGRP).setGenWrite(s.st_mode & S_IWGRP).setGenExecute(s.st_mode & S_IXGRP);
	o.setGenRead(s.st_mode & S_IROTH).setGenWrite(s.st_mode & S_IWOTH).setGenExecute(s.st_mode & S_IXOTH);
	if(0==getuid()) o.setTakeOwnership(true);
	const FXACLEntity &ownerent=FXACLEntity::owner();
	if(u) append(FXACL::Entry(ownerent, 0, u));
	if(g) append(FXACL::Entry(ownerent.group(), 0, g));
	if(o) append(FXACL::Entry(FXACLEntity::everything(), 0, o));
}
mode_t FXACLPrivate::fromACL(const FXACLEntity &owner, struct stat &s)
{
	mode_t mode=s.st_mode & ~(S_IRWXU|S_IRWXG|S_IRWXO);
	for(QValueList<FXACL::Entry>::const_iterator it=begin(); it!=end(); ++it)
	{
		const FXACL::Entry &e=*it;
		if(e.entity.p->amGroup)
		{
			if(e.entity.p->amPublic)
			{	// Public includes group and user
				if(e.grant & FXACL_GENREAD)    mode|=S_IROTH|S_IRGRP|S_IRUSR;
				if(e.grant & FXACL_GENWRITE)   mode|=S_IWOTH|S_IWGRP|S_IWUSR;
				if(e.grant & FXACL_GENEXECUTE) mode|=S_IXOTH|S_IXGRP|S_IXUSR;
			}
			else if(e.entity.p->amOwner || e.entity.p->groupId==owner.p->groupId)
			{
				if(e.grant & FXACL_GENREAD)    mode|=S_IRGRP;
				if(e.grant & FXACL_GENWRITE)   mode|=S_IWGRP;
				if(e.grant & FXACL_GENEXECUTE) mode|=S_IXGRP;
			}
		}
		else
		{
			if(e.entity.p->amOwner || e.entity.p->userId==owner.p->userId)
			{
				if(e.grant & FXACL_GENREAD)    mode|=S_IRUSR;
				if(e.grant & FXACL_GENWRITE)   mode|=S_IWUSR;
				if(e.grant & FXACL_GENEXECUTE) mode|=S_IXUSR;
			}
		}
	}
	for(QValueList<FXACL::Entry>::const_iterator it=begin(); it!=end(); ++it)
	{
		const FXACL::Entry &e=*it;
		if(e.entity.p->amGroup)
		{
			if(e.entity.p->amPublic)
			{	// Public includes group and user
				if(e.deny & FXACL_GENREAD)    mode&=~(S_IROTH|S_IRGRP|S_IRUSR);
				if(e.deny & FXACL_GENWRITE)   mode&=~(S_IWOTH|S_IWGRP|S_IWUSR);
				if(e.deny & FXACL_GENEXECUTE) mode&=~(S_IXOTH|S_IXGRP|S_IXUSR);
			}
			else if(e.entity.p->amOwner || e.entity.p->groupId==owner.p->groupId)
			{
				if(e.deny & FXACL_GENREAD)    mode&=~S_IRGRP;
				if(e.deny & FXACL_GENWRITE)   mode&=~S_IWGRP;
				if(e.deny & FXACL_GENEXECUTE) mode&=~S_IXGRP;
			}
		}
		else
		{
			if(e.entity.p->amOwner || e.entity.p->userId==owner.p->userId)
			{
				if(e.deny & FXACL_GENREAD)    mode&=~S_IRUSR;
				if(e.deny & FXACL_GENWRITE)   mode&=~S_IWUSR;
				if(e.deny & FXACL_GENEXECUTE) mode&=~S_IXUSR;
			}
		}
	}
	return mode;
}
#endif

FXACL::FXACL(FXACL::EntityType type, const FXACLEntity &owner) : p(0)
{
	FXERRHM(p=new FXACLPrivate(type, owner));
}
void FXACL::init(void *_sd, FXACL::EntityType type)
{
#ifdef USE_WINAPI
	SECURITY_DESCRIPTOR *sd=(SECURITY_DESCRIPTOR *) _sd;
	FXRBOp unsd=FXRBFunc(&LocalFree, sd);
	PACL acl;
	SID *owner, *group;
	assert(IsValidSecurityDescriptor(sd));
	SECURITY_DESCRIPTOR_CONTROL sdc;
	DWORD sdrev;
	FXERRHWIN(GetSecurityDescriptorControl(sd, &sdc, &sdrev));
	/*if(sdc & SE_SELF_RELATIVE)
	{	// Unpack it
		DWORD absSize=0, daclSize=0, saclSize=0, ownerSize=0, groupSize=0;
		MakeAbsoluteSD(sd, 0, &absSize, 0, &daclSize, 0, &saclSize, 0, &ownerSize, 0, &groupSize);
		SECURITY_DESCRIPTOR *abssd;
		FXERRHM(abssd=(SECURITY_DESCRIPTOR *) LocalAlloc(LMEM_FIXED, absSize+daclSize+saclSize+ownerSize+groupSize));
		FXRBOp unabssd=FXRBFunc(&LocalFree, abssd);
		acl=(PACL) FXOFFSETPTR(abssd, absSize);
		PACL sacl=(PACL) FXOFFSETPTR(acl, daclSize);
		owner=(SID *) FXOFFSETPTR(sacl, saclSize);
		group=(SID *) FXOFFSETPTR(owner, ownerSize);
		FXERRHWIN(MakeAbsoluteSD(sd, abssd, &absSize, acl, &daclSize, sacl, &saclSize, owner, &ownerSize, group, &groupSize));
		FXERRHWIN(!LocalFree(sd));
		sd=abssd;
		unabssd.dismiss();
	}
	else*/
	{
		BOOL present, defaulted;
		FXERRHWIN(GetSecurityDescriptorDacl(sd, &present, &acl, &defaulted));
		FXERRHWIN(GetSecurityDescriptorOwner(sd, (PSID *) &owner, &defaulted));
		FXERRHWIN(GetSecurityDescriptorGroup(sd, (PSID *) &group, &defaulted));
	}
	FXACLEntity myowner;
	FXERRHM(myowner.p=new FXACLEntityPrivate(owner, group, FXString::nullStr()));
	FXERRHM(p=new FXACLPrivate(type, myowner));
	if(acl) p->readACL(acl);
#endif
}
FXACL::FXACL(const FXString &path, FXACL::EntityType type) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
#ifdef USE_WINAPI
	SECURITY_DESCRIPTOR *sd;
	LPSTR _path=(LPSTR) path.text();
	if(':'==path[1] && 3==path.length())
	{	/* Windows is funny and pops up various dialog boxes asking for removable media if
		the drive is empty. We don't want this, we actually want an exception. Unfortunately,
		SetErrorMode() is a bit flaky when the program is running in the debugger so here
		we make a special case and use the virtually undocumented way of using magic parameters
		to CreateFile(). */
		FXString temp("\\\\.\\"+path.left(2));
		HANDLE h;
		FXERRHWIN(INVALID_HANDLE_VALUE!=(h=CreateFile((LPSTR) temp.text(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)));
		FXRBOp unh=FXRBFunc(CloseHandle, h);
		DWORD read=0;
		DISK_GEOMETRY gms[20];
		DWORD ret=DeviceIoControl(h, IOCTL_DISK_GET_MEDIA_TYPES, NULL, 0, gms, sizeof(gms), &read, NULL);
		if(!ret || FixedMedia!=gms[0].MediaType)
		{	// Ask it if it has a disc in
			FXERRHWIN(DeviceIoControl(h, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, NULL, 0, &read, NULL));
		}
	}
	FXERRHWIN(ERROR_SUCCESS==GetNamedSecurityInfo(_path, mapType(type),
		DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION,
		0, 0, 0, NULL, (PSID *) &sd));
	init(sd, type);
#endif
#ifdef USE_POSIX
	struct stat s={0};
	FXERRHOS(stat(path.text(), &s));
	FXACLEntity myowner;
	FXERRHM(myowner.p=new FXACLEntityPrivate(s.st_uid, s.st_gid, false, FXString::nullStr()));
	FXERRHM(p=new FXACLPrivate(type, myowner));
	p->buildACL(s);
#endif
	unconstr.dismiss();
}
FXACL::FXACL(void *h, FXACL::EntityType type) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
#ifdef USE_WINAPI
	SECURITY_DESCRIPTOR *sd;
	FXERRHWIN(ERROR_SUCCESS==GetSecurityInfo((HANDLE) h, mapType(type),
		DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION,
		0, 0, 0, NULL, (PSID *) &sd));
	init(sd, type);
#endif
#ifdef USE_POSIX
	FXERRG("Not supported under POSIX", 0, FXERRH_ISDEBUG);
#endif
	unconstr.dismiss();
}
FXACL::FXACL(int fd, FXACL::EntityType type) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
#ifdef USE_WINAPI
	// Decode to HANDLE
	void *h=(void *) _get_osfhandle(fd);
	SECURITY_DESCRIPTOR *sd;
	FXERRHWIN(ERROR_SUCCESS==GetSecurityInfo((HANDLE) h, mapType(type),
		DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION,
		0, 0, 0, NULL, (PSID *) &sd));
	init(sd, type);
#endif
#ifdef USE_POSIX
	struct stat s={0};
	FXERRHOS(fstat(fd, &s));
	FXACLEntity myowner;
	FXERRHM(myowner.p=new FXACLEntityPrivate(s.st_uid, s.st_gid, false, FXString::nullStr()));
	FXERRHM(p=new FXACLPrivate(type, myowner));
	p->buildACL(s);
#endif
	unconstr.dismiss();
}
FXACL::FXACL(const FXACL &o) : p(0)
{
	if(o.p) { FXERRHM(p=new FXACLPrivate(*o.p)); }
}
FXACL &FXACL::operator=(const FXACL &o)
{
	if(o.p) *p=*o.p;
	return *this;
}
FXACL::~FXACL()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
bool FXACL::operator==(const FXACL &o) const
{
	return *p==*o.p;
}
bool FXACL::operator!=(const FXACL &o) const
{
	return *p!=*o.p;
}
FXACL::EntityType FXACL::type() const
{
	return p->type;
}
void FXACL::setType(FXACL::EntityType type)
{
	p->type=type;
	p->dirty=true;
}
bool FXACL::hasInherited() const
{
	return p->hasInherited;
}
void FXACL::setHasInherited(bool newval)
{
#ifdef USE_POSIX
	FXERRH(!newval, "ACLs can never inherit on POSIX", 0, FXERRH_ISDEBUG);
#endif
	p->hasInherited=newval;
}
FXuint FXACL::count() const
{
	return p->count();
}
void FXACL::insert(const FXACLIterator &it, const FXACL::Entry &entry)
{
#ifdef USE_POSIX
	FXERRH(entry.entity.p->amOwner || FXACLEntity::everything()==entry.entity, "ACL entries on POSIX need to be owner, owner group or everything", 0, FXERRH_ISDEBUG);
#endif
	p->insert(*it.p, entry);
	p->dirty=true;
}
void FXACL::remove(const FXACLIterator &it)
{
	p->remove(*it.p);
	p->dirty=true;
}
FXACLIterator FXACL::begin() const
{
	return FXACLIterator(*this, false);
}
FXACLIterator FXACL::end() const
{
	return FXACLIterator(*this, true);
}
void FXACL::prepend(const Entry &entry)
{
	insert(begin(), entry);
}
void FXACL::append(const Entry &entry)
{
	insert(end(), entry);
}
const FXACLEntity &FXACL::owner() const
{
	return p->owner;
}
void FXACL::setOwner(const FXACLEntity &entity)
{
	p->owner=entity;
}


extern bool FXACL_DenyAllNonTnCode;
bool FXACL_DenyAllNonTnCode;
bool FXACL::check(FXACL::Perms __what) const
{
	Permissions _what=__what;
	if(FXACL_DenyAllNonTnCode && !_what.amTn) return false;
#ifdef USE_WINAPI
	int_toWin32SecurityDescriptor();
	DWORD what=translate(_what, p->type);
	GENERIC_MAPPING gm={0};
	DWORD privsetlen=fxaclinit->pssize, granted=0;
	BOOL okay=FALSE;
	HANDLE token; //mytoken;
	/*if(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY|TOKEN_DUPLICATE, TRUE, &mytoken))
	{
		FXRBOp unmytoken=FXRBFunc(CloseHandle, mytoken);
		FXERRHWIN(DuplicateToken(mytoken, SecurityImpersonation, &token));
	}
	else*/
	{
		//if(ERROR_NO_TOKEN!=GetLastError()) { FXERRHWIN(0); }
		FXERRHWIN(DuplicateToken(fxaclinit->myprocessh, SecurityImpersonation, &token));
	}
	FXRBOp untoken=FXRBFunc(CloseHandle, token);
	FXERRHWIN(AccessCheck(p->sd, token, what, &gm, fxaclinit->ps, &privsetlen, &granted, &okay));
	if(okay) return true;
	else return false;
#endif
#ifdef USE_POSIX
	// If I'm root, I have all access to everything
	if(!getuid()) return true;
	for(QValueList<FXACL::Entry>::const_iterator it=p->begin(); it!=p->end(); ++it)
	{
		const FXACL::Entry &e=*it;
		if(e.entity.p->amGroup)
		{
			if(e.entity.p->amPublic)
			{
				if(_what & e.deny) return false;
			}
			else if((e.entity.p->amOwner && p->owner.p->groupId==getgid()) || e.entity.p->groupId==getgid())
			{
				if(_what & e.deny) return false;
			}
		}
		else
		{
			if((e.entity.p->amOwner && p->owner.p->userId==getuid()) || e.entity.p->userId==getuid())
			{
				if(_what & e.deny) return false;
			}
		}
	}
	for(QValueList<FXACL::Entry>::const_iterator it=p->begin(); it!=p->end(); ++it)
	{
		const FXACL::Entry &e=*it;
		if(e.entity.p->amGroup)
		{
			if(e.entity.p->amPublic)
			{
				if((_what & e.grant)==_what) return true;
			}
			else if((e.entity.p->amOwner && p->owner.p->groupId==getgid()) || e.entity.p->groupId==getgid())
			{
				if((_what & e.grant)==_what) return true;
			}
		}
		else
		{
			if((e.entity.p->amOwner && p->owner.p->userId==getuid()) || e.entity.p->userId==getuid())
			{
				if((_what & e.grant)==_what) return true;
			}
		}
	}
	return false;
#endif
}
void FXACL::checkE(FXACL::Perms what) const
{
	if(!check(what))
	{
		FXERRGNOPERM(FXTrans::tr("FXACL", "Permission denied"), 0);
	}
}

FXString FXACL::report() const
{
	FXString ret=FXTrans::tr("FXACL",	"Owner: %1 (group: %2)\n  Deny          Grant         Entity\n").arg(p->owner.asString()).arg(p->owner.group().asString());
	for(QValueList<FXACL::Entry>::iterator it=p->begin(); it!=p->end(); ++it)
	{
		FXACL::Entry &e=*it;
		ret+=FXString("  %1  %2  %3\n").arg(e.deny.asString(p->type), -12).arg(e.grant.asString(p->type), -12)
			.arg(e.entity.asString());
	}
	return ret;
}

void FXACL::writeTo(const FXString &path) const
{
#ifdef USE_WINAPI
	int_toWin32SecurityDescriptor();
	LPSTR _path=(LPSTR) path.text();
	SECURITY_INFORMATION si=DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION;
	if(p->hasInherited)
		si|=UNPROTECTED_DACL_SECURITY_INFORMATION;
	else
		si|=PROTECTED_DACL_SECURITY_INFORMATION;
	/* If we're setting the owner to not the current user, the call will fail
	unless a token for that owner is in the current thread */
	bool doImpers=FXACLEntity::currentUser()!=p->owner;
	if(doImpers)
	{
		FXERRH(p->owner.p->token, FXTrans::tr("FXACL", "You must authenticate an entity before you can set it as owner"), FXACL_OWNERNEEDSAUTH, 0);
		FXERRHWIN(SetThreadToken(NULL, p->owner.p->token));
	}
	DWORD ret=SetNamedSecurityInfo(_path, mapType(p->type), si, (PSID) p->owner.p->sid,
		(PSID) p->owner.p->group, p->acl, NULL);
	if(doImpers)
	{
		if(!RevertToSelf())
		{
			FXERRGWIN(GetLastError(), FXERRH_ISFATAL);
		}
	}
	FXERRHWIN(ERROR_SUCCESS==ret);
#endif
#ifdef USE_POSIX
	struct stat s={0};
	FXERRHOS(stat(path.text(), &s));
	FXACLEntity fileowner;
	FXERRHM(fileowner.p=new FXACLEntityPrivate(s.st_uid, s.st_gid, false, FXString::nullStr()));
	if(fileowner!=p->owner)
	{	// Try to change file owner (need superuser)
		FXERRHOS(chown(path.text(), p->owner.p->userId, p->owner.p->groupId));
	}
	FXERRHOS(chmod(path.text(), p->fromACL(fileowner, s)));
#endif
}
void FXACL::writeTo(void *h) const
{
#ifdef USE_WINAPI
	int_toWin32SecurityDescriptor();
	SECURITY_INFORMATION si=DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION;
	if(p->hasInherited)
		si|=UNPROTECTED_DACL_SECURITY_INFORMATION;
	else
		si|=PROTECTED_DACL_SECURITY_INFORMATION;
	/* If we're setting the owner to not the current user, the call will fail
	unless a token for that owner is in the current thread */
	bool doImpers=FXACLEntity::currentUser()!=p->owner;
	if(doImpers)
	{
		FXERRH(p->owner.p->token, FXTrans::tr("FXACL", "You must authenticate an entity before you can set it as owner"), FXACL_OWNERNEEDSAUTH, 0);
		FXERRHWIN(SetThreadToken(NULL, p->owner.p->token));
	}
	DWORD ret=SetSecurityInfo((HANDLE) h, mapType(p->type), si, (PSID) p->owner.p->sid,
		(PSID) p->owner.p->group, p->acl, NULL);
	if(doImpers)
	{
		if(!RevertToSelf())
		{
			FXERRGWIN(GetLastError(), FXERRH_ISFATAL);
		}
	}
	FXERRHWIN(ERROR_SUCCESS==ret);
#endif
#ifdef USE_POSIX
	FXERRG("Not supported under POSIX", 0, FXERRH_ISDEBUG);
#endif
}
void FXACL::writeTo(int fd) const
{
#ifdef USE_WINAPI
	// Decode to HANDLE
	HANDLE h=(HANDLE) _get_osfhandle(fd);
	writeTo((void *) h);
#endif
#ifdef USE_POSIX
	struct stat s={0};
	FXERRHOS(fstat(fd, &s));
	FXACLEntity fileowner;
	FXERRHM(fileowner.p=new FXACLEntityPrivate(s.st_uid, s.st_gid, false, FXString::nullStr()));
	if(fileowner!=p->owner)
	{	// Try to change file owner (need superuser)
		FXERRHOS(fchown(fd, p->owner.p->userId, p->owner.p->groupId));
	}
	FXERRHOS(fchmod(fd, p->fromACL(fileowner, s)));
#endif
}

FXACL FXACL::default_(FXACL::EntityType type, int flags)
{
	FXACL ret(type);
	ret.append(Entry(FXACLEntity::owner(), 0, Permissions().setAll((flags & 4)!=0)));
	if(flags & 2)
		ret.append(Entry(FXACLEntity::owner().group(), 0, Permissions().setGenRead().setGenWrite().setGenExecute((flags & 4)!=0)));
	else if(flags & 1)
		ret.append(Entry(FXACLEntity::owner().group(), 0, Permissions().setGenRead().setGenExecute((flags & 4)!=0)));
	return ret;
}

FXACL::ACLSupport FXACL::hostOSACLSupport()
{
	ACLSupport ret;
	ret.perOwnerGroup=true;		// Both POSIX and NT have this
#ifdef USE_WINAPI
	ret.perEntity=true;			// NT has this
	ret.hasInheritance=true;	// Win2k+ has this
#endif
	return ret;
}

void *FXACL::int_toWin32SecurityDescriptor() const
{
#ifdef USE_WINAPI
	if(p->dirty)
	{
		free(p->sd);
		p->sd=0;
	}
	if(!p->sd)
	{
		FXERRHM(p->sd=(SECURITY_DESCRIPTOR *) malloc(SECURITY_DESCRIPTOR_MIN_LENGTH));
		FXERRHWIN(InitializeSecurityDescriptor(p->sd, SECURITY_DESCRIPTOR_REVISION));
		if(!p->isEmpty())
		{
			if(p->acl)
			{
				free(p->acl);
				p->acl=0;
			}
			DWORD acllen=sizeof(ACL);
			for(QValueList<FXACL::Entry>::iterator it=p->begin(); it!=p->end(); ++it)
			{
				FXACL::Entry &e=*it;
				if(e.deny)
				{
					acllen+=sizeof(ACCESS_DENIED_ACE)-sizeof(DWORD)+GetLengthSid(e.entity.p->sid);
				}
				if(e.grant)
				{
					acllen+=sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+GetLengthSid(e.entity.p->sid);
				}
			}
			FXERRHM(p->acl=(ACL *) malloc(acllen));
			FXERRHWIN(InitializeAcl(p->acl, acllen, ACL_REVISION));
			for(QValueList<FXACL::Entry>::iterator it=p->begin(); it!=p->end(); ++it)
			{
				FXACL::Entry &e=*it;
				if(e.deny)
				{
					DWORD flags=e.inheritable ? CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE : 0;
					if(e.inherited) flags|=INHERITED_ACE;
					FXERRHWIN(AddAccessDeniedAceEx(p->acl, ACL_REVISION, flags, translate(e.deny, p->type), e.entity.p->sid));
				}
			}
			for(QValueList<FXACL::Entry>::iterator it=p->begin(); it!=p->end(); ++it)
			{
				FXACL::Entry &e=*it;
				if(e.grant)
				{
					DWORD flags=e.inheritable ? CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE : 0;
					if(e.inherited) flags|=INHERITED_ACE;
					FXERRHWIN(AddAccessAllowedAceEx(p->acl, ACL_REVISION, flags, translate(e.grant, p->type), e.entity.p->sid));
				}
			}
			FXERRHWIN(SetSecurityDescriptorDacl(p->sd, TRUE, p->acl, FALSE)); 
		}
		FXERRHWIN(SetSecurityDescriptorOwner(p->sd, p->owner.p->sid, FALSE));
		FXERRHWIN(SetSecurityDescriptorGroup(p->sd, p->owner.p->group, FALSE));
		assert(IsValidSecurityDescriptor(p->sd));
		p->dirty=false;
	}
	return p->sd;
#endif
#ifdef USE_POSIX
	return 0;
#endif
}



FXACLIterator::FXACLIterator(const FXACL &acl, bool end) : p(0)
{
	FXERRHM(p=new FXACLIteratorPrivate(const_cast<FXACL &>(acl), *acl.p, end));
}
FXACLIterator::FXACLIterator(const FXACLIterator &o) : p(0)
{
	FXERRHM(p=new FXACLIteratorPrivate(*o.p));
}
FXACLIterator::~FXACLIterator()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
FXACLIterator &FXACLIterator::operator=(const FXACLIterator &o)
{
	*p=*o.p;
	return *this;
}
bool FXACLIterator::operator==(const FXACLIterator &o) const
{
	return *p==*o.p;
}
bool FXACLIterator::operator!=(const FXACLIterator &o) const
{
	return *p!=*o.p;
}
const FXACL::Entry &FXACLIterator::operator *() const
{
	return *(*p);
}
const FXACL::Entry *FXACLIterator::operator->() const
{
	return &(*(*p));
}
FXACLIterator &FXACLIterator::operator++()
{
	++(*p);
	return *this;
}
FXACLIterator &FXACLIterator::operator+=(FXuint i)
{
	while(i-- && *p!=p->list.p->end()) ++(*p);
	return *this;
}
FXACLIterator &FXACLIterator::operator--()
{
	--(*p);
	return *this;
}
FXACLIterator &FXACLIterator::operator-=(FXuint i)
{
	while(i++ && *p!=p->list.p->begin()) --(*p);
	return *this;
}


} // namespace

