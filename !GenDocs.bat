rmdir /S /Q html
mkdir html
"g:\Program Files\doxygen\bin\doxygen.exe" DOxygenStuff/doxygen.cfg 2> doxygen_output.txt
copy DOxygenStuff\tornado_wm.png html
