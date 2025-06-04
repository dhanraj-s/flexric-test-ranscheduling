rm -rf build
mkdir build
cd build
cmake -DXAPP_MULTILANGUAGE=ON .. && make -j8
sudo make install
