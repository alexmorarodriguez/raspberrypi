i2cdetect -y 1
#-Wall print all warning messages
#-o specifies the output executable filename
#-g generates additional symbolic debugging information for use with gdb debugger
#-v running the compilation in verbose mode to studi library-paths (-L) and libraries (-l)
gcc -Wall -v  -o temp temp.c -lwiringPi
sudo ./temp > file.log 2>&1 

