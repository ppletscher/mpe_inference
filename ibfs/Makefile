all:	mex_gc_ibfs.mexa64

mex_gc_ibfs.mexa64:	mex_gc_ibfs.cpp GCoptimization.cpp ibfs.cpp LinkedBlockList.cpp
					mex -lut -largeArrayDims mex_gc_ibfs.cpp GCoptimization.cpp ibfs.cpp LinkedBlockList.cpp

clean:	
		rm -f GCoptimization.o ibfs.o LinkedBlockList.o *.mexa64
