/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "saucy.hpp"
#include "IncludeComponents.hpp"
#include "SaucyGraph.hpp"

namespace saucy_ {
  
Graph::Graph(InterchangeabilitySet* ic):ics(ic){
  // first get max arity+1 of symbols
  unsigned int maxArity=0;
  for(auto sa: ics->symbargs){
    PFSymbol* symb = sa.first;
    if(maxArity < symb->nrSorts()){
      maxArity = symb->nrSorts();
    }
  }
  domElArgNodes.resize(maxArity);
  
  // enumerate domain
  std::unordered_set<const DomainElement*> domain;
  ics->getDomain(domain, true); // includes elements occurring as constants
  Assert(domain.size()>=ics->occursAsConstant.size());
  
  // create domain element nodes and corresponding argument nodes
  highestColor = maxArity;
  for(auto de: domain){
    auto deNode = getNextNode();
    domEl2Node[de]=deNode;
    node2DomEl[deNode]=de;
    color[deNode]=0; // give each domElNode the same color
    
    for(unsigned int i=0; i<maxArity; ++i){
      auto deArgNode = getNextNode();
      domElArgNodes[i][de]=deArgNode;
      color[deArgNode]=i+1; // give each domElArgNode with the same argument the same color
      edges.push_back({deNode,deArgNode}); // put an edge between each domElArgNode and its corresponding domElNode
    }
  }
  
  // fix colors for those domain elements occurring as constants
  for(auto de: ics->occursAsConstant){
    color[domEl2Node[de]]=getNextColor(); // domain elements occurring in theory should have unique color so they never take part in an isomorphism
  }
  
  Assert(color.size()==(maxArity+1)*domain.size());
  
  if(ics->occursAsConstant.size()==domain.size()){
    // no more nodes with color 0, introduce dummy node for Saucy
    unsigned int dummy = getNextNode();
    color[dummy]=0;
  }
}
  
unsigned int Graph::getNextNode(){
  color.push_back(0);
  return color.size()-1;
}

unsigned int Graph::getNextColor(){
  ++highestColor;
  return highestColor;
}

void Graph::addTuple(PFSymbol* symb, const std::vector<const DomainElement*>& args, unsigned int truth_value, std::set<unsigned int>& argpos){  
  unsigned int newnode = getNextNode();
  TupleNode tn(symb,truth_value);
  for(unsigned int i=0; i<args.size(); ++i){
    if(argpos.count(i)){
      edges.push_back({newnode,domElArgNodes[i][args[i]]}); // edge between tuplenode and domelargnode
    }else{
      tn.args.push_back(args[i]);
    }
  }
  if(tupleColors.count(tn)==0){
    tupleColors[tn]=getNextColor();
  }
  color[newnode]=tupleColors[tn];
}

void Graph::addPredTable(PFSymbol* symb, const PredTable* pt, unsigned int truthval, std::set<unsigned int>& argpos){
  if(!pt->finite()){
    throw IdpException("Symmetry breaking does not support infinite interpretations.");
  }
  auto ptIt = pt->begin();
  while(!ptIt.isAtEnd()){
    addTuple(symb,*ptIt,truthval,argpos);
    ++ptIt;
  }
}

void Graph::addInterpretations(const Structure* s){
  for(auto sa: ics->symbargs){
    PFSymbol* symb = sa.first;
    std::set<unsigned int>& argpos = sa.second;
    PredInter* pi = s->inter(symb);
    addPredTable(symb,pi->ct(),1,argpos);
    if(!pi->approxTwoValued()){
      addPredTable(symb,pi->cf(),2,argpos); // TODO: find out how to extract the two finite tables instead of always ct and cf
    }
  }
}

// This method is given to Saucy as a polymorphic consumer of the detected generator permutations

std::vector<std::unordered_map<unsigned int, unsigned int> > generators;

int addPermutation(int n, const int *ct_perm, int nsupp, int *support, void *arg) {
  if (n == 0 || nsupp == 0) {
    return 1;
  }
  
  std::unordered_map<unsigned int, unsigned int> generator;
  generators.push_back(generator);
  for(int i=0; i<n; ++i) {
    if(i!=ct_perm[i]){
      generators.back()[i]=ct_perm[i];
    }
  }
  
  return 1;
}

void Graph::runSaucy(std::vector<Symmetry*>& out){
  ArgPosSet threeval;
  ics->getThreeValuedArgPositions(threeval);
  if(threeval.symbols.size()==0){
    return; // no symbols to break symmetry for
  }
  
  createSaucy();
  if (getOption(IntType::VERBOSE_SYMMETRY) > 2) {
    std::clog << "saucy nodes: " << sg->n << std::endl;
    std::clog << "saucy edges: " << sg->e << std::endl;
  }
  
  struct saucy* s = saucy_alloc(sg->n);
  struct saucy_stats stats;
  saucy_search(s, sg, 0, addPermutation, 0, &stats);
  saucy_free(s);
  
  for(auto gen: generators){
    Symmetry* newSym = new Symmetry(threeval);
    for(auto paar: gen){
      if(node2DomEl.count(paar.first)){ // else the node permuted is not a domain node
        newSym->addImage(node2DomEl[paar.first], node2DomEl[paar.second]);
      }
    }
    out.push_back(newSym);
    if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
        newSym->print(std::clog);
    }
  }
  
  freeSaucy();
}

void Graph::freeSaucy(){
  free(sg->adj);
  free(sg->edg);
  free(sg->colors);
  free(sg);
  generators.clear();
}

void Graph::createSaucy(){
  sg = (saucy_graph*) malloc(sizeof (struct saucy_graph));
  
  unsigned int n = color.size();
  // set the colors right
  sg->colors = (int*) malloc(n * sizeof (int));
  for(unsigned int i=0; i<n; ++i){
    sg->colors[i]=color[i];
  }
  
  std::vector<std::vector<uint> > neighbours(n);
  for(auto edge: edges){
    neighbours[edge.first].push_back(edge.second);
    neighbours[edge.second].push_back(edge.first);
  }
  
    // now count the number of neighboring nodes
  sg->adj = (int*) malloc((n + 1) * sizeof (int));
  sg->adj[0] = 0;
  int ctr = 0;
  for (auto nblist : neighbours) {
    sg->adj[ctr + 1] = sg->adj[ctr] + nblist.size();
    ++ctr;
  }

  // finally, initialize the lists of neighboring nodes, C-style
  sg->edg = (int*) malloc(sg->adj[n] * sizeof (int));
  ctr = 0;
  for (auto nblist : neighbours) {
    for (auto l : nblist) {
      sg->edg[ctr] = l;
      ++ctr;
    }
  }

  sg->n = n;
  sg->e = sg->adj[n] / 2;
}

}