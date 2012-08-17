


#include <stdio.h>
#include <stdlib.h>
#include "ibfs.h"
#include <limits.h>

#define TERMINAL ( (arc *) 1 )		/* to terminal */

#define END_OF_ORPHANS   ( (node *) 1 )
#define PREVIOUSLY_ORPHAN ( (node *) 2 )
#define END_OF_LIST_NODE ( (node *) 1 )




#define ADD_ORPHAN_BACK(n)				\
if ((n)->nextOrphan == NULL)			\
{										\
	(n)->parent = (n)->firstArc;			\
}										\
if (orphanFirst != END_OF_ORPHANS)		\
{										\
	orphanLast = (orphanLast->nextOrphan = (n));	\
}										\
else									\
{										\
	orphanLast = (orphanFirst = (n));	\
}										\
(n)->nextOrphan = END_OF_ORPHANS





#define ADD_ORPHAN_FRONT(n)				\
if ((n)->nextOrphan == NULL)			\
{										\
	(n)->parent = (n)->firstArc;			\
}										\
if (orphanFirst == END_OF_ORPHANS)		\
{										\
	(n)->nextOrphan = END_OF_ORPHANS;	\
	orphanLast = (orphanFirst = (n));	\
}										\
else									\
{										\
	(n)->nextOrphan = orphanFirst;		\
	orphanFirst = (n);					\
}




#define SET_ACTIVE(i)	\
	if ((i)->nextActive == NULL)	\
	{				\
		(i)->nextActive = END_OF_LIST_NODE;	\
		if (activeFirst1 == END_OF_LIST_NODE)	\
		{						\
			activeLast1 = (activeFirst1 = (i));	\
		}						\
		else						\
		{						\
			activeLast1 = (activeLast1->nextActive = (i));	\
		}							\
  }






#define NODE_PTR_TO_INDEX(p)		\
( ((p) == NULL) ? (-1) : ((int)((p)-nodes)) )



#define NODE_INDEX_TO_PTR(ind)		\
( ((ind) == -1) ? (NULL) : (nodes+ind) )









template <typename captype, typename tcaptype, typename flowtype> IBFSGraph<captype, tcaptype, flowtype>::IBFSGraph(int numNodes, int numEdges, void (*errorFunctionArg)(char*))
{
	errorFunction = errorFunctionArg;
	nNodes = 0;

	nodes = (node*) malloc((numNodes+1)*sizeof(node));
	arcs = (arc*) malloc((2*numEdges)*sizeof(arc));
	if (!nodes || !arcs)
	{
		if (errorFunction)
		{
			(*errorFunction)("Cannot Allocate Memory!\n");
		}
		exit(1);
	}

	node *maxNode = nodes + numNodes - 1;
	for (nodeLast = nodes; nodeLast <= maxNode; nodeLast++)
	{
		nodeLast->firstArc = NULL;
		nodeLast->nextActive = NULL;
		nodeLast->label = 0;
		nodeLast->terminalCap = 0;
	}
	arcLast = arcs;
	flow = 0;
}

template <typename captype, typename tcaptype, typename flowtype> IBFSGraph<captype, tcaptype, flowtype>::~IBFSGraph()
{
	free(nodes);
	free(arcs);
}


