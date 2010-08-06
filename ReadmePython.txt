Readme for TnFOX Python bindings (30th March 2004):
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
Please consult the documentation (Python support) for build instructions.
The below is just (outdated) detail for those interested.

Issue 1: Linking takes a very, very long time:
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
On MSVC7.1:
Even here on my dual Athlon 1700, linking takes about two hours
for release builds. The reason is because the very large number
of symbols from all the heavy template usage each occupy their
own COMDAT section and for release builds I have told it to
eliminate unused COMDATs. This would appear to be an O(n^2)
operation :(

If your machine is too slow, you may have to forego COMDAT
folding which is the most expensive (remove the ICF and add
/OPT:NOICF). If it's still too slow, you will have to disable
unused COMDAT elimination with /OPT:NOREF. For reference, a
debug build comes in at around 15.2Mb and release 11.5Mb.

Issue 2: Bug in pyste does not clarify overload (fixed by PatchWrappers.txt):
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
Affects:
* FXIODeviceS

readBlock() and writeBlock() have a convenience overload in
FXIODevice which converts from FXuchar * to char *. Pyste fails
to notice this when wrapping FXIODeviceS and so fails to clarify
which overload it wants.

Solution: Prefix with a suitable cast indicating the correct
overload

Issue 3: Translating C arrays (fixed by PatchWrappers.txt):
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
Affects:
* FXApp::getArgv, init
* FXGLTriangleMesh::getVertexBuffer, getColorBuffer, getNormalBuffer,
getTextureCoordBuffer
* FXGLViewer::lasso[1], select[12]
* FXImage::getData
* FXObjectList::list

The header file CArrays.h provides a nasty macro which provides a
translation function for these methods. The function creates an
emulation of a python list which proxies accesses to the array. Note that
unlike normal python lists, this one is not extendable but individual
elements can be altered, slices made and anything else you'd expect
from a python list.

[1]: These offer an extremely simple container with only __getitem__
and __len__ only. No slicing or anything else is available.
[2]: I had to disable the ability for python to override these methods.

