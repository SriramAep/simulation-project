echo "Removing the build folder"
rm -rf build/
echo "Generating build system"
cmake -B build
echo "Compiling code..."
cmake --build build