template <typename captype, typename tcaptype, typename flowtype> void IBFSGraph<captype, tcaptype, flowtype>::augment(arc *middle_arc, AugmentationInfo* augInfo)
{
	node *i, *j;
	arc *a;
	captype bottleneck, flowCurr;

#ifdef STATS
	statsNumAugs++;
	statsNumPushes++;
	int augLen=1;
#endif

	// src tree
	bottleneck = middle_arc->rCap;
	if (augInfo->remainingExcess == 0)
	{
		augInfo->remainingExcess = INT_MAX;
		for (i=middle_arc->sister->head; ; i=a->head)
		{
	#ifdef STATS
			augLen++;
			statsNumPushes++;
	#endif
			a = i->parent;
			if (a == TERMINAL) break;
			if (augInfo->remainingExcess > a->sister->rCap)
			{
				augInfo->remainingExcess = a->sister->rCap;
			}
		}
		if (augInfo->remainingExcess > i->terminalCap)
		{
			augInfo->remainingExcess = i->terminalCap;
		}
	}
	if (bottleneck > augInfo->remainingExcess)
	{
		bottleneck = augInfo->remainingExcess;
	}

	// sink tree
	if (augInfo->remainingDeficit == 0)
	{
		augInfo->remainingDeficit = INT_MAX;
		for (i=middle_arc->head; ; i=a->head)
		{
	#ifdef STATS
			augLen++;
			statsNumPushes++;
	#endif
			a = i->parent;
			if (a == TERMINAL) break;
			if (augInfo->remainingDeficit > a->rCap)
			{
				augInfo->remainingDeficit = a->rCap;
			}
		}
		if (augInfo->remainingDeficit > (-i->terminalCap))
		{
			augInfo->remainingDeficit = (-i->terminalCap);
		}
	}
	if (bottleneck > augInfo->remainingDeficit)
	{
		bottleneck = augInfo->remainingDeficit;
	}

#ifdef STATS
	if (augLenMin > augLen)
	{
		augLenMin = augLen;
	}
	if (augLenMax < augLen)
	{
		augLenMax = augLen;
	}
#endif

	//
	// augment sink tree
	//
	augInfo->remainingDeficit -= bottleneck;
	augInfo->flowDeficit += bottleneck;
	if (augInfo->remainingDeficit == 0)
	{
		flowCurr = augInfo->flowDeficit;
		augInfo->flowDeficit = 0;
		for (i=middle_arc->head; ; i=a->head)
		{
			a = i->parent;
			if (a == TERMINAL) break;
			a->sister->rCap += flowCurr;
			a->sister_rCap = 1;
			a->rCap -= flowCurr;
			if (a->rCap == 0)
			{
				a->sister->sister_rCap = 0;
				j=i->parent->head->firstSon;
				if (j == i)
				{
					i->parent->head->firstSon = NODE_INDEX_TO_PTR(i->nextSibling);
				}
				else
				{
					for (; j->nextSibling != (i-nodes); j = (nodes+j->nextSibling));
					j->nextSibling = i->nextSibling;
				}
				ADD_ORPHAN_FRONT(i);
			}
		}
		i->terminalCap += flowCurr;
		if (i->terminalCap == 0)
		{
			ADD_ORPHAN_FRONT(i);
		}
		if (orphanFirst != END_OF_ORPHANS)
		{
			adoptionSink();
		}
	}

	//
	// augment src tree
	//
	middle_arc->sister->rCap += bottleneck;
	middle_arc->sister_rCap = 1;
	middle_arc->rCap -= bottleneck;
	if (middle_arc->rCap == 0)
	{
		middle_arc->sister->sister_rCap = 0;
	}
	augInfo->remainingExcess -= bottleneck;
	augInfo->flowExcess += bottleneck;
	if (augInfo->remainingExcess == 0)
	{
		flowCurr = augInfo->flowExcess;
		augInfo->flowExcess = 0;
		for (i=middle_arc->sister->head; ; i=a->head)
		{
			a = i->parent;
			if (a == TERMINAL) break;
			a->rCap += flowCurr;
			a->sister->sister_rCap = 1;
			a->sister->rCap -= flowCurr;
			if (a->sister->rCap == 0)
			{
				a->sister_rCap = 0;
				j=i->parent->head->firstSon;
				if (j == i)
				{
					i->parent->head->firstSon = NODE_INDEX_TO_PTR(i->nextSibling);
				}
				else
				{
					for (; j->nextSibling != (i-nodes); j = (nodes+j->nextSibling));
					j->nextSibling = i->nextSibling;
				}
				ADD_ORPHAN_FRONT(i);
			}
		}
		i->terminalCap -= flowCurr;
		if (i->terminalCap == 0)
		{
			ADD_ORPHAN_FRONT(i);
		}
		if (orphanFirst != END_OF_ORPHANS)
		{
			adoptionSrc();
		}
	}

	flow += bottleneck;

#ifdef STATS
//	for (int k=0; k<numOrphanPhaseNodes; k++)
//	{
//		int inc = orphanPhaseNodes[k]->label;
//		inc = (inc < 0) ? (-inc) : (inc);
//		inc = inc - orphanPhaseLabels[k];
//		if (inc == 0)
//		{
//			numOrphans0++;
//		}
//		else if (inc == 1)
//		{
//			numOrphans1++;
//		}
//		else
//		{
//			numOrphans2++;
//		}
//	}
#endif
}



