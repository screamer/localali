/**
Author: Jialu Hu
Date: Jun. 11, 2013
File name: algorithm/subnet.h
Description: Searching high-scoring subnetworks.
**/

#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include "verbose.h"

template<typename NP, typename SN, typename LG>
class Search
{
private:
	typedef NP NetworkPool;
	typedef SN SubNet;
	typedef LG LayerGraph;
	typedef typename LayerGraph::Graph Graph;
	typedef typename SubNet::K_Spine K_Spine;
public:
  TEMPLATE_GRAPH_TYPEDEFS(Graph);
  /// Labels of the nodes.
  typedef typename Graph::template NodeMap<std::string> OrigLabelNodeMap;
  /// Mapping from labels to original nodes.
  typedef std::unordered_map<std::string, typename Graph::Node> InvOrigLabelNodeMap;

    unsigned _numSpecies;
    unsigned _seedSize;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution;
	Search();
	~Search(){}
	void test(LayerGraph&,NetworkPool&);
	bool sampleSubNet(SubNet&,LayerGraph&,NetworkPool&,std::discrete_distribution<int>&);
	int sampleStringElement(int);
	bool sampleKSpine(Node&,K_Spine&,LayerGraph&,NetworkPool&);
	bool expandspine(K_Spine,K_Spine&,std::vector<Node>,Node,LayerGraph&,NetworkPool&,unsigned);
	void verifyspine(LayerGraph&,NetworkPool&);
	bool checkConnection(SubNet&,LayerGraph&,NetworkPool&);
	
};

template<typename NP, typename SN, typename LG>
Search<NP,SN,LG>::Search()
:generator(std::chrono::system_clock::now().time_since_epoch().count())
,distribution(0,10000)
{
	_numSpecies=5;
	_seedSize=3;
}

template<typename NP, typename SN, typename LG>
void
Search<NP,SN,LG>::test(LayerGraph& layergraph,NetworkPool& networks)
{
	verifyspine(layergraph,networks);
	std::discrete_distribution<int> discrete(layergraph.density.begin(),layergraph.density.end());
	SubNet mysubnet;
	if(!sampleSubNet(mysubnet,layergraph,networks,discrete) &&
	   g_verbosity>=VERBOSE_NON_ESSENTIAL)
	{
		std::cerr << "Sampling of subnet failed!" << std::endl;
	}
}

/// Randomly sample a d subnet from G_h
template<typename NP, typename SN, typename LG>
bool
Search<NP,SN,LG>::sampleSubNet(SubNet& subnet, LayerGraph& layergraph, NetworkPool& networks,std::discrete_distribution<int>& discrete)
{
	/// Randomly sample a k-spine containing node.
	int dice_roll;
	Node node;
	for(unsigned i=0;i<_seedSize;++i)
	{
		dice_roll = discrete(generator);
		node = layergraph.validnodes[dice_roll];
		K_Spine pspine;
	    if(!sampleKSpine(node,pspine,layergraph,networks))
	    {
			std::cerr <<"Invalid sample node!"<<std::endl;return false;
		}

		/// Output this sample.
		if(g_verbosity >= VERBOSE_NON_ESSENTIAL)
		{
			for(unsigned j=0;j<_numSpecies;++j)
				std::cout << layergraph.node2label[pspine.data[j]]<<" ";
			std::cout << std::endl;
		}
	    subnet.net_spines.push_back(pspine);
	}
	//subnet.induceSubgraphs();
	return checkConnection(subnet,layergraph,networks);
}

/// Collect these nodes which can conduct a successful sample of k-spine.
template<typename NP, typename SN, typename LG>
void
Search<NP,SN,LG>::verifyspine(LayerGraph& layergraph,NetworkPool& networks)
{
	for(NodeIt node(layergraph.graph); node!=lemon::INVALID; ++node)
	{
		K_Spine pspine;
		if(sampleKSpine(node,pspine,layergraph,networks))
		{
			layergraph.setConfiguration(node);
		}
	}
}

