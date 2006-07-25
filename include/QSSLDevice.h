/********************************************************************************
*                                                                               *
*                           Data Encryption Support                             *
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

#ifndef QSSLDEVICE_H
#define QSSLDEVICE_H

#include "QIODeviceS.h"
#include "FXString.h"
#include "FXSecure.h"

namespace FX {

/*! \file QSSLDevice.h
\brief Defines things used in providing secure data transport
*/

class FXStream;
class QSSLDevice;

/*! \class FXSSLPKey
\ingroup security
\brief A container holding a variable length asymmetric encryption key

This is a generic container of an asymmetric encryption key used
with FX::QSSLDevice and FX::FXSSLKey which can be dumped to and from
storage. Its internal state resides within the FX::Secure namespace
and thus is automatically zeroed on deletion.

Currently the only supported key format is RSA. The somewhat tricky
implementation details are abstracted away from you so even if new
formats were to be added in the future, there would be almost no
code changes.

Support for public/private key interoperability with PGP, SSH or Apache is provided by
readFromPEM() and writeToPEM() which use the standard PEM format
without encryption. You should even be able to give these routines
X509 certificates without a problem.

Saving and loading saves the full key (both private and public parts).
This is because a key takes up so little space you might as well save
regenerating the public part of a key. Obviously, the storage of
private keys should be secure. If you want to save just the public or
private part alone, hatch it off using publicKey() and privateKey() and
save just that part.

\note To prevent accidental sending of the private part in an IPC msg,
QSSLDevice will not permit encryption using a FXSSLKey containing a
private part. This should help you catch accidental programming errors
which would severely impact security.

\note If you are dealing with a party who you cannot trust (ie; non-
local software), run verify() on any FXSSLPKey's you receive from
a third party before use.

<h4>File format:</h4>
+0: Key size in bits<br>
+4: Key type (the enum)<br>
+6: "PUB1" or "PRV1" denoting public or private key respectively<br>
+10: length of key in bytes<br>
+14: Key data in <b>big-endian</b> format<br>
+n: next tag, or "ENDK" for no more<br>
*/
class FXSSLPKeyPrivate;
class FXAPI FXSSLPKey
{
	friend class QSSLDevice;
	FXSSLPKeyPrivate *p;
public:
	//! Types of asymmetric key
	enum KeyType
	{
		NoEncryption=0,	//!< No encryption is performed
		RSA,			//!< A RSA key to any bit length
		DH				//!< Unsupported at present
	};
	/*! Constructs an asymmetric key of size \em bitsize and type \em type. Calls
	generate() if type has been set */
	explicit FXSSLPKey(FXuint bitsize=0, KeyType type=NoEncryption);
	~FXSSLPKey();
	FXSSLPKey(const FXSSLPKey &other);
	FXSSLPKey &operator=(const FXSSLPKey &other);
	//! Returns true if the two keys are the same
	bool operator==(const FXSSLPKey &other) const;
	//! Returns true if the two keys are not the same
	bool operator!=(const FXSSLPKey &other) const { return !(*this==other); }
	//! Returns true if this key is smaller than the other
	bool operator<(const FXSSLPKey &other) const;
	//! Returns true if this key is bigger than the other
	bool operator>(const FXSSLPKey &other) const { return !(*this<other); }
	//! Returns the type of the key
	KeyType type() const throw();
	//! Sets the type of the key, clearing any key data if the type has changed
	void setType(KeyType type);
	//! Returns the type as a string
	FXString typeAsString() const;
	//! Returns the length of the key in bytes
	FXuint bytesLen() const throw();
	//! Returns the length of the key in bits
	FXuint bitsLen() const throw();
	//! Sets the length of the key in bits
	void setBitsLen(FXuint newsize);
	//! Returns true if this key has a public part
	bool hasPublicKey() const throw();
	//! Returns the public part of the key
	FXSSLPKey publicKey() const;
	//! Returns the public part of the key as a hexadecimal string
	FXString publicKeyAsString() const;
	//! Returns a hash of the public part of the key
	Secure::TigerHashValue publicKeyAsHash() const;
	//! Returns true if this key has a private part
	bool hasPrivateKey() const throw();
	//! Returns the private part of the key
	FXSSLPKey privateKey() const;
	//! Generates a new unique key
	void generate();
	//! Returns true if the public and private parts of the asymmetric key correspond
	bool verify() const;
	//! Reads a public or private key in from PEM format data
	void readFromPEM(QIODevice *dev);
	//! Writes the public or private key out in PEM format
	void writeAsPEM(QIODevice *dev) const;

