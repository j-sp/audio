fm_test: fm_test.o adsr.o
	gcc fm_test.o adsr.o -lm -lportaudio -o fm_test

fm_test.o: fm_test.c adsr.h
	gcc -c fm_test.c

adsr.o: adsr.c
	gcc -c adsr.c

clean:
	rm -f *.o fm_test