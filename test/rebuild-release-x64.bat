rd /S/Q build
md build
rd /S/Q bin
md bin

cd build

cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . --config Release

cd ..