	//! Creates a public key from a previously created string by FX::FXSSLPKey::publicKeyAsString()
	static FXSSLPKey publicKeyFromString(const FXString &s, KeyType type);
	friend FXAPI FXStream &operator<<(FXStream &s, const FXSSLPKey &i);
	friend FXAPI FXStream &operator>>(FXStream &s, FXSSLPKey &i);
};
//! Writes the asymmetric key to stream \em s
FXAPI FXStream &operator<<(FXStream &s, const FXSSLPKey &i);
//! Reads an asymmetric key from stream \em s
FXAPI FXStream &operator>>(FXStream &s, FXSSLPKey &i);

/*! \class FXSSLKey
\brief A container holding a variable length symmetric encryption key

This is a generic container of a symmetric encryption key used with FX::QSSLDevice
which can be dumped to and from storage. Its internal state resides
within the secure heap and thus is automatically zeroed on deletion.

Furthermore you can optionally choose to encrypt your symmetric key with
asymmetric encryption. This means that data written out by FX::QSSLDevice
actually contains the very key also needed to decrypt it but obviously,
in order to decrypt the decryption key you need the private key part of
the asymmetric pair. Asymmetric keys are stored in a FX::FXSSLPKey.

Supported key formats are Blowfish & AES for symmetric encryption.
Most of the implementation details in using
the different key formats are abstracted away from you so you can treat
them in a generic fashion.

If you generate a symmetric key from a piece of text using generateFromText()
(eg; a human memorable piece of text), you should be aware that this reduces
the possibilities in a brute
strength attack to only around 80^len where len is the password length
and assuming the password contains a mixture of alphanumeric letters,
mixed capitalisation and numbers. Thus a six letter password is only
262 billion possibilities which is nothing. An eight letter password
is somewhat better with 1677 trillion. To give you some idea, my
home computer can attempt several million passwords per minute which
really means you need a twelve letter password or higher to be sure.
Obviously government security services could crunch that in minutes.
To make it slightly more difficult for them, you can add random salt
to your key using setSaltLen(n) though be careful as 2^n extra
key tests need to be made at the time of decryption. I've made this
much quicker by incorporating a Tiger hash of the key (which is also
salted with 16 extra bits) but 2^n * 65536*O(hash) can quickly become
slow. generateFromText() internally sets a salt length of 8 bits.

\note Salting has been implemented by EORing in a length of random
data equal to the salt length into the key generated from the plaintext.
Of course, you can add salt to normal randomly generated keys for
extra protection.

The maximum key length which can be generated from a piece of plaintext
is 480 bits. The first 192 bits is generated using the Tiger hash
algorithm (FX::Secure::TigerHash), the next 160 bits is generated using
SHA-1 and the next 128 bits from MD5. Thus key generation up to 192 bits
is the quickest. An optional number of rounds can be used whereby the
hashing process is reapplied to the previous key so many times - this
slows down attackers. The default is 65536 and because of each hash's
results feeding into the other hash functions (where key size is greater
than 192), it should really mean a substantial increase in effort. Always
use a minimum of two if bitsize > 192 otherwise attack becomes as easy
as the weakest hash.

\warning Don't keep your keys in an unencrypted state for any longer
than you have to ie; in memory. Keys are actually more important than
the data you're keeping safe as you'll tend to use one key for all your
data.

<h4>File format:</h4>
+0: Key type (the enum). 0xffff if key is encrypted.<br>
+2: Key size in bytes (if encrypted, then pkey.bytesLen())<br>
If key is not encrypted:
   +6: Key size in bits<br>
  +10: Key salt length in bits<br>
+6|+14: Key data in <b>big-endian</b> byte order
*/
class FXSSLKeyPrivate;
class FXAPI FXSSLKey
{
	friend class QSSLDevice;
	FXSSLKeyPrivate *p;
public:
	//! Types of symmetric key
	enum KeyType
	{
		NoEncryption=0,	//!< No encryption is performed OR type will be set by private key decryption
		Blowfish,		//!< A Blowfish key between 64 and 448 bits which must be a multiple of 64
		AES,			//!< An AES key of 128, 192 or 256 bits
		Encrypted=0xffff //!< A key encrypted by a FX::FXSSLPKey
	};
	/*! Constructs a new encryption key of type \em type and from plaintext \em text. If
	text is null, generates a cryptographically sound random key of the specified type
	(generate()) otherwise uses generateFromText()
	*/
	explicit FXSSLKey(FXuint bitsize=0, KeyType type=NoEncryption, const FXString &text=(const char *) 0);
	~FXSSLKey();
	FXSSLKey(const FXSSLKey &other);
	FXSSLKey &operator=(const FXSSLKey &other);
	//! Returns true if the two keys are the same
	bool operator==(const FXSSLKey &other) const;
	//! Returns true if the two keys are not the same
	bool operator!=(const FXSSLKey &other) const { return !(*this==other); }
	//! Returns true if this key is smaller than the other one
	bool operator<(const FXSSLKey &other) const;
	//! Returns true if this key is bigger than the other one
	bool operator>(const FXSSLKey &other) const { return !(*this<other); }
	//! Returns the type of the key
	KeyType type() const throw();
	//! Sets the type of the key, clearing any key data if the type has changed
	void setType(KeyType type);
	//! Returns the type as a string
	FXString typeAsString() const;
	//! Returns the amount of salt this key will have
	FXuint saltLen() const throw();
	//! Sets the amount of salt this key will have. Must be 64 or less and less than or equal to the key length and a multiple of eight.
	void setSaltLen(FXuint salt);
	//! Returns the asymmetric key encrypting this key. Is zero if none.
	FXSSLPKey *asymmetricKey() const throw();
	//! Sets the asymmetric key encrypting this key (0 for no key). A copy is made. Returns \c *this.
	FXSSLKey &setAsymmetricKey(const FXSSLPKey *pkey);
	//! Returns the length of the key in bytes
	FXuint bytesLen() const throw();
	//! Returns the length of the key in bits
	FXuint bitsLen() const throw();
	//! Sets the length of the key in bits. Must be a multiple of eight.
	void setBitsLen(FXuint newsize);
	//! Generates a new unique key
	void generate();
	/*! Generates a key from a piece of plaintext. Use with caution and see the notes above.
	\note Automatically sets the salt length to 16
	*/
	void generateFromText(const FXString &text, int rounds=65536);
	//! Returns a Tiger hash of the key
	Secure::TigerHashValue hash() const throw();