//k = NULL;
//for (j=i->firstSon; j != NULL; j=NODE_INDEX_TO_PTR(j->nextSibling))
//{
//#ifdef STATS
//	orphanArcs3++;
//#endif
////				if (min_label == i->label && 
////					j->parent != j->firstArc)
////				{
////					tmp_a = *(j->parent);
////					*(j->parent) = *(j->firstArc);
////					*(j->firstArc) = tmp_a;
////					j->parent->sister->sister = j->parent;
////					j->firstArc->sister->sister = j->firstArc;
////					j->parent = j->firstArc;
////				}
//	if (i->parent == NULL ||
//		j->label != (min_label+1) ||
//		j->hasFlatChildren)
//	{
//		ADD_ORPHAN_BACK(j);
//		if (k == NULL)
//		{
//			i->firstSon = NODE_INDEX_TO_PTR(j->nextSibling);
//		}
//		else
//		{
//			k->nextSibling = j->nextSibling;
//		}
//	}
//	else
//	{
//		k=j;
//		i->hasFlatChildren = true;
//	}





template <typename captype, typename tcaptype, typename flowtype> void IBFSGraph<captype, tcaptype, flowtype>::adoptionSrc()
{
	node *i, *j;
	arc *a, *a_end;
	arc tmp_a;
	int min_label;

	while (orphanFirst != END_OF_ORPHANS)
	{
		// terminalCap is used as a next pointer for the orphans list
		i = orphanFirst;
		orphanFirst = i->nextOrphan;

#ifdef STATS
		statsNumOrphans++;
//		if (orphanPhaseNodesHash[i-nodes] == false)
//		{
//			orphanPhaseNodesHash[i-nodes] = true;
//			orphanPhaseLabels[numOrphanPhaseNodes] = (i->label);
//			orphanPhaseNodes[numOrphanPhaseNodes] = i;
//			numOrphanPhaseNodes++;
//		}
#endif

		// we need PREVIOUSLY_ORPHAN vs NULL
		// in order to establish whether the node
		// has already started a "new parent" scan
		// while in this level or not (used in ADD_ORPHAN)
		i->nextOrphan = PREVIOUSLY_ORPHAN;
		a=i->parent;
		i->parent = NULL;
		a_end = (i+1)->firstArc;

#ifdef COUNT_RELABELS
		if (scans[i-nodes] == 0)
		{
			scanIndices.push_back(i-nodes);
		}
		if ((scans[i-nodes]%2) == 0)
		{
			scans[i-nodes]++;
		}
#endif

		// check for rehook
		if (i->label != 1)
		{
			min_label = i->label - 1;
			for (; a != a_end; a++)
			{
#ifdef STATS
				orphanArcs1++;
#endif
				j = a->head;
				if (a->sister_rCap != 0 && 
					j->parent != NULL &&
					j->label == min_label)
				{
					i->parent = a;
					i->nextSibling = NODE_PTR_TO_INDEX(j->firstSon);
					j->firstSon = i;
					break;
				}
			}
		}

		// give up on node - relabel it!
		if (i->parent == NULL)
		{
#ifdef COUNT_RELABELS
			scans[i-nodes]++;
#endif

			min_label = activeLevel+1;
			for (a=i->firstArc; a != a_end; a++)
			{
#ifdef STATS
				orphanArcs2++;
#endif
				j = a->head;
				if (j->parent != NULL &&
					j->label > 0 &&
					j->label < min_label &&
					a->sister_rCap != 0)
				{
					min_label = j->label;
					i->parent = a;
					if (min_label == i->label) break;
				}
			}
			for (j=i->firstSon; j; j=NODE_INDEX_TO_PTR(j->nextSibling))
			{
#ifdef STATS
				orphanArcs3++;
#endif
				if (min_label == i->label && 
					j->parent != j->firstArc)
				{
					tmp_a = *(j->parent);
					*(j->parent) = *(j->firstArc);
					*(j->firstArc) = tmp_a;
					j->parent->sister->sister = j->parent;
					j->firstArc->sister->sister = j->firstArc;
				}
				ADD_ORPHAN_BACK(j);
			}
			i->firstSon = NULL;
			if (i->parent == NULL)
			{
				i->nextOrphan = NULL;
			}
			else
			{
				i->label = (min_label+1);
				i->nextSibling = NODE_PTR_TO_INDEX(i->parent->head->firstSon);
				i->parent->head->firstSon = i;
				if (min_label == activeLevel)
				{
					SET_ACTIVE(i);
				}
			}
		}
	}
}


