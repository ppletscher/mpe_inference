all:	mex_gc_bk.mexa64

mex_gc_bk.mexa64:	mex_gc_bk.cpp GCoptimization.cpp maxflow.cpp graph.cpp LinkedBlockList.cpp
					mex -lut -largeArrayDims mex_gc_bk.cpp GCoptimization.cpp maxflow.cpp graph.cpp LinkedBlockList.cpp

clean:	
		rm -f GCoptimization.o maxflow.o graph.o LinkedBlockList.o *.mexa64
