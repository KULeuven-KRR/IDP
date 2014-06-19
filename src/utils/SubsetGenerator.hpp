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

/**
 * Class to generate all possible subsets of a given set, from smallest to largest sets, with the possibility to stop at a maximum size.
 * The implementation required a complex algorithm as the aim is specifically to allow generation of all subsets of small size, independent of size of the total set.
 */
template<class T, class Comparator>
class SubsetGenerator {
private:
	std::vector<T> totalset; 	// Input set
	uint maxsubsetsize; 		// The size of the largest sets to generate

	// A "level" is the set of all subsets of a size "level"
	// At each level, currIndices has size level and each element in the vector is an index into totalset
	// Generation happens by incrementing the last element until all have been seen, then increment
	// the next-to-last by one, restart the last and continue with it, ...
	std::vector<uint> currIndices;
	int minimalindex; // Indicates the lowest index to be added to the next level. Level = minimalindex+1

	std::set<T, Comparator> currentsubset;

private:
	bool atLevelEnd() const {
		return (currIndices.size() == 0 // We just returned the empty set OR
						|| (currIndices[minimalindex] == totalset.size() - 1 // we just returned the subset consists of all last elements in totalset
								&& currIndices[0] == currIndices[minimalindex] - minimalindex));
	}

public:
	SubsetGenerator(const std::vector<T>& totalset, int maxsubsetsize = -1)
			: 	totalset(totalset),
			  	maxsubsetsize(maxsubsetsize < 0 ? totalset.size() : maxsubsetsize),
				minimalindex(-1) {
	}

	const std::set<T, Comparator>& getCurrentSubset() const {
		return currentsubset;
	}

	bool hasNextSubset() const {
		return not atLevelEnd() || (currIndices.size() <= maxsubsetsize);
	}

	void nextSubset() {
		if (not hasNextSubset()) {
			throw IdpException("No next subset available");
		}

		if (atLevelEnd()) {
			minimalindex++;
			currIndices.push_back(0);
			currentsubset.clear();
			for (uint i = 0; i < currIndices.size(); ++i) {
				currIndices[i] = i;
				currentsubset.insert(totalset[i]);
			}
			return;
		}

		if (currIndices[minimalindex] < totalset.size() - 1) {
			currentsubset.erase(totalset[currIndices[minimalindex]]);
			currIndices[minimalindex]++;
			currentsubset.insert(totalset[currIndices[minimalindex]]);
			return;
		}

		int target = -1;
		for (uint i = 0; i < currIndices.size(); ++i) {
			if (target != -1) {
				currentsubset.erase(totalset[currIndices[i]]);
				currIndices[i] = target;
				currentsubset.insert(totalset[currIndices[i]]);
				target++;
				continue;
			}
			auto maxfornext = totalset.size() - (minimalindex - i);
			if (i + 1 < currIndices.size() && currIndices[i + 1] != maxfornext) {
				continue;
			}
			currentsubset.erase(totalset[currIndices[i]]);
			currIndices[i]++;
			currentsubset.insert(totalset[currIndices[i]]);
			target = currIndices[i] + 1;
		}
	}
};
