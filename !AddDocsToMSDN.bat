@if not exist "C:\Program Files\Microsoft Help 2.0 SDK" goto needsdk
@if not %USERNAME% == Administrator goto needadmin

"C:\Program Files\Microsoft Help 2.0 SDK\hxconv.exe" -o TnFOXDocs TnFOXDocs.chm
cd TnFOXDocs
"C:\Program Files\Microsoft Help 2.0 SDK\hxcomp.exe" -p TnFOXDocs.HxC -l logfile.txt
copy ..\DOxygenStuff\TnFOXCollection.* .
"C:\Program Files\Microsoft Help 2.0 SDK\hxreg.exe" -n TnFOXDocsNamespace -d "TnFOXDocs Namespace" -c TnFOXCollection.HxC
"C:\Program Files\Microsoft Help 2.0 SDK\hxreg.exe" -n TnFOXDocsNamespace -i TnFOXHelpTitle -s TnFOXDocs.HxS -x TnFOXDocs.HxI
cd ..

@echo Shut down all MSVC's and tick the TnFOXDocs box
start iexplore ms-help://MS.VSCC.2003/VSCCCommon/cm/CollectionManager.htm

@echo All done!
@goto end

:needsdk

@echo You need the Visual Studio .NET Help Integration Kit for this to work. Get it at http://www.microsoft.com/downloads/details.aspx?familyid=ce1b26dc-d6af-42a1-a9a4-88c4eb456d87&displaylang=en
@goto end

:needadmin

@echo You need to run this as the Administrator user
runas /user:administrator !AddDocsToMSDN.bat

:end
