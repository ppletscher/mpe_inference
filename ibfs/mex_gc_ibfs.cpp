#include <mex.h>
#include <vector>
#include <map>
#include "energy.h"
#include "GCoptimization.h"

// Input Arguments

#define POTENTIAL_UNARY_IN      prhs[0]
#define EDGES_IN                prhs[1]
#define POTENTIAL_PAIR_IN       prhs[2]
#define NR_IN                   3
#define NR_IN_OPT               0

// Output Arguments

#define LABEL_OUT               plhs[0]
#define ENERGY_OUT              plhs[1]
#define NR_OUT                  1
#define NR_OUT_OPT              1

double* potential_pair;
std::vector< std::map<int,int> >* edge_lookup_ptr;
size_t num_edges;

double smoothCost(int s1, int s2, int l1, int l2)
{
    std::map<int,int>::iterator it;
    it = (*edge_lookup_ptr)[s1].find(s2);
    if (it == (*edge_lookup_ptr)[s1].end()) {
        mexErrMsgTxt("An error occurred during the lookup!\n");
    }
    int e_idx = it->second;
    if (e_idx >= num_edges) {
        mexErrMsgTxt("Exceeding number of edges!\n");
    }

    // smaller index changes the fastest (only matters for non-symmetric
    // potentials)
    if (s1 < s2) {
        return potential_pair[e_idx*4+l1+l2*2];
    }
    else {
        return potential_pair[e_idx*4+l2+l1*2];
    }
}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Check for proper number of arguments
    if ( (nrhs < NR_IN) || (nrhs > NR_IN + NR_IN_OPT) || \
            (nlhs < NR_OUT) || (nlhs > NR_OUT + NR_OUT_OPT) ) {
        mexErrMsgTxt("Wrong number of arguments.");
    }

    // dimensionality checks
    size_t num_nodes = mxGetN(POTENTIAL_UNARY_IN);
    num_edges = mxGetN(EDGES_IN);
    if ( mxGetM(POTENTIAL_UNARY_IN) != 2 ) {
        mexErrMsgTxt("Only support binary problems!");
    }
    if ( mxGetM(EDGES_IN) != 2 ) {
        mexErrMsgTxt("Edges array has wrong size!");
    }
    if ( (mxGetM(POTENTIAL_PAIR_IN) != 4) || (mxGetN(POTENTIAL_PAIR_IN) != num_edges) ) {
        mexErrMsgTxt("Inconsistent size of the pairwise potentials.");
    }

    // read the inputs
    double* potential_unary = mxGetPr(POTENTIAL_UNARY_IN);
    double* edges = mxGetPr(EDGES_IN); // TODO: convert to int32? Or require it to be int32??
    potential_pair = mxGetPr(POTENTIAL_PAIR_IN);
            
    // unaries and function pointer for the pairwise
    GCoptimizationGeneralGraph* gco = new GCoptimizationGeneralGraph(num_nodes, 2);
    gco->setDataCost(potential_unary);
    gco->setSmoothCost(&smoothCost);

    // an edge lookup table for the pairwise potentials (for within smoothcost)
    std::vector< std::map<int,int> > edge_lookup;
    edge_lookup.resize(num_nodes);
    edge_lookup_ptr = &edge_lookup;
    for (int i = 0; i < (int)(num_edges); i++) {
        int s = int(edges[2*i])-1;
        int d = int(edges[2*i+1])-1;

        gco->setNeighbors(s, d);
        edge_lookup[s].insert(std::make_pair(d,i));
        edge_lookup[d].insert(std::make_pair(s,i));
    }

    // solve the graphcut problem
    double energy = gco->expansion(1);

    // return the labeling and the energy
    double *ptr;
    if (nlhs > 1) {
        ENERGY_OUT = mxCreateDoubleMatrix(1, 1, mxREAL);
        ptr = mxGetPr(ENERGY_OUT);
        *ptr = energy;
    }

    LABEL_OUT = mxCreateDoubleMatrix(1, num_nodes, mxREAL);
    ptr = mxGetPr(LABEL_OUT);    
    for(size_t i = 0; i < num_nodes; i++) {
        ptr[i] = gco->whatLabel(i);
    }

    delete gco;
}
