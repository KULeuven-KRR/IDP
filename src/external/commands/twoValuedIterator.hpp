/* 
 * File:   twoValuedIterator.hpp
 * Author: rupsbant
 *
 * Created on December 6, 2014, 10:22 PM
 */

#pragma once

#include <iostream>
#include "commandinterface.hpp"

#include "inferences/makeTwoValued/TwoValuedStructureIterator.hpp"
#include "inferences/modelexpansion/LuaTraceMonitor.hpp"
#include "utils/LogAction.hpp"
#include "lua/luaconnection.hpp"

InternalArgument createIteratorCommand(Structure* structure) {
    LuaTraceMonitor* tracer = NULL;
    if (getOption(BoolType::TRACE)) {
        tracer = LuaConnection::getLuaTraceMonitor();
    }
    auto iterator = new TwoValuedStructureIterator(structure);
    InternalArgument ia(iterator);
    //iterator->init();
    return ia;
}

typedef TypedInference<LIST(Structure*) > TwoValuedIteratorBase;

class TwoValuedIterator : public TwoValuedIteratorBase {
public:

    TwoValuedIterator()
    : TwoValuedIteratorBase("createTwoValuedIterator",
    "Create an iterator generating 2-valued models of the theory which are more precise than the given structure.", false) {
        setNameSpace(getInternalNamespaceName());
    }

    InternalArgument execute(const std::vector<InternalArgument>& args) const {
        auto ret = createIteratorCommand(get<0>(args));
        return ret;
    }
};