template <typename captype, typename tcaptype, typename flowtype> void IBFSGraph<captype, tcaptype, flowtype>::adoptionSink()
{
	node *i, *j;
	arc *a, *a_end;
	arc tmp_a;
	int min_label;

	while (orphanFirst != END_OF_ORPHANS)
	{
		// terminalCap is used as a next pointer for the orphans list
		i = orphanFirst;
		orphanFirst = i->nextOrphan;

#ifdef STATS
		statsNumOrphans++;
//		if (orphanPhaseNodesHash[i-nodes] == false)
//		{
//			orphanPhaseNodesHash[i-nodes] = true;
//			orphanPhaseLabels[numOrphanPhaseNodes] = -(i->label);
//			orphanPhaseNodes[numOrphanPhaseNodes] = i;
//			numOrphanPhaseNodes++;
//		}
#endif
		// we need PREVIOUSLY_ORPHAN vs NULL
		// in order to establish whether the node
		// has already started a "new parent" scan
		// while in this level or not (used in ADD_ORPHAN)
		i->nextOrphan = PREVIOUSLY_ORPHAN;
		a = i->parent;
		i->parent = NULL;
		a_end = (i+1)->firstArc;

#ifdef COUNT_RELABELS
		if (scans[i-nodes] == 0)
		{
			scanIndices.push_back(i-nodes);
		}
		if ((scans[i-nodes]%2) == 0)
		{
			scans[i-nodes]++;
		}
#endif

		// check for rehook
		if (i->label != -1)
		{
			min_label = i->label + 1;
			for (; a != a_end; a++)
			{
#ifdef STATS
				orphanArcs1++;
#endif
				j = a->head;
				if (a->rCap != 0 && 
					j->parent != NULL &&
					j->label == min_label)
				{
					i->parent = a;
					i->nextSibling = NODE_PTR_TO_INDEX(j->firstSon);
					j->firstSon = i;
					break;
				}
			}
		}

		// give up on node - relabel it!
		if (i->parent == NULL)
		{
#ifdef COUNT_RELABELS
			scans[i-nodes]++;
#endif

			min_label = -(activeLevel+1);
			for (a=i->firstArc; a != a_end; a++)
			{
#ifdef STATS
				orphanArcs2++;
#endif
				j = a->head;
				if (a->rCap != 0 &&
					j->parent != NULL &&
					j->label < 0 &&
					j->label > min_label)
				{
					min_label = j->label;
					i->parent = a;
					if (min_label == i->label) break;
				}
			}
			for (j=i->firstSon; j; j=NODE_INDEX_TO_PTR(j->nextSibling))
			{
#ifdef STATS
				orphanArcs3++;
#endif
				if (min_label == i->label && 
					j->parent != j->firstArc)
				{
					tmp_a = *(j->parent);
					*(j->parent) = *(j->firstArc);
					*(j->firstArc) = tmp_a;
					j->parent->sister->sister = j->parent;
					j->firstArc->sister->sister = j->firstArc;
				}
				ADD_ORPHAN_BACK(j);
			}
			i->firstSon = NULL;
			if (i->parent == NULL)
			{
				i->nextOrphan = NULL;
			}
			else
			{
				i->label = (min_label-1);
				i->nextSibling = NODE_PTR_TO_INDEX(i->parent->head->firstSon);
				i->parent->head->firstSon = i;
				if (min_label == -activeLevel)
				{
					SET_ACTIVE(i);
				}
			}
		}
	}
}


