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

class DomainElement;
class PFSymbol;

namespace saucy {

struct TupleNode {
  PFSymbol* symb;
  std::vector<DomainElement*> args;
  
  size_t getHash(){
    size_t seed = symb;
    for (auto x : args) {
      seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
  
  bool equals(TupelNode& other){
    if(symb!=other.symb || args.size()!=other.args.size()){
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

class Graph {
private:
  std::unordered_map<PFSymbol*, std::vector<bool> > symbolargs; // symbolargs[symb][i] denotes whether (symb,i) is part of the set of symbol arguments
  
  unsigned int highestNode = 0;
  std::unordered_map<TupleNode, unsigned int> tupleNodes;
  std::vector<std::unordered_map<DomainElement*, unsigned int> > domElNodes;
  std::vector<std::pair<unsigned int,unsigned int> > edges;
  
  unsigned int highestColor = 0;
  std::unordered_map<unsigned int, unsigned int> color;
  
public:
  Graph(std::unordered_map<PFSymbol*, std::unordered_set<unsigned int>* >& symbargs);
  
  void addTupleNode(PFSymbol* symb, std::vector<DomainElement*>& args);
};

}