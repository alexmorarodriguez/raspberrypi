i2cdetect -y 1
gcc -Wall -o temp temp.c -lwiringPi
sudo ./temp > file.log 2>&1 