template <typename captype, typename tcaptype, typename flowtype> void IBFSGraph<captype, tcaptype, flowtype>::prepareGraph()
{
	node *i, *j;
	arc *a, tmp_a;

	//printf("c sizeof(ptr) = %ld \n", sizeof(node*));
	//printf("c sizeof(node) = %ld \n", sizeof(node));
	//printf("c sizeof(arc) = %ld \n", sizeof(arc));
	//printf("c #nodes = %ld \n", nodeLast-nodes);
	//printf("c #arcs = %ld \n", (arcLast-arcs) + (nodeLast-nodes));
	//printf("c #grid_arcs = %ld \n", arcLast-arcs);
	//printf("c trivial_flow = %ld \n", flow);

	// calculate start arc pointers for every node
	for (i=nodes; i<nodeLast; i++)
	{
		if (i > nodes)
		{
			i->label += (i-1)->label;
		}
	}
	for (i=nodeLast; i>=nodes; i--)
	{
		if (i > nodes)
		{
			i->label = (i-1)->label;
		}
		else
		{
			i->label = 0;
		}
		i->firstArc = arcs + i->label;
	}

	// swap arcs
	for (i=nodes; i<nodeLast; i++)
	{
		for (; i->firstArc != (arcs+((i+1)->label)); i->firstArc++)
		{
			for (j = i->firstArc->sister->head; j != i; j = i->firstArc->sister->head)
			{
				// get and advance last arc fwd in proper node
				a = j->firstArc;
				j->firstArc++;

				// prepare sister pointers
				if (a->sister == i->firstArc)
				{
					i->firstArc->sister = i->firstArc;
					a->sister = a;
				}
				else
				{
					a->sister->sister = i->firstArc;
					i->firstArc->sister->sister = a;
				}

				// swap
				tmp_a = (*(i->firstArc));
				(*(i->firstArc)) = (*a);
				(*a) = tmp_a;
			}
		}
	}

	// reset first arc pointers
	for (i=nodes; i<nodeLast; i++)
	{
		i->firstArc = arcs + i->label;
		i->label = 0;
	}

	// set the sister_rCap field
	for (i=nodes; i<nodeLast; i++)
	{
		for (a=i->firstArc; a != (i+1)->firstArc; a++)
		{
			if (a->sister->rCap == 0)
			{
				a->sister_rCap = 0;
			}
			else
			{
				a->sister_rCap = 1;
			}
		}
	}
	
	// check consistency
	//for (i=nodes; i<nodeLast; i++)
	//{
	//	for (a=i->firstArc; a !=(i+1)->firstArc; a++)
	//	{
	//		if (a->sister->head != i ||
	//			a->sister->sister != a)
	//		{
	//			exit(1);
	//		}
	//	}
	//}
}

