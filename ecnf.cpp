/************************************
	ecnf.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ecnf.hpp"
#include <iostream>

/******************
	ECNF OUTPUT 
******************/

outputECNF::outputECNF(FILE* f) : _out(f) {

}
outputECNF::~outputECNF(){

}

void outputECNF::outputinit(GroundFeatures* gf){
	fprintf(_out,"p ecnf");
	if(gf->_containsDefinitions) fprintf(_out," def");
	if(gf->_containsAggregates) fprintf(_out," aggr");
	fprintf(_out,"\n");
}

void outputECNF::outputend() { }

void outputECNF::outputunitclause(int l){
	vector<int> v;
	v.push_back(l);
	outputclause(v);
}

void outputECNF::outputclause(const vector<int>& vi){
	for(unsigned int n = 0; n < vi.size(); ++n){
		fprintf(_out,"%d ",vi[n]);
	}
	fprintf(_out,"0\n");
}

void outputECNF::outputunitrule(int h, int b) {
	// TODO
	// vector<int> v;
	//if(!Ground::isTrue(b)){
//		v.push_back(b);
//	}
//	outputrule(h, v, true);
}

void outputECNF::outputrule(int h, const vector<int>& b, bool c){
	if(c) {
		fprintf(_out,"C %d ",h);
	}
	else {
		fprintf(_out,"D %d ",h);
	}
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");
}

void outputECNF::outputunitfdrule(int d, int h, int b) {
	// TODO
	/*vector<int> v;
	if(!Ground::isTrue(b)){
		v.push_back(b);
	}
	outputfdrule(d, h, v, true);*/
}

void outputECNF::outputfdrule(int d, int h, const vector<int>& b, bool c) {
	if(c) {
		fprintf(_out,"FD C %d %d ",d,h);
	}
	else {
		fprintf(_out,"FD D %d %d ",d,h);
	}
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");
}

void outputECNF::outputmax(int h, bool defined, int setid, bool lowerthan, int bound) {
	/*fprintf(_out,"Max %d " ,h);
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");*/
}

void outputECNF::outputmin(int h, bool defined, int setid, bool lowerthan, int bound){
	/*fprintf(_out,"Min %d " ,h);
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");*/
}

void outputECNF::outputsum(int h, bool defined, int setid, bool lowerthan, int bound){
	/*fprintf(_out,"Sum %d " ,h);
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");*/
}

void outputECNF::outputprod(int h, bool defined, int setid, bool lowerthan, int bound){
	/*fprintf(_out,"Prod %d " ,h);
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");*/
}

void outputECNF::outputcard(int h, bool defined, int setid, bool lowerthan, int bound){
	/*fprintf(_out,"Card %d " ,h);
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");*/
}

//TODO important: no longer supported by solver
void outputECNF::outputeu(const vector<int>& b) {
	fprintf(_out,"EU ");
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");
}

//TODO important: no longer supported by solver
void outputECNF::outputamo(const vector<int>& b) {
	fprintf(_out,"AMO ");
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%d ",b[n]);
	}
	fprintf(_out,"0\n");
}

void outputECNF::outputset(int s, const vector<int>& sets) {
	fprintf(_out,"Set %d ",s);
	for(unsigned int n = 0; n < sets.size(); ++n)
		fprintf(_out,"%d ",sets[n]);
	fprintf(_out,"0\n");
}

void outputECNF::outputwset(int s, const vector<int>& sets, const vector<int>& weights) {
	fprintf(_out,"WSet %d ",s);
	for(unsigned int n = 0; n < sets.size(); ++n)
		fprintf(_out,"%d=%d ",sets[n],weights[n]);
	fprintf(_out,"0\n");
}

void outputECNF::outputfixpdef(int d, const vector<int>& sd, bool l) {
	if(l) fprintf(_out,"L %d ",d);
	else fprintf(_out,"G %d ",d);
	for(unsigned int n = 0; n < sd.size(); ++n) fprintf(_out,"%d ",sd[n]);
	fprintf(_out,"0\n");
}

void outputECNF::outputunsat(){
	fprintf(_out,"1 0\n-1 0\n");
}

/****************************
	HUMAN READABLE OUTPUT 
****************************/

outputHR::outputHR(FILE* f) : _out(f) { }
outputHR::~outputHR(){ }

