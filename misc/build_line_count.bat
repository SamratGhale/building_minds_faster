@echo off
set CommonCompilerFlags=-MTd -DDEBUG=1 /std:c++17 -nologo -fp:fast -Gm- -GR- -Od -Oi -FC -Z7 /EHsc
set CommonLinkerFlags= -incremental:no -opt:ref 
IF NOT EXIST .\build mkdir .\build
pushd .\build

del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\misc\line_count.cpp /link %CommonLinkerFlags%
popd