template <typename captype, typename tcaptype, typename flowtype> flowtype IBFSGraph<captype, tcaptype, flowtype>::maxflow()
{
	node *i, *j, *i_tmp, *prevSrc, *prevSink;
	arc *a, *a_end, *a_tmp;
	AugmentationInfo augInfo;

	prepareGraph();

#ifdef STATS
	numAugs = 0;
	numOrphans = 0;
	grownSinkTree = 0;
	grownSourceTree = 0;

	numPushes = 0;
	orphanArcs1 = 0;
	orphanArcs2 = 0;
	orphanArcs3 = 0;
	growthArcs = 0;

	skippedGrowth = 0;
	numOrphans0 = 0;
	numOrphans1 = 0;
	numOrphans2 = 0;
	augLenMin = 999999;
	augLenMax = 0;
#endif

	//
	// init
	//
	orphanFirst = END_OF_ORPHANS;
	activeFirst1 = END_OF_LIST_NODE;
	activeLevel = 1;
	for (i=nodes; i<nodeLast; i++)
	{
		i->nextActive = NULL;
		i->firstSon = NULL;

		if (i->terminalCap == 0)
		{
			i->parent = NULL;
		}
		else
		{
			i->parent = TERMINAL;
			if (i->terminalCap > 0)
			{
				i->label = 1;
				SET_ACTIVE(i);
			}
			else
			{
				i->label = -1;
				SET_ACTIVE(i);
			}
		}
	}
	activeFirst0 = activeFirst1;
	activeFirst1 = END_OF_LIST_NODE;

	//
	// growth + augment
	//
	prevSrc = NULL;
	prevSink = NULL;
	augInfo.flowDeficit = 0;
	augInfo.flowExcess = 0;
	while (activeFirst0 != END_OF_LIST_NODE)
	{
		//
		// BFS level
		//
		while (activeFirst0 != END_OF_LIST_NODE)
		{
			/*
			i = active_src_first0;
			while (i->nextActive != END_OF_LIST_NODE)
			{
				if (i->parent &&
					i->nextActive->parent &&
					i->label > 0 &&
					i->label != activeLevel_src &&
					i->label != activeLevel_src+1)
				{
					i = active_src_first0;
					printf("flow=%d, active_label=%d, src active ",
						flow, activeLevel_src);
					while (i != END_OF_LIST_NODE)
					{
						if (i->parent &&
							(i->nextActive == END_OF_LIST_NODE ||
							i->nextActive->parent == NULL ||
							i->label != i->nextActive->label))
						{
							printf("%d ", i->label);
						}
						i = i->nextActive;
					}
					printf("\n");
					break;
				}
				i = i->nextActive;
			}
			*/

			i = activeFirst0;
			activeFirst0 = i->nextActive;
			i->nextActive = NULL;
			if (i->parent == NULL)
			{
#ifdef STATS
				skippedGrowth++;
#endif
				continue;
			}
			//if (ABS(i->label) != activeLevel &&
			//	ABS(i->label) != (activeLevel+1))
			//{
//#ifdef FLOATS
			//	printf("ERROR, flow=%f, label=%d, active_label=%d\n",
			//		flow, i->label, activeLevel);
//#else
			//	printf("ERROR, flow=%d, label=%d, active_label=%d\n",
			//		flow, i->label, activeLevel);
//#endif
			//	exit(1);
			//}

			if (i->label > 0)
			{
				//
				// GROWTH SRC
				//
				if (i->label != activeLevel)
				{
#ifdef STATS
					skippedGrowth++;
#endif
					SET_ACTIVE(i);
					continue;
				}

#ifdef STATS
				grownSourceTree++;
#endif
				a_end = (i+1)->firstArc;
				for (a=i->firstArc; a != a_end; a++)
				{
#ifdef STATS
					growthArcs++;
#endif
					if (a->rCap != 0)
					{
						j = a->head;
						if (j->parent == NULL)
						{
							j->label = i->label+1;
							j->parent = a->sister;
							j->nextSibling = NODE_PTR_TO_INDEX(i->firstSon);
							i->firstSon = j;
							SET_ACTIVE(j);
						}
						else if (j->label < 0)
						{
							i->nextActive = activeFirst0;
							activeFirst0 = i;
							if (prevSrc != i)
							{
								augInfo.remainingExcess = 0;
								if (augInfo.flowExcess != 0)
								{
									i_tmp = prevSrc;
									for (; ; i_tmp=a_tmp->head)
									{
										a_tmp = i_tmp->parent;
										if (a_tmp == TERMINAL) break;
										a_tmp->rCap += augInfo.flowExcess;
										a_tmp->sister->sister_rCap = 1;
										a_tmp->sister->rCap -= augInfo.flowExcess;
									}
									i_tmp->terminalCap -= augInfo.flowExcess;
									augInfo.flowExcess = 0;
								}
							}
							if (prevSrc != i || prevSink != j)
							{
								augInfo.remainingDeficit = 0;
								if (augInfo.flowDeficit != 0)
								{
									i_tmp = prevSink;
									for (; ; i_tmp=a_tmp->head)
									{
										a_tmp = i_tmp->parent;
										if (a_tmp == TERMINAL) break;
										a_tmp->sister->rCap += augInfo.flowDeficit;
										a_tmp->sister_rCap = 1;
										a_tmp->rCap -= augInfo.flowDeficit;
									}
									i_tmp->terminalCap += augInfo.flowDeficit;
									augInfo.flowDeficit = 0;
								}
							}
							augment(a, &augInfo);
							prevSrc = i;
							prevSink = j;
							break;
						}
					}
				}
			}
			else
			{
				//
				// GROWTH SINK
				//
				if (-(i->label) != activeLevel)
				{
#ifdef STATS
					skippedGrowth++;
#endif
					SET_ACTIVE(i);
					continue;
				}

#ifdef STATS
				grownSinkTree++;
#endif
				a_end = (i+1)->firstArc;
				for (a=i->firstArc; a != a_end; a++)
				{
#ifdef STATS
					growthArcs++;
#endif
					if (a->sister_rCap != 0)
					{
						j = a->head;
						if (j->parent == NULL)
						{
							j->label = i->label-1;
							j->parent = a->sister;
							j->nextSibling = NODE_PTR_TO_INDEX(i->firstSon);
							i->firstSon = j;
							SET_ACTIVE(j);
						}
						else if (j->label > 0)
						{
							i->nextActive = activeFirst0;
							activeFirst0 = i;
							if (prevSink != i)
							{
								augInfo.remainingDeficit = 0;
								if (augInfo.flowDeficit != 0)
								{
									i_tmp = prevSink;
									for (; ; i_tmp=a_tmp->head)
									{
										a_tmp = i_tmp->parent;
										if (a_tmp == TERMINAL) break;
										a_tmp->sister->rCap += augInfo.flowDeficit;
										a_tmp->sister_rCap = 1;
										a_tmp->rCap -= augInfo.flowDeficit;
									}
									i_tmp->terminalCap += augInfo.flowDeficit;
									augInfo.flowDeficit = 0;
								}
							}
							if (prevSink != i || prevSrc != j)
							{
								augInfo.remainingExcess = 0;
								if (augInfo.flowExcess != 0)
								{
									i_tmp = prevSrc;
									for (; ; i_tmp=a_tmp->head)
									{
										a_tmp = i_tmp->parent;
										if (a_tmp == TERMINAL) break;
										a_tmp->rCap += augInfo.flowExcess;
										a_tmp->sister->sister_rCap = 1;
										a_tmp->sister->rCap -= augInfo.flowExcess;
									}
									i_tmp->terminalCap -= augInfo.flowExcess;
									augInfo.flowExcess = 0;
								}
							}
							augment(a->sister, &augInfo);
							prevSrc = j;
							prevSink = i;
							break;
						}
					}
				}
			}
		}

		augInfo.remainingDeficit = 0;
		if (augInfo.flowDeficit != 0)
		{
			i_tmp = prevSink;
			for (; ; i_tmp=a_tmp->head)
			{
				a_tmp = i_tmp->parent;
				if (a_tmp == TERMINAL) break;
				a_tmp->sister->rCap += augInfo.flowDeficit;
				a_tmp->sister_rCap = 1;
				a_tmp->rCap -= augInfo.flowDeficit;
			}
			i_tmp->terminalCap += augInfo.flowDeficit;
			augInfo.flowDeficit = 0;
			prevSink = NULL;
		}
		augInfo.remainingExcess = 0;
		if (augInfo.flowExcess != 0)
		{
			i_tmp = prevSrc;
			for (; ; i_tmp=a_tmp->head)
			{
				a_tmp = i_tmp->parent;
				if (a_tmp == TERMINAL) break;
				a_tmp->rCap += augInfo.flowExcess;
				a_tmp->sister->sister_rCap = 1;
				a_tmp->sister->rCap -= augInfo.flowExcess;
			}
			i_tmp->terminalCap -= augInfo.flowExcess;
			augInfo.flowExcess = 0;
			prevSrc = NULL;
		}

		//
		// switch to next level
		//
#ifdef COUNT_RELABELS
		for (int k=0; k<scanIndices.size(); k++)
		{
			total++;
			if (scans[scanIndices[k]] <= 10)
			{
				counts[scans[scanIndices[k]]-1]++;
			}
		}
#endif

		activeFirst0 = activeFirst1;
		activeFirst1 = END_OF_LIST_NODE;
		activeLevel++;
	}
	
	return flow;
}

