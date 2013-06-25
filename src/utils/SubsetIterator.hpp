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

#include <vector>
#include <set>

template<class T, class Comparator>
class SubsetGenerator{
private:
	std::vector<T> totalset;
	std::vector<uint> level2index;
	bool atstart;
	uint minimalindex; // Indicates the lowest index to be added to the next level. The size of index is minimalindex+1
	uint maxsize;
	std::set<T,Comparator> currentsubset;

private:
	bool atLevelEnd() const {
		return not atstart && (level2index.size()==0 || (level2index[minimalindex]==totalset.size()-1 && level2index[0]==level2index[minimalindex]-minimalindex));
	}

public:
	SubsetGenerator(const std::vector<T>& totalset, int maxsubsetsize = -1): totalset(totalset), atstart(true), minimalindex(-1), maxsize(maxsubsetsize<0?totalset.size():maxsubsetsize){

	}

	const std::set<T,Comparator>& getCurrentSubset() const {
		return currentsubset;
	}

	bool hasNextSubset() const {
		auto result = not atLevelEnd() || (level2index.size()<maxsize);
		return result;
	}

	void nextSubset(){
		if(not hasNextSubset()){
			throw IdpException("No next subset available");
		}

		if(atstart){
			atstart = false;
			return;
		}

		if(atLevelEnd()){
			minimalindex++;
			level2index.push_back(0);
			currentsubset.clear();
			for(uint i=0; i<level2index.size(); ++i){
				level2index[i]=i;
				currentsubset.insert(totalset[i]);
			}
			return;
		}

		if(level2index[minimalindex]<totalset.size()-1){
			currentsubset.erase(totalset[level2index[minimalindex]]);
			level2index[minimalindex]++;
			currentsubset.insert(totalset[level2index[minimalindex]]);
			return;
		}

		int target = -1;
		for(uint i=0; i<level2index.size(); ++i){
			if(target!=-1){
				currentsubset.erase(totalset[level2index[i]]);
				level2index[i]=target;
				currentsubset.insert(totalset[level2index[i]]);
				target++;
				continue;
			}
			auto maxfornext = totalset.size()-(minimalindex-i);
			if(i+1<level2index.size() && level2index[i+1]!=maxfornext){
				continue;
			}
			currentsubset.erase(totalset[level2index[i]]);
			level2index[i]++;
			currentsubset.insert(totalset[level2index[i]]);
			target = level2index[i]+1;
		}
	}
};

template<class T, class C>
class SubsetIterator{
	SubsetGenerator<T,C>& generator;
	bool atend;
public:
	SubsetIterator(SubsetGenerator<T,C>& generator): generator(generator), atend(not generator.hasNextSubset()){
	}
	SubsetIterator(SubsetGenerator<T,C>& generator, bool atend): generator(generator), atend(atend){
	}
	void operator++(){
		if(not generator.hasNextSubset()){
			atend = true;
		}else{
			generator.nextSubset();
		}
	}
	std::set<T,C> operator*(){
		return generator.getCurrentSubset();
	}

	bool operator==(SubsetIterator<T,C>& rhs){
		return atend == rhs.atend;
	}
	bool operator!=(SubsetIterator<T,C>& rhs){
		return not (atend == rhs.atend);
	}
};

template<class T,class C>
SubsetIterator<T,C> begin(SubsetGenerator<T,C>& subset){
	return SubsetIterator<T,C>(subset);
}
template<class T, class C>
SubsetIterator<T,C> end(SubsetGenerator<T,C>& subset){
	return SubsetIterator<T,C>(subset, true);
}
