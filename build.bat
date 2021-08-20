call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Optimization switches /O2
set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -std:c++17  -FC -Z7 
set CommonLinkerFlags=-incremental:no -opt:ref

IF NOT EXIST .\build mkdir .\build

pushd .\build
cl %CommonCompilerFlags% -Fetest.cpp ..\main.cpp /link %CommonLinkerFlags% 
popd

echo ----------------------
echo ** Complile Succeed ** 