
main: libRangeEncoder.so
	g++ -g test.cpp -L. -lRangeEncoder -o test

libRangeEncoder.so:
	g++ -g range_encoder.cpp -fPIC -shared -o libRangeEncoder.so

clean:
	rm -f *.so test