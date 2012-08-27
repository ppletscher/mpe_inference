all:	mex_gc_bk.mexa64

mex_gc_bk.mexa64:	../mex_gc.cpp GCoptimization.cpp maxflow.cpp graph.cpp LinkedBlockList.cpp
					cp ../mex_gc.cpp mex_gc_bk.cpp
					mex -lut -largeArrayDims mex_gc_bk.cpp GCoptimization.cpp maxflow.cpp graph.cpp LinkedBlockList.cpp

clean:	
		rm -f mex_gc_bk.cpp GCoptimization.o maxflow.o graph.o LinkedBlockList.o *.mexa64
