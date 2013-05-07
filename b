rm *.o *.a
clang++ -stdlib=libc++ -I/Users/trent/code/local/include -c tgui2.cpp
clang++ -stdlib=libc++ -I/Users/trent/code/local/include -c tgui2_widgets.cpp
ar rc libtgui2.a *.o
ranlib libtgui2.a
