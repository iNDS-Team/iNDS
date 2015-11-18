
#TARGET="Unix Makefiles"
TARGET="CodeBlocks - Unix Makefiles"
#TARGET="KDevelop3"
#TARGET="Eclipse CDT4 - Unix Makefiles"

rm -fr Alone
mkdir Alone
cd Alone
cp ../CMakeLists_Alone.txt CMakeLists.txt

cmake -G "${TARGET}" -DCMAKE_BUILD_TYPE=Debug 
#cmake -G "${TARGET}" -DCMAKE_BUILD_TYPE=Release 