string toHR(int l) {
	// TODO
/*	int al = abs(l);
	if(!l) {
		return "True";
	}
	else if(al >= Data::offset()) {
		string str = (l > 0) ?  "" : "~";
		str = str + "Ts_" + itos(al);
		return str;
	}
	else {
		PFSymbol* pfs = Data::lit2Symb(al);
		if(pfs->ispred()) {
			Predicate* p = dynamic_cast<Predicate*>(pfs);
			string str = (l > 0) ? "" : "~";
			str = str + (p->name()).substr(0,(p->name()).find('/'));
			if(p->arity()) {
				al = al - p->offset();
				vector<string> a(p->arity());
				for(unsigned int n = p->arity() - 1; n >= 0; --n) {
					int cs = p->sort(n)->size();
					int curr = al % cs;
					a[n] = p->sort(n)->inter()->getEl(curr);
					al = (al - curr) / cs;
				}
				assert(!al);
				str = str + '(' + a[0];
				for(unsigned int n = 1; n < a.size(); ++n) str = str + ',' + a[n];
				str = str + ')';
			}
			return str;
		}
		else {
			Function* f = dynamic_cast<Function*>(pfs);
			string str = (f->name()).substr(0,(f->name()).find('/'));
			al = al - f->offset();
			int rs = f->outsort()->size();
			int rv = al % rs;
			al = (al - rv) / rs;
			if(f->arity()) {
				vector<string> a(f->arity());
				for(unsigned int n = f->arity() - 1; n >= 0; --n) {
					int cs = f->insort(n)->size();
					int curr = al % cs;
					a[n] = f->insort(n)->inter()->getEl(curr);
					al = (al - curr) / cs;
				}
				assert(!al);
				str = str + '(' + a[0];
				for(unsigned int n = 1; n < a.size(); ++n) str = str + ',' + a[n];
				str = str + ')';
			}
			str = str + ((l > 0) ? "=" : "~=") + f->outsort()->inter()->getEl(rv);
			return str;
		}
	}
	*/
	return "";
}

void outputHR::outputinit(GroundFeatures*){}
void outputHR::outputend(){}
void outputHR::outputunitclause(int l){
	fprintf(_out,"%s.\n",toHR(l).c_str());
}

void outputHR::outputclause(const vector<int>& vi){
	if(vi.empty()) fprintf(_out,"False. // Inconsistent ground theory\n");
	fprintf(_out,"%s",toHR(vi[0]).c_str());
	for(unsigned int n = 1; n < vi.size(); ++n){
		fprintf(_out," | %s",toHR(vi[n]).c_str());
	}
	fprintf(_out,".\n");
}

void outputHR::outputunitrule(int h, int b) {
	// TODO
/*	if(Ground::isTrue(b)){
		fprintf(_out,"%s <- True.\n",toHR(h).c_str());
	}
	else{
		fprintf(_out,"%s <- %s.\n",toHR(h).c_str(),toHR(b).c_str());
	}*/
}

void outputHR::outputunitfdrule(int d, int h, int b) {
	fprintf(_out,"In definition nr. %d: ",d);
	outputunitrule(h,b);
}

void outputHR::outputrule(int h, const vector<int>& b, bool c){
	fprintf(_out,"%s <- %s",toHR(h).c_str(),toHR(b[0]).c_str());
	if(c) {
		for(unsigned int n = 1; n < b.size(); ++n){
			fprintf(_out," & %s",toHR(b[n]).c_str());
		}
	}
	else {
		for(unsigned int n = 1; n < b.size(); ++n){
			fprintf(_out," | %s",toHR(b[n]).c_str());
		}
	}
	fprintf(_out,".\n");
}

void outputHR::outputfdrule(int d, int h, const vector<int>& b, bool c){
	fprintf(_out,"In definition nr. %d: ",d);
	outputrule(h,b,c);
}

void outputHR::outputmax(int h, bool defined, int setid, bool lowerthan, int bound) {
	outputaggregate(h, defined, setid, lowerthan, bound, "maximum");
}

void outputHR::outputmin(int h, bool defined, int setid, bool lowerthan, int bound){
	outputaggregate(h, defined, setid, lowerthan, bound, "minimum");
}

void outputHR::outputsum(int h, bool defined, int setid, bool lowerthan, int bound){
	outputaggregate(h, defined, setid, lowerthan, bound, "sum");
}

void outputHR::outputprod(int h, bool defined, int setid, bool lowerthan, int bound){
	outputaggregate(h, defined, setid, lowerthan, bound, "product");
}

void outputHR::outputcard(int h, bool defined, int setid, bool lowerthan, int bound){
	outputaggregate(h, defined, setid, lowerthan, bound, "cardinality");
}

void outputHR::outputaggregate(int h, bool defined, int setid, bool lowerthan, int bound, const char* type){
	if(defined){
		fprintf(_out,"%s is true iff the %s of set nr. %d %s %d.\n", toHR(h).c_str(), type, setid, lowerthan?"=<":">=", bound);
	}else{
		fprintf(_out,"%s is defined as the %s of set nr. %d %s %d.\n", toHR(h).c_str(), type, setid, lowerthan?"=<":">=", bound);
	}
}

void outputHR::outputeu(const vector<int>& b) {
	fprintf(_out,"There is exactly one true literal in {");
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%s ",toHR(b[n]).c_str());
	}
	fprintf(_out,"}.\n");
}

void outputHR::outputamo(const vector<int>& b) {
	fprintf(_out,"There is at most one true literal in {");
	for(unsigned int n = 0; n < b.size(); ++n){
		fprintf(_out,"%s ",toHR(b[n]).c_str());
	}
	fprintf(_out,"}.\n");
}

void outputHR::outputset(int s, const vector<int>& sets) {
	fprintf(_out,"Set nr. %d contains the true literals among {",s);
	for(unsigned int n = 0; n < sets.size(); ++n)
		fprintf(_out,"%s ",toHR(sets[n]).c_str());
	fprintf(_out,"}.\n");
}

