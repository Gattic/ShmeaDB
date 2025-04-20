mkdir build
cd build
emcmake cmake ..
emmake make -j4
emmake make install

mkdir GNetObjects
cd GNetObjects
emar x ../Backend/Networking/libGNet.a
cd ..
echo "Extracted GNet Objects"

mkdir GDBObjects
cd GDBObjects
emar x ../Backend/Database/libDB.a
cd ..
echo "Extracted GDB Objects"

mkdir GPlotterObjects
cd GPlotterObjects
emar x ../Backend/Plotter/libPlotter.a
cd ..
echo "Extracted GPlotter Objects"

mkdir GObjects
cd GObjects
emar x ../libshmea.a
cd ..
echo "Extracted G Objects"

emar rcs libshmea_combined.a GObjects/*.o GDBObjects/*.o GPlotterObjects/*.o GNetObjects/*.o
echo "Combined G Objects into libshmea_combined.a"

cp libshmea_combined.a $HOME/.local/lib/libshmea.a
echo "Copied libshmea_combined.a to $HOME/.local/lib/libshmea.a"

rm ~/.local/lib/libshmea.so
echo "Removed ~/.local/lib/libshmea.so (if it exists, not allowed to exist for emsdk)"
