
main: libre.so
	g++ -g test.cpp -L. -lre -o test

libre.so:
	g++ -g range_encoder.cpp -fPIC -shared -o libre.so

clean:
	rm -f *.so test