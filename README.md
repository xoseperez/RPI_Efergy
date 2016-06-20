gcc -o EfergyRPI_001 EfergyRPI_001.c -lm
rtl_fm -f 433550000 -s 200000 -r 96000 -g 19.7 2>/dev/null | ./EfergyRPI_001 
