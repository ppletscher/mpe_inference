#include "mex.h"
#include "energy.h"
#include "GCoptimization.h"
#include <vector>
#include <map>

int idx_higher;
int nEdges;

std::vector< std::map<int,int> >* edgeLookupPtr;

double* smoothStandard;
double* smoothHigher;

double* edgeWeight;

double smoothCost(int s1, int s2, int l1, int l2)
{
    if ((s1 == idx_higher) || (s2 == idx_higher)) {
        if (s2==idx_higher) {
            return smoothHigher[l1+l2*2];
        }
        else {
            return smoothHigher[l2+l1*2];
        }
    }
    else {
        std::map<int,int>::iterator it;
        it = (*edgeLookupPtr)[s1].find(s2);
        if (it == (*edgeLookupPtr)[s1].end()) {
            mexErrMsgTxt("An error occurred during the lookup!\n");
        }
        int e_idx = it->second;
        if (e_idx >= nEdges) {
            //mexPrintf("s1: %d, s2: %d, e_idx: %d.\n", s1, s2, e_idx);
            //for (it = (*edgeLookupPtr)[s1].begin(); it != (*edgeLookupPtr)[s1].end(); ++it) {
            //    mexPrintf("%d, %d.\n", it->first, it->second);
            //}
            mexErrMsgTxt("Exceeding number of edges!\n");
        }

        if (edgeWeight[e_idx] < 0) {
            //mexPrintf("smoothCost: weight is negative! e_idx: %d.\n", e_idx);
            mexErrMsgTxt("smoothCost: weight is negative!");
        }

        // TODO: check orientation! -> doesn't matter as symmetric!
        return smoothStandard[l1+l2*2]*edgeWeight[e_idx];
    }
}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs,
                 const mxArray *prhs[])
{
    if(!(nrhs == 5 || nrhs == 4))
        mexErrMsgTxt("4 or 5 input parameters required!");
    if(mxGetM(prhs[0]) != 2 || mxGetM(prhs[1]) != 2 || mxGetM(prhs[2]) != 1 || mxGetM(prhs[3]) != 2 || mxGetN(prhs[3]) != 2)
        mexErrMsgTxt("Inconsistent input size!");
            
    int nNodes = mxGetN(prhs[0]);
    nEdges = mxGetN(prhs[1]);

    double *nodeData = mxGetPr(prhs[0]);
    double *nodeEdge = mxGetPr(prhs[1]);
    edgeWeight = mxGetPr(prhs[2]);
    smoothStandard = mxGetPr(prhs[3]);
    if (nrhs > 4) {
        smoothHigher = mxGetPr(prhs[4]);
    }

    GCoptimizationGeneralGraph* gco = new GCoptimizationGeneralGraph(nNodes, 2);

    gco->setDataCost(nodeData);
    
    gco->setSmoothCost(&smoothCost);
    
    if (nrhs == 5) {
        idx_higher = nNodes-1;
    }
    else {
        idx_higher = -1;
    }

    //mexPrintf("idx_higher: %d\n", idx_higher); 
    std::vector< std::map<int,int> > edgeLookup;
    edgeLookup.resize(nNodes);
    edgeLookupPtr = &edgeLookup;
    for (int i = 0; i < (int)(nEdges); i++) {
        int s = int(nodeEdge[2*i])-1;
        int d = int(nodeEdge[2*i+1])-1;

        gco->setNeighbors(s, d);

        if ((s != idx_higher) && (d != idx_higher)) {
            edgeLookup[s].insert(std::make_pair(d,i));
            edgeLookup[d].insert(std::make_pair(s,i));
        }
        if ((i < idx_higher) && (edgeWeight[i] < 0)) {
            mexErrMsgTxt("negative weights not allowed!\n");
        }
    }
    //mexPrintf("numEdges: %d.\n", (int)(nEdges));

    double energy = gco->expansion(1);

    double *ptr;
    if(nlhs > 1)
    {
        plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
        ptr = mxGetPr(plhs[1]);
        *ptr = energy;
    }

    plhs[0] = mxCreateDoubleMatrix(1, nNodes, mxREAL);
    ptr = mxGetPr(plhs[0]);    
    
    for(size_t i = 0; i < nNodes; i++) {
        ptr[i] = gco->whatLabel(i);
    }

    delete gco;
}
