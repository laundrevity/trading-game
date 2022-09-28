trading game <br>
for first build,
```
docker-compose up -d
docker-compose exec tg fish
mkdir build
cd build
cmake -DCMAKE_RELEASE_TYPE=Debug -DCMAKE_CXX_COMPILER=/usr/bin/g++-11 ../cxx
make -j12
```