	friend FXAPI FXStream &operator<<(FXStream &s, const FXSSLKey &i);
	friend FXAPI FXStream &operator>>(FXStream &s, FXSSLKey &i);
};
//! Writes the symmetric key to stream \em s. Does not write its asymmetric key if it has one.
FXAPI FXStream &operator<<(FXStream &s, const FXSSLKey &i);
//! Reads a symmetric key from stream \em s. Does not alter any existing asymmetric key.
FXAPI FXStream &operator>>(FXStream &s, FXSSLKey &i);

/*! \class QSSLDevice
\ingroup security
\ingroup fiodevices
\ingroup siodevices
\brief An i/o device encrypting what goes through it

Thanks to the wonderful flexibility of the OpenSSL encryption library,
TnFOX can offer completely integrated encryption facilities which can
work with any FX::QIODevice. While the most common use will be with
FX::QBlkSocket, you could just as easily apply it to file data or
indeed anything else. Synchronous devices work the full SSL/TLS
negotiation protocol whereas file devices simply apply symmetric
or asymmetric encryption based on the FX::FXSSLKey you provide.

QSSLDevice offers SSL v2/v3 & TLS v1 protocols with RC2, RC4, DES, Blowfish,
IDEA, 3DES & AES symmetric encryption and RSA asymmetric encryption
(ie; public-key) to any bit length. Furthermore RSA, Diffie-Hellman and
DSS authentication methods are available. Within these, you have the
strongest available encryption currently known.

Unfortunately usage of strong encryption in certain ways is illegal
in many illiberal countries (such as the US and some countries in
Europe). Some countries won't let you export it, many limit the
maximum key length, most require key escrow (where your private key
can be obtained by court order and failure to comply results in
prison) and in some usage is completely illegal altogether. Where
things get even more fun is that you can do something with encryption
legal in your country of residence but if you take a holiday to the
US say, there they can put you in prison for a very long time.

\warning <b>TnFOX, its makers or anyone else do not take responsibility
for YOU breaking the law in your country or any other country. TnFOX
merely provides the facilities, it is YOU who chooses how to use them.
If you do not accept that you take full responsibility for how you
use this facility, then you cannot use QSSLDevice
nor compile in OpenSSL support!</b>

More fun again is that various encryption algorithms offered by
QSSLDevice are patented in some countries but not in others. The
following is a \b non-definitive list and no responsibility is
taken for the list being remotely accurate:
<table>
<tr><th>Algorithm</th><th>Legal State</th><th>Where</th>
<tr><td>SSL itself</td><td>Royalty free unless you sue Netscape</td><td>?</td>
<tr><td>DES</td><td>Royalty free</td><td>?</td>
<tr><td>3DES</td><td>Royalty free</td><td>?</td>
<tr><td>Blowfish</td><td>Unpatented</td><td>Everywhere</td>
<tr><td>IDEA</td><td>Patented till 2011</td><td>USA,Europe</td>
<tr><td>AES</td><td>Royalty free?</td><td>Everywhere?</td>
<tr><td>RC5</td><td>Patented</td><td>USA,Japan,Europe in 2003</td>
<tr><td></td><td></td><td></td>
<tr><td>RSA</td><td>Unpatented</td><td>Patent expired in 2000</td>
<tr><td>Diffie-Hellman</td><td>Patent expired in 1997?</td><td>?</td>
<tr><td>DSA/DSS</td><td>Royalty free</td><td>Everywhere</td>
</table>
\warning Again, YOU are responsible for ensuring that your
software using TnFOX is not breaking patents

If you want a good overview of cryptography, see
http://home.ecn.ab.ca/~jsavard/crypto/intro.htm
or the book "Applied Cryptography" by Bruce Schneier.

<h3>Usage:</h3>
The first QSSLDevice created in the process will take the longest
as required data structures are cached in-memory and the random
number generator seeded with 4096 bits of randomness. Like most
FX::QIODevice's, QSSLDevice is thread-safe[1] though FX::FXSSLKey is not.
If the OpenSSL library was not available at compile-time, the first
QSSLDevice created throws an exception of code QSSLDEVICE_NOTENABLED.

At the time of writing (August 2003), the minimum symmetric
encryption key length should be 128 bits and the minimum asymmetric
encryption key length should be 2048 to 3072 bits. If your local
legal situation permits it, higher is better though bear in mind
that with asymmetric encryption especially, bigger keys means
substantial increases in encoding and decoding time.

<table>
<tr><th>Algorithm</th><th>Strength</th><th>Notes</th>
<tr><td>DES</td><td>56 bit</td><td>You don't want to use this except for legacy applications</td>
<tr><td>3DES</td><td>112 bit</td><td>Triple DES is 168 bit but due to a weakness is effectively 112 bit<br>It's also very slow thus probably best to avoid this too</td>
<tr><td>Blowfish</td><td>32-448</td><td>Especially useful for file encryption as it's fast</td>
<tr><td>IDEA</td><td>128</td><td>Patented for commercial use</td>
<tr><td>AES</td><td>128/192/256</td><td>The official US government symmetric encryption standard</td>
<tr><td></td><td></td><td></td>
<tr><td>RSA</td><td>512+</td><td>The traditional asymmetric algorithm</td>
<tr><td>DH (Diffie-Hellman)</td><td>512+</td><td>The default for non-authenticated connections</td>
<tr><td>DSA/DSS</td><td>512+</td><td>Used only for authentication</td>
</table>

Furthermore there are two hashing algorithms available: MD5 and SHA1. MD5 has a collision
weakness so use SHA1 where possible. SHA1 was formerly the US government standard cryptographic
hash.

The default settings create a SSL v3 & TLS only capable device
with ciphers set to \c "HIGH:@STRENGTH" ie; only the strongest
encryption ordered by strength (which are currently SHA1 with AES(256)
or 3DES and RSA or DH) but \b with non-authenticated protocols (ie;
certificates are optional). These settings aren't very compatible with most
existing servers on the internet so using setCiphers() you may
wish to set \c "DEFAULT:@STRENGTH" (all less non-authenticated protocols) or
\c "ALL:@STRENGTH" (all protocols including non-authenticated). See the OpenSSL
documentation for example cipher strings and how they should be formatted.

There is some basic support for certificates. You can specify a private
key file and certificate file using setPrivateKeyFile() and
setCertificateFile() plus you can compare FX::FXNetwork::dnsReverseLookup()
with peerHostNameByCertificate(). All files must be in PEM/X509 format.

<h4>Compression:</h4>
All encryption methods are weaker if the data they are encrypting has
something known about it eg; it is HTML or ASCII text. Another source
of weakness is verbosity - the more data there is to analyse, the easier
to crack. The simple solution to this is to apply Lempel-Ziv compression
on the data before encrypting using something like FX::QGZipDevice.
See the examples below.

<h4>Randomness:</h4>
FX::Secure::Randomness is a high quality source of randomness and thus
can take a very long time to generate 4096 bits. If less than 4096 bits
is available then to prevent applications just sitting around waiting
for it, QSSLDevice uses a
number of sources of instantaneous entropy to seed the cryptographically
secure Pseudo-Random Number Generator (PNRG):
\li On Linux, \c /dev/urandom (FX::Secure::Randomness uses \c /dev/random)
\li On Windows, the current screen contents; a full list of of heaps,
processes, threads and DLL's currently running; the current foreground
window handle; the global memory state; the current process id; the
CryptoAPI's PRNG and the Intel processor's PRNG.

Thereafter with every new QSSLDevice created, up to 4096 bits of
entropy is added from FX::Secure::Randomness to keep the PRNG fresh.
Every new FXSSLKey or FXSSLPKey also adds up to its bitsize of entropy
to the PRNG.

<h4>Synchronous usage:</h4>
It is best to use TLS v1 (which is SSL v3.1), then SSL v3 and lastly
SSL v2 only if you absolutely have to due to compatibility reasons.
Because SSL v2 has security issues, by default it is disabled though
you can still connect to SSL v2 servers (you can change this in
the constructor parameters). If you enable everything, a negotiation
procedure is followed at the start of the connection to agree the best
available protocol.

If you don't bother with certificates, the default settings of
QSSLDevice use anonymous Diffie-Hellman for key exchange using an
internal list of primes for generation of unique keys.
A new key is generated per new connection. Obviously this mechanism
is liable to man-in-the-middle attack (ie; you can't be sure who you're
connecting to is actually who you think), but if you merely want your
data going over the wire to be unintelligible, this is very sufficient.

\note The embedded lists of primes currently only include 4096 bit
and 1024 bit primes. Asking for other bit lengths incurs a \em very
significant time penalty.

If speed is more important than security, consider 128 bit AES with
1024 bit key exchange. This is because 128 bit AES is faster than
most other similar strength block-ciphers. It is considerably faster
than plain DES and vastly faster than 3DES. AES is also faster than any other
for small packet transfers (smaller than 1024 bytes) so bear this in mind.

[1]: OpenSSL itself is only partially threadsafe - in particular, it is not
threadsafe when multiple threads use a SSL connection at the same time (which
unfortunately TnFOX requires as this is what the synchronous i/o model requires).
QSSLDevice sets some conservative options in OpenSSL to prevent packet
fragmentation (which would cause reads during writes or writes during reads) and
also serialises all reads and writes ie; only one read and write may happen
at once - but both a read and a write concurrently. This appears to be safe
from testing, but internal changes to OpenSSL may cause future breakage.

<h4>File-based usage:</h4>
Most of the focus so far in this documentation has been for encrypting
communications. However, if you have some data to which you want to
FXRESTRICT access, QSSLDevice can also apply straight off symmetric
or asymmetric encryption to raw data.

Asymmetric encryption has been implemented as symmetric encryption
but encrypting the symmetric key with asymmetric encryption and embedding
it with the data. This makes things substantially easier never mind
improving performance. You typically use asymmetric encryption if your
connection to the destination of the data is insecure (eg; internet,
postal mail, telephone etc) whereby your recipient generates a
RSA public/private key pair and sends you the
public part as a PEM or X509 format file. Read that into a FX::FXSSLPKey
and write out your data. Now only your intended recipient can read the
data.

For symmetric encryption, your two likely choices for ciphers are
Blowfish and AES. Both are relatively fast (~40Mb/sec on my machine)
and both scale well with key bit size (AES is 40% slower with 256 bit
keys than 128 bit). The other ciphers have been left out as they have
known weaknesses or are patented.

Strongly consider setting the FX::QIODeviceFlags::IO_ShredTruncate
bit when opening any secure file.

\note Due to limitations within the OpenSSL library, on 32 bit systems data
greater than 2Gb cannot be worked with. Attempting to do so causes
undefined operation. This problem goes away on 64 bit systems.

As of v0.86, support for seeking, mixing reads & writes and truncating has been added.
This comes at the cost of no longer being able to use Cipher-Block Chaining
(CBC) mode (as previous versions did) as you'd need to know all the data up
to the seek point. A similar FXRESTRICTion would obviously apply to Cipher
Feedback (CFB) mode, so that leaves us with Output Feedback (OFB) mode or
Counter (CTR) mode. Counter mode is really Electronic Codebook (ECB) with
the input as a combination of nonce and counter with the output XORed with
the plaintext to generate the ciphertext.

Now OFB and CTR modes are weaker than CFB and CBC as the plaintext has no
effect on the encryption - it is determined entirely by starting conditions.
CTR mode has the advantage of instant seeks whereas OFB must be iterated
from beginning to the seek point on each seek - so I have opted for CTR
mode despite that it is probably slightly weaker.

Performance-wise, throughput is heavily dependent on the speed at which
your processor can XOR portions of memory. There is an SSE2 instruction for
XORing 16 bytes at a time, but it requires 16 byte aligned memory which
is highly unlikely in general purpose usage. This implementation does
make use of memory alignment up to eight (for which the src, dest AND
offset inside encrypted stream buffer must all be multiples of eight) but
profiling shows that even that is rarely used compared against the four
byte aligned XOR on 32 bit systems. If however you are using a cipher with
a large (>16) block size, you could see substantial speed increases if you always
pass a buffer to readBlock() and writeBlock() which is eight byte aligned.


<h4>Usage:</h4>
Usage is as with all things in TnFOX, ridiculously easy:

Communication-type use (synchronous):
\code
QBlkSocket myserversocket;
...
QBlkSocket newsocketraw=myserversocket.waitForConnection();
// Perhaps spin off a new thread, or authenticate using FX::FXSRP first
// You will need a try...catch() block as negotiation may fail
QSSLDevice newsocket(&newsocketraw);
newsocket.create(newsocketraw.mode());
newsocket.read(NULL, 0);	// Just negotiate, don't read
if(newsocket.peerHostNameByCertificate()!=FXNetwork::dnsReverseLookup(newsocketraw.peerAddress())) reject;
...
\endcode

File-type use:
\code
QMemMap myfileraw("myencryptedfile.txt");
QSSLDevice myfile(&myfileraw);
// Get password from user into FXString mypassword
myfile.setKey(FXSSLKey(352, FXSSLKey::Blowfish, mypassword));
myfile.open(IO_ReadOnly);
// Read what you like as normal ...
\endcode

\code
QMemMap myencryptedfile("myencryptedfile.bin");
QSSLDevice myfileencryptor(&myencryptedfile);
myfileencryptor.setKey(thekey);
QGZipDevice myfilecompressor(&myfileencryptor);
QIODevice *myfile=&myfilecompressor;
FXStream s(myfile);
myfile->open(IO_WriteOnly);
s << "Some text to compress, then encrypt, then write to a memory mapped file";
myfile->close();
\endcode

Just especially as a note to myself who keeps forgetting how I'm supposed to use
asymmetric encryption, here's how you do it:
\code
FXSSLPKey &thekey;
FXSSLPKey pthekey(thekey.publicKey());
QMemMap myencryptedfile("myencryptedfile.bin");
QSSLDevice myfileencryptor(&myencryptedfile);
FXSSLKey tempkey(128, FXSSLKey::AES);
tempkey.setAsymmetricKey(&pthekey);
myfileencryptor.setKey(tempkey);
QGZipDevice myfilecompressor(&myfileencryptor);
QIODevice *myfile=&myfilecompressor;
FXStream s(myfile);
myfile->open(IO_WriteOnly);
s << "Some text to compress, then encrypt, then write to a memory mapped file";
myfile->close();
\endcode
To decrypt using asymmetric encryption, indirect via a temporary symmetric key like so:
\code
FXSSLPKey &thekey;
QSSLDevice &myfileencryptor;
myfileencryptor.setKey(FXSSLKey().setAsymmetricKey(&thekey));
...
\endcode

<h4>File formats:</h4>
QSSLDevice uses a proprietary file format for its secure files.
Sorry about this, I did look at the OpenPGP file format and
concluded it was too much hassle. I just wanted a basic secure
file format with no bells or whistles.

\warning This file format by me may contain weaknesses as a result of
my inexperience with cryptography. If you are an expert and see one,
please notify me. I have built in versioning to allow seamless upgrades.

+0: "TNFXSECD"<br>
+8: File version, currently 2<br>
+9: If "SKEY" then a FX::FXSSLKey but with the key data encrypted by the
public part of its asymmetric key. The encryption is done by binding the
key data (in big-endian format), the bitsize (little-endian 4 bytes), the
salt length (little-endian 4 bytes) and the type (little-endian 2 bytes)
together and asymmetrically encrypting<br>
+9[+skeylen]: "TEST" then a 192 bit Tiger hash of the key with 16
bits of salt if salting on the key wasn't used (used to test for bad keys)<br>
+37[+skeylen]: A random nonce of the same size as the cipher's block size<br>
+37+noncelen[+skeylen]: The encrypted data, encrypted by XORing original data
with the output of the encryption of the nonce XORed by the file pointer (CTR mode)

<b>Cryptoanalysis:</b> As I previously mentioned, I've never touched cryptography
before writing this class and while I have learned lots in the past few weeks,
I cannot say I am experienced. What I have done is work on the basis that the
less information the attacker has, the better. I've also used what the internet
says is best practice though I really need the book by Bruce Schneier (if I
had the money, I would). Below I present an analysis of my file format to
aid others in finding any weaknesses I may have introduced:

An attacker can not know from the file format:
\li The size or type of the symmetric key used to encrypt it
\li The exact size of the original data
\li Whether the key was generated from a password or not
\li If either the same key was used for more than one file or if the
contents of two files are identical
\li Ample sanity checks are used so even malicious altering of the file structure
will not help you.

Known weaknesses:
\li Altering the encrypted data alters the same portion of the decrypted
data - in particular, bitflipping the encrypted bitflips the decrypted.
If you know the format of the original data, this can be used effectively
as an attack, though you still need to know the key length as the encrypted
data is offset by that amount.
\li You can calculate the input data to the cipher. This enables certain methods
of attack.

However an attacker \em can know the following:
\li The size of the public key used to encrypt the symmetric key
\li A hash of the symmetric key, but this is never the same for the same key

Therefore it seemed to me that the weakest point was the hash as a brute force
strength attack could be used and the Tiger hash function is much quicker
than a decrypt - though this would still require the attacker
to know which cipher (there are only two available) and its key length.
Thus I introduced a 16 bit salt appended to the key before hashing which
means for something encrypted by a 128 bit key, an average of 2^(128-1+16)*O(hash)
is needed, or 10 with forty-two zeros after it Tiger hashs. I figure
that 32768 Tiger hashs are probably slower than a single iteration of
an attack on the cipher.

This salting of the hash is disabled however when salting of the key is
enabled because of performance reasons. Salting of the key is performed
by EORing in random data prior to encryption and up to eight bytes may
be used (ie; 64 bits). Key salting should be enabled when the entropic
quality of the key is low eg; if it is generated from plaintext.

For keys generated from plaintext, by default the hashing function(s) in
FXSSLKey::generateFromText() is run 65536 times and 16 bits of salting set.
This means an average of (80^passwordlen)*65536*O(pwhash)*65536*O(hash)/2
which with 128 bit key and a seven character password is 9 with twenty-two
zeros after it Tiger hashs. If a straight off attack were used because
of the unsalted hash, that would be 17 with thirty-seven zeros after it
Tiger hashs - basically, we sacrifice brute-force attack strength for
strength in key generation from plaintext. Also of course attacking the
cipher directly is made much harder due to greater key entropy.

<h3>Acknowledgements:</h3>
This class uses the excellent OpenSSL library developed by Eric Young,
Tim Hudson & the OpenSSL team. Since this library does not include
any actual OpenSSL code, TnFOX does not need to state:
\code
This product includes software developed by the OpenSSL Project
for use in the OpenSSL Toolkit. (http://www.openssl.org/)
\endcode
... but if OpenSSL support is present in the build of the library you
use in your end product, then \b you must place the above notice
in all advertising of your product as per the OpenSSL license. <i>This
is irrespective of whether you use QSSLDevice or not!</i>
*/
struct QSSLDevicePrivate;
class FXAPIR QSSLDevice : public QIODeviceS
{
	QSSLDevicePrivate *p;
	QSSLDevice(const QSSLDevice &);
	QSSLDevice &operator=(const QSSLDevice &);
	virtual FXDLLLOCAL void *int_getOSHandle() const;
	inline FXDLLLOCAL void int_genEBuffer() const;
	FXDLLLOCAL void int_xorInEBuffer(char *dest, const char *src, FXuval amount);
public:
	/*! Constructs an instance working with encrypted data device \em encrypteddev.
	Setting enablev2 to false permanently disables the SSL v2 protocol for this
	device (the default) */
	QSSLDevice(QIODevice *encrypteddev=0, bool enablev2=false);
	~QSSLDevice();

