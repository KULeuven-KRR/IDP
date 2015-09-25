#pragma once

#include "visitors/TheoryMutatingVisitor.hpp"
#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"

#include <vector>
#include <map>
#include <theory/theory.hpp>
using namespace std;


class AddMarkers: public TheoryMutatingVisitor {
    VISITORFRIENDS()

    vector<Variable*> vars;
    vector<Predicate*> newpreds;
    map<Predicate*, pair<vector<Variable*>, Formula*>> marker2formula;
    map<Predicate*, pair<vector<Variable*>, Rule*>> marker2rule;
    map<Rule*, DefId> rule2defid;
    map<Predicate*, ParseInfo> marker2parseinfo;
    map<Predicate*, PFSymbol*> falseDefMarkers;

public:
    Theory* execute(Theory* t);

    ~AddMarkers() ;

    const vector<Predicate*>& getMarkers() const ;
    vector<TheoryComponent*> getComponentsFromMarkers(const vector<DomainAtom>& pfs) const ;
protected:
    Formula* addMarker(Formula* f) ;
    Formula* visit(PredForm* pf) ;
    Formula* visit(AggForm* f) ;
    Formula* visit(EqChainForm* f) ;
    Formula* visit(EquivForm* f) ;

    template<class Object>
    map<Variable*, const DomainElement*> getVarInstantiation(const pair<vector<Variable*>, Object*>& varAndForm, const DomainAtom& pf, stringstream& ss) const ;

    Formula* visit(BoolForm* bf) ;
    Formula* visit(QuantForm* q) ;
    void addRuleMarker(int defID, Rule* r, bool falsedefrule) ;
    Definition* visit(Definition* d) ;

    virtual AbstractGroundTheory* visit(AbstractGroundTheory*) ;
    virtual GroundDefinition* visit(GroundDefinition*) ;
    virtual GroundRule* visit(AggGroundRule*) ;
    virtual GroundRule* visit(PCGroundRule*) ;
    virtual Rule* visit(Rule*) ;
    virtual FixpDef* visit(FixpDef*) ;
    virtual Term* visit(VarTerm*) ;
    virtual Term* visit(FuncTerm*) ;
    virtual Term* visit(DomainTerm*) ;
    virtual Term* visit(AggTerm*) ;
    virtual EnumSetExpr* visit(EnumSetExpr*) ;
    virtual QuantSetExpr* visit(QuantSetExpr*) ;
};