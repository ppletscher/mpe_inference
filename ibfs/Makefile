all:	mex_gc_ibfs.mexa64

mex_gc_ibfs.mexa64:	../mex_gc.cpp GCoptimization.cpp ibfs.cpp LinkedBlockList.cpp
					cp ../mex_gc.cpp mex_gc_ibfs.cpp
					mex -lut -largeArrayDims mex_gc_ibfs.cpp GCoptimization.cpp ibfs.cpp LinkedBlockList.cpp

clean:	
		rm -f GCoptimization.o ibfs.o LinkedBlockList.o mex_gc_ibfs.cpp *.mexa64
