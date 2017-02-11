all:
	gcc lispy.c -o lispy -ledit -std=c99

clean:
	rm lispy
