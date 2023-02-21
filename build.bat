@echo off
set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -Od -Oi -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref
IF NOT EXIST .\build mkdir .\build
pushd .\build

del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\platform.cpp /link %CommonLinkerFlags%
popd