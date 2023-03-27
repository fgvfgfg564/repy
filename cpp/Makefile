
main: libre.so
	mkdir -p test_exec
	g++ -g test.cpp -L. -lre -o test_exec/test

libre.so: range_encoder.cpp bitstream.cpp CDFTable.cpp range_encoder_pyfunc.cpp
	g++ -g range_encoder.cpp bitstream.cpp CDFTable.cpp range_encoder_pyfunc.cpp -fPIC -shared -o libre.so

clean:
	rm -f *.so test *.o

test_bitstream: libre.so test_bitstream.cpp
	mkdir -p test_exec
	g++ -g test_bitstream.cpp -L. -lre -o test_exec/test_bitstream