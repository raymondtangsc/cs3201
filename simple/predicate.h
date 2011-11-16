/*
 * CS3201 Simple Static Analyzer
 * Copyright (C) 2011 Soares Chen Ruo Fei
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include "simple/condition_set.h"

namespace simple {

class SimplePredicate {
  public:
    virtual const ConditionSet& global_set() = 0;
    virtual void filter_set(ConditionSet& conditions) = 0;

    virtual ~SimplePredicate() { }
};

class PredicateTable {
  public:
    virtual std::shared_ptr<SimplePredicate> get_predicate(const std::string& name) = 0;

    virtual ~PredicateTable() { }
};

class PredicateFactory {
  public:
    virtual std::shared_ptr<PredicateTable> make_predicates(SimpleRoot ast) = 0;

    virtual ~PredicateFactory() { }
};


}
