test: adsr_test.o adsr.o
	gcc adsr_test.o adsr.o -lm -lportaudio -o adsr_test

adsr_test.o: adsr_test.c adsr.h
	gcc -c adsr_test.c

adsr.o: adsr.c
	gcc -c adsr.c

graph: adsr_graph.o adsr.o
	gcc adsr_graph.o adsr.o -o adsr_graph

adsr_graph.o: adsr_graph.c adsr.h
	gcc -c adsr_graph.c

clean:
	rm -f *.o adsr_graph adsr_test diag.txt