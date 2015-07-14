#pragma once
class DomainAtom;

std::vector<DomainAtom> minimizeAssumps(AbstractTheory *newtheory, Structure *s, MXAssumptions markers);
