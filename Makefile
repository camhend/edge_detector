CFLAGS= -g -Wall

all: edge_detector 

edge_detector: edge_detector.c
	gcc $(CFLAGS) edge_detector.c -o edge_detector

clean: 
	@echo -n Cleaning...
	@rm -f *.o *.ppm edge_detector
	@echo done