	//! Returns the encrypted data device being used
	QIODevice *encryptedDev() const throw();
	//! Sets the encrypted data device being used. Closes any previously set device
	void setEncryptedDev(QIODevice *dev);
	//! Returns the key being used to encrypt & decrypt the data (file type devices only)
	const FXSSLKey &key() const;
	//! Sets the key being used to encrypt & decrypt the data (file type devices only)
	void setKey(const FXSSLKey &key);
	//! Returns true is SSL v2 is enabled
	bool SSLv2Available() const throw();
	//! Sets if SSL v2 is available during negotiation
	void setSSLv2Available(bool a);
	//! Returns true is SSL v3 is enabled
	bool SSLv3Available() const throw();
	//! Sets if SSL v3 is available during negotiation
	void setSSLv3Available(bool a);
	//! Returns the list of ciphers to be available during the negotiation process (synchronous devices only)
	FXString ciphers() const;
	//! Sets the list of ciphers to be available during the negotiation process (synchronous devices only)
	void setCiphers(const FXString &list);

	//! True if SSL v2 is currently in use
	bool usingSSLv2() const;
	//! True if SSL v3 is currently in use
	bool usingSSLv3() const;
	//! True if TLS v1 is currently in use
	bool usingTLSv1() const;
	/*! QSSLDevice retrieves the X509 authentication certificate of the other
	end of the connection and returns the host name here which you should
	case-insensitively compare to FX::FXNetwork::dnsReverseLookup() on the IP
	address of the other end. Note that with non-authenticated protocols, this
	call returns a null string */
	FXString peerHostNameByCertificate() const;
	//! Returns the actual cipher in use
	FXString cipherName() const;
	//! Returns the bits used by the cipher in use
	FXuint cipherBits() const;
	//! Returns a textual description of the cipher in use
	FXString cipherDescription() const;
	/*! Performs a protocol & key renegotiation with the other end. This correctly
	forces clients to renegotiate as well as servers. Reads and writes may continue
	during renegotiation but only if they are performed in another thread.
	*/
	void renegotiate();