template<typename NP, typename SN, typename LG>
bool
Search<NP,SN,LG>::checkConnection(SubNet& subnet,LayerGraph& layergraph,NetworkPool& networks)
{
	for(unsigned i=0;i<_numSpecies;++i)
	{
		std::vector<std::string> nodeset;
		for(unsigned j=0;j<_seedSize;++j)
		{
			std::string element=layergraph.node2label[subnet.net_spines[j].data[i]];
			if(find(nodeset.begin(),nodeset.end(),element)!=nodeset.end())continue;
			nodeset.push_back(element);
		}
		if(!networks.subnetworkConnection(nodeset,i)) return false;
	}
	return true;
}

template<typename NP, typename SN, typename LG>
bool
Search<NP,SN,LG>::sampleKSpine(Node& node,K_Spine& pspine,LayerGraph& layergraph,NetworkPool& networks)
{
	std::vector<Node> candidates;//
	K_Spine spine;
	unsigned k=_numSpecies;
	unsigned host=networks.getHost(layergraph.node2label[node]);
	spine.data[host]=node;
	spine.states[host]=true;
	if(g_verbosity==VERBOSE_DEBUG)
	std::cerr << layergraph.node2label[node] << std::endl;
	
	for(IncEdgeIt it(layergraph.graph,node);it!=lemon::INVALID;++it)
	{
		Node rnode=layergraph.graph.runningNode(it);
		host=networks.getHost(layergraph.node2label[rnode]);
		if(g_verbosity==VERBOSE_DEBUG)
		std::cerr << layergraph.node2label[rnode] << std::endl;
		if(spine.states[host])continue;
		if(node < rnode) candidates.push_back(rnode);
	}
	
	return expandspine(spine,pspine,candidates, node,layergraph,networks,k);
}

template<typename NP, typename SN, typename LG>
int
Search<NP,SN,LG>::sampleStringElement(int up)
{
    int dice_roll = distribution(generator)%(up);
    return dice_roll;
}
template<typename NP, typename SN, typename LG>
bool
Search<NP,SN,LG>::expandspine(	K_Spine spine
								, K_Spine& pspine
								, std::vector<Node> candidates
								, Node node					
								, LayerGraph& layergraph
								, NetworkPool& networks
								, unsigned k)
{

	if(0==--k)
	{
		pspine=spine;
		return true;
	}

	while(!candidates.empty())
	{
		unsigned host;
		int size_Candidates=candidates.size();
		typename std::vector<Node>::iterator in=candidates.begin()+sampleStringElement(size_Candidates);
		Node w = *in;
		candidates.erase(in);
		std::vector<Node> secondCandidates;//V'extention
		std::vector<Node> exclCandidates;//ExclCandidates  V'UN(V')
		/// Assign V'UN(V') to exclCandidates.
		for(unsigned i=0;i<_numSpecies;i++)
		{
			if(!spine.states[i])continue;
			Node spinenode=spine.data[i];
			exclCandidates.push_back(spinenode);
	
			for(IncEdgeIt it(layergraph.graph,spinenode);it!=lemon::INVALID;++it)
			{
				Node rnode=layergraph.graph.runningNode(it);
				host=networks.getHost(layergraph.node2label[rnode]);
				if(spine.states[host])continue;/// If yes, it's not a neighbor.			
				if( std::find(exclCandidates.begin(), exclCandidates.end(), rnode)
				    != exclCandidates.end())continue;
				exclCandidates.push_back(rnode);			
			}
		}
		/// Assign new value to secondCandidates.
		for(IncEdgeIt it(layergraph.graph,w);it!=lemon::INVALID;++it)
		{
			Node rnode=layergraph.graph.runningNode(it);
			host=networks.getHost(layergraph.node2label[rnode]);
			if(spine.states[host])continue;
			if( std::find(exclCandidates.begin(), exclCandidates.end(), rnode)
				    != exclCandidates.end())continue;
			if (node < rnode) secondCandidates.push_back(rnode);				
		}
		K_Spine newspine(spine);
		host=networks.getHost(layergraph.node2label[w]);
		if(g_verbosity==VERBOSE_DEBUG)
		std::cerr << layergraph.node2label[w] << std::endl;
		newspine.data[host]=w;
		newspine.states[host]=true;
		for(unsigned i=0;i<candidates.size();++i)
		{
			Node rnode=candidates[i];
			host=networks.getHost(layergraph.node2label[rnode]);
			if(newspine.states[host])continue;
			secondCandidates.push_back(rnode);
		}
		if(expandspine(newspine, pspine, secondCandidates, node, layergraph, networks, k))return true;
	}
	return false;
}