void outputHR::outputwset(int s, const vector<int>& sets, const vector<int>& weights) {
	fprintf(_out,"Weighted set nr. %d contains the true literals among {",s);
	for(unsigned int n = 0; n < sets.size(); ++n)
		fprintf(_out,"%s=%d ",toHR(sets[n]).c_str(),weights[n]);
	fprintf(_out,"}.\n");
}

void outputHR::outputfixpdef(int d, const vector<int>& sd, bool l) {
	if(l) fprintf(_out,"Least");
	else fprintf(_out,"Greatest");
	fprintf(_out," fixpoint definition nr. %d has subdefinitions ",d);
	for(unsigned int n = 0; n < sd.size(); ++n) fprintf(_out,"%d ",sd[n]);
	fprintf(_out,"\n");
}

void outputHR::outputunsat(){
	fprintf(_out,"False. // Inconsistent ground theory\n");
}


/*******************************
	INTEGRATED SYSTEM OUTPUT
********************************/

void copyToVec(const vector<int>& v, vector<Literal>& v2){
	for(vector<int>::const_iterator i=v.begin(); i<v.end(); i++){
		if(*i<0){
			v2.push_back(Literal(-(*i), true));
		}else{
			v2.push_back(Literal(*i, false));
		}
	}
}

/*outputToSolver::outputToSolver() : _solver(NULL) {
	ECNF_mode modes;
	modes.verbosity = 7;
	_solver = new PCSolver(modes);
}*/
outputToSolver::outputToSolver(PropositionalSolver* solver) : _solver(solver) {}
outputToSolver::~outputToSolver(){ }

void outputToSolver::outputinit(GroundFeatures*){}
void outputToSolver::outputend(){}
void outputToSolver::outputunitclause(int l){
	vector<Literal> v;
	if(l<0){
		v.push_back(Literal(-l, true));
	}else{
		v.push_back(Literal(l, false));
	}
	solver()->addClause(v);
}

void outputToSolver::outputclause(const vector<int>& lits){
	vector<Literal> l;
	copyToVec(lits, l);
	solver()->addClause(l);
}

void outputToSolver::outputunitrule(int h, int b) {
	vector<int> body;
	body.push_back(b);
	outputrule(h, body, true);
}

void outputToSolver::outputunitfdrule(int d, int h, int b) {
	//TODO later
}

void outputToSolver::outputrule(int head, const vector<int>& b, bool c){
	vector<Literal> l;
	if(head<0){
		l.push_back(Literal(-head, true));
	}else{
		l.push_back(Literal(head, false));
	}
	copyToVec(b, l);
	solver()->addRule(c, l);
}

void outputToSolver::outputfdrule(int d, int h, const vector<int>& b, bool c){
	//TODO
}

void outputToSolver::outputmax(int h, bool defined, int setid, bool lowerthan, int bound) {
	solver()->addAggrExpr(Literal(h), setid, bound, lowerthan, MAX, defined);
}

void outputToSolver::outputmin(int h, bool defined, int setid, bool lowerthan, int bound){
	solver()->addAggrExpr(Literal(h), setid, bound, lowerthan, MIN, defined);
}

void outputToSolver::outputsum(int h, bool defined, int setid, bool lowerthan, int bound){
	solver()->addAggrExpr(Literal(h), setid, bound, lowerthan, SUM, defined);
}

void outputToSolver::outputprod(int h, bool defined, int setid, bool lowerthan, int bound){
	solver()->addAggrExpr(Literal(h), setid, bound, lowerthan, PROD, defined);
}

void outputToSolver::outputcard(int h, bool defined, int setid, bool lowerthan, int bound){
	solver()->addAggrExpr(Literal(h), setid, bound, lowerthan, CARD, defined);
}

//Not supported by solver
void outputToSolver::outputeu(const vector<int>& b) {
	assert(false);
}

//Not supported by solver
void outputToSolver::outputamo(const vector<int>& b) {
	assert(false);
}

void outputToSolver::outputset(int setid, const vector<int>& lits) {
	vector<Literal> l;
	copyToVec(lits, l);
	solver()->addSet(setid, l);
}

void outputToSolver::outputwset(int setid, const vector<int>& lits, const vector<int>& weights) {
	vector<Literal> l;
	copyToVec(lits, l);
	solver()->addSet(setid, l, weights);
}

void outputToSolver::outputfixpdef(int d, const vector<int>& sd, bool l) {
	//TODO later
}

void outputToSolver::outputunsat(){
	//FIXME this will probably crash
	vector<Literal> l;
	l.push_back(Literal(1));
	solver()->addClause(l);
	vector<Literal> l2;
	l2.push_back(Literal(1, true));
	solver()->addClause(l2);
}

/*****************************
	Internal encf theories
*****************************/

//This should change into printer->print(theory)
void EcnfTheory::print(GroundPrinter* p) {
	p->outputinit(&_features);
	for(unsigned int n = 0; n < _clauses.size(); ++n) {
		p->outputclause(_clauses[n]);
	}
	// TODO: definitions, aggregates, ...
}