	//! Returns the size of the TNFXSECD header for this instance
	FXuint fileHeaderLen() const throw();

	virtual bool isSynchronous() const;
	virtual bool create(FXuint mode=IO_ReadWrite);
	virtual bool open(FXuint mode=IO_ReadWrite);
	virtual void close();
	virtual void flush();
	virtual FXfval size() const;
	//! Note that unlike most FX::QIODevice's, extending the file sets random data rather than zeros
	virtual void truncate(FXfval size);
	virtual FXfval at() const;
	virtual bool at(FXfval newpos);
	virtual bool atEnd() const;
	virtual const FXACL &permissions() const;
	virtual void setPermissions(const FXACL &);
	virtual FXuval readBlock(char *data, FXuval maxlen);
	virtual FXuval writeBlock(const char *data, FXuval maxlen);
	virtual FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos);
	virtual FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen);
	virtual int ungetch(int c);
public:
	/*! Sets the certificate file to be used for all new QSSLDevice connections.
	Will throw an error if the private key does not match the certificate key.
	*/
	static void setCertificateFile(const FXString &path);
	/*! Sets the private key to be used for all new QSSLDevice connections from
	a PEM format file which is optionally encrypted with \em password. You
	should set this before the certificate file so that it can be checked against it.
	\note Normally QSSLDevice generates a random private key, so you don't need
	to set this before use
	*/
	static void setPrivateKeyFile(const FXString &path, const FXString &password);
	//! Returns a cipher string representing the best available non-authenticated encryption (currently ADH-AES256-SHA)
	static const FXString &strongestAnonCipher();
	//! Returns a cipher string representing the fastest non-authenticated encryption (currently ADH-AES128-SHA)
	static const FXString &fastestAnonCipher();
};

} // namespace

#endif
