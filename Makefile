
all:
	g++ -g -o hw4 hw4.cpp GLee.o -lGL -lGLU -lglut -lm 	
clean:
	rm -f hw4
