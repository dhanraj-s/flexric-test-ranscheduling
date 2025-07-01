rm -rf build
mkdir build
cd build
cmake -DXAPP_MULTILANGUAGE=ON -DE2AP_V2=ON -DKPM_VERSION=KPM_V2_03 .. && make -j8

sudo make install
