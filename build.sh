[ -d './build' ] || mkdir -p './build'
cd build

TYPE="${1:-Debug}"

cmake .. -DCMAKE_BUILD_TYPE="$TYPE"

cmake --build . --config "$TYPE"
