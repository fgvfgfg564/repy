
main: libre.so
	mkdir -p test_exec
	g++ -g test.cpp -L. -lre -o test_exec/test

libre.so:
	g++ -g range_encoder.cpp -fPIC -shared -o libre.so

clean:
	rm -f *.so test

test_bitstream: libre.so test_bitstream.cpp
	mkdir -p test_exec
	g++ -g test_bitstream.cpp -L. -lre -o test_exec/test_bitstream