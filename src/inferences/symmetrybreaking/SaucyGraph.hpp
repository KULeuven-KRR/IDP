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

#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include "Symmetry.hpp"

class DomainElement;
class PFSymbol;
class PredTable;
class Structure;
class Symmetry;
struct saucy_graph;

namespace saucy_ {

struct TupleNode {
  TupleNode(PFSymbol* s, unsigned int val):symb(s),truth_value(val){}
  
  PFSymbol* symb;
  unsigned int truth_value; // true, false, unknown
  std::vector<const DomainElement*> args;
  
  size_t getHash() const{
    size_t seed = (size_t) symb + truth_value;
    for (auto x : args) {
      seed ^= (size_t) x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
  
  bool equals(const TupleNode& other) const{
    if(symb!=other.symb || truth_value!= other.truth_value || args.size()!=other.args.size()){
      return false;
    }
    for(unsigned int i=0; i<args.size(); ++i){
      if(args[i]!=other.args[i]){
        return false;
      }
    }
    return true;
  }
};

struct TNHash {
	size_t operator()(const TupleNode& tn) const {
		return tn.getHash();
	}
};

struct TNEqual {
	bool operator()(const TupleNode& a, const TupleNode& b) const {
		return a.equals(b);
	}
};

class Graph {
private:
  InterchangeabilitySet* ics;
  
  unsigned int highestColor = -1;
  std::vector<unsigned int> color; // color[i] is the color of the ith node
  std::vector<std::pair<unsigned int,unsigned int> > edges;
  
  std::unordered_map<const DomainElement*, unsigned int> domEl2Node; // denotes the nodes associated with this domain element
  std::unordered_map<unsigned int, const DomainElement*> node2DomEl; // denotes the nodes associated with this domain element
  std::vector<std::unordered_map<const DomainElement*, unsigned int> > domElArgNodes; // denotes the argument nodes associated with this domain element domEl2Node[i][de] represents the node for de coupled to argument i
  std::unordered_map<TupleNode, unsigned int, TNHash, TNEqual> tupleColors; // denotes the different colors for tupleNodes, e.g. maps tuple P(_,a)=t to the appropriate color tupleNodes[P(_,a)=t]
  
  unsigned int getNextNode();
  unsigned int getNextColor();
  
  void addTuple(PFSymbol* symb, const std::vector<const DomainElement*>& args, unsigned int truth_value, std::set<unsigned int>& argpos);
  void addPredTable(PFSymbol* symb, const PredTable* pt, unsigned int truth_value, std::set<unsigned int>& argpos);
  
  saucy_graph* sg;
  
  void createSaucy();
  void freeSaucy();
  
public:
  Graph(InterchangeabilitySet* ics);
  
  void addInterpretations(const Structure* s);
  void runSaucy(std::vector<Symmetry*>& out);
};

}