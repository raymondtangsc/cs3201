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

#include <memory>
#include <string>
#include <map>

namespace simple {

class ProcAst;
class StatementAst;
class ContainerAst;
class ConditionalAst;
class WhileAst;
class AssignmentAst;
class CallAst;
class ExprAst;
class VariableAst;
class ConstAst;
class BinaryOpAst;
class ContainerVisitor;
class StatementVisitor;
class ExprVisitor;

typedef std::map<int, StatementAst*> LineTable;

class InconsistentAstError : public std::exception { };

/*
 * SimpleVariable is just a convenient class to indicate in function
 * parameters that it is accepting a simple variable instead of a string 
 * that can mean anything.
 */
class SimpleVariable {
  public:
    SimpleVariable() : _name() { }
    SimpleVariable(const std::string& name) : _name(name) { }
    SimpleVariable(const SimpleVariable& other) : _name(other._name) { }

    std::string get_name() {
        return _name;
    }

    bool equals(const SimpleVariable& other) const {
        return _name == other._name;
    }

    bool operator ==(const SimpleVariable& other) const {
        return equals(other);
    }

    bool operator <(const SimpleVariable& other) const {
        return _name < other._name;
    }

    bool operator >(const SimpleVariable& other) const {
        return _name > other._name;
    }

    ~SimpleVariable() { }

  private:
    std::string _name;
};

class SimpleConstant {
  public:
    SimpleConstant(int value) : _value(value) { }
    
    SimpleConstant(const SimpleConstant& other) :
        _value(other._value) 
    { }

    int get_int() {
        return _value;
    }

    ~SimpleConstant() { }

  private:
    int _value;
};

/**
 * The ProcAst represents a procedure in the source code. It does not
 * share the same hierarchy with other AST nodes as the CS3201 
 * specification treats procedures differently from other nodes.
 */
class ProcAst {
  public:
    /**
     * Get the name of the procedure as a string.
     */
    virtual std::string get_name() = 0;

    /**
     * Get the first statement node in the given procedure.
     */
    virtual StatementAst* get_statement() = 0;

    virtual ~ProcAst() { }
};

class SimpleRoot {
  public:
    typedef std::map<std::string, std::shared_ptr<ProcAst> > ProcListType;

    class iterator;

    template <typename Iterator>
    SimpleRoot(Iterator it, Iterator end) :
        _procs(new ProcListType())
    {
        for(; it != end; ++it) {
            insert_proc(*it);
        }
    }

    /*
     * Single proc construction
     */
    SimpleRoot(ProcAst *proc);
    SimpleRoot(ProcListType *procs);
    SimpleRoot(const SimpleRoot& other);
    SimpleRoot(SimpleRoot&& other);

    ProcAst* get_proc(const std::string& name);

    iterator begin();
    iterator end();

    SimpleRoot& operator =(const SimpleRoot& other);

    ~SimpleRoot();

    class iterator {
      public:
        typedef ProcListType::iterator    _iterator;
        typedef ProcAst*                  value_type;

        iterator(_iterator it);
        iterator& operator ++();
        ProcAst* operator *();
        bool operator ==(const iterator& other) const;
        bool operator !=(const iterator& other) const;
      private:
        _iterator _it;
    };

  private:
    void insert_proc(ProcAst *proc);
    std::shared_ptr<ProcListType> _procs;
};

/**
 * StatementASt represents a single statement in the Simple programming 
 * language.
 */
class StatementAst {
  public:
    /**
     * Get the line number of the AST node in the original source code.  
     */
    virtual int get_line() = 0;

    /**
     * Get the next statement within the same scope. Returns NULL if it's the 
     * last statement.
     */
    virtual StatementAst* next() = 0;

    /**
     * Get the previous statement within the same scope. Return NULL if it's the
     * first statement.
     */
    virtual StatementAst* prev() = 0;

    /**
     * Get the container node that contains the statement node. Due to weird rules 
     * in Simple, only If and While statements are container node, and the procedure 
     * node does not "contains" a statement.
     *
     * If the statement is in the top scope within the procedure, the return 
     * result here is NULL.
     */
    virtual ContainerAst* get_parent() = 0;

    /**
     * Get the procedure that contains the statement. All statements are always 
     * owned by certain procedure as there is no global scope in Simple.
     */
    virtual ProcAst* get_proc() = 0;

    /**
     * Accepts a statement visitor to determine the type of the statement using the 
     * visitor pattern.
     */
    virtual void accept_statement_visitor(StatementVisitor* visitor) = 0;

    virtual ~StatementAst() { }
};

/**
 * A ContainerAst is a statement AST node that contains child statements in a 
 * nested code. In simple there are only two types of container statements: If and While.
 */
class ContainerAst : public StatementAst {
  public:
    /**
     * Accepts a container visitor to determine whether the container statement is 
     * an If node or While node using the visitor pattern. 
     */
    virtual void accept_container_visitor(ContainerVisitor* visitor) = 0;

    virtual ~ContainerAst() { }
};

/**
 * A ConditionalAst represents an if statement. In Simple only a single variable 
 * name is allowed to be used as condition, and the condition only evaluates to
 * true if the variable is zero.
 */
class ConditionalAst : public ContainerAst {
  public:
    /**
     * Get the variable name used as the condition in the if statement. 
     */
    virtual SimpleVariable* get_variable() = 0;

    /**
     * get the first statement in the then branch in the if statement.
     */
    virtual StatementAst* get_then_branch() = 0;

    /**
     * get the first statement in the else branch in the if statement.
     * In Simple the else branch is compulsory so the pointer returned 
     * should be never be NULL.
     */
    virtual StatementAst* get_else_branch() = 0;

    virtual ~ConditionalAst() { }
};

/**
 * A WhileAst represents a while statement. Same as in the if statement, 
 * only the variable name is allowed in the while condition.
 */
class WhileAst : public ContainerAst {
  public:
    /**
     * Get the variable name used as the condition in the if statement.
     */
    virtual SimpleVariable* get_variable() = 0;

    /**
     * Get the first statement in the while statement body.
     */
    virtual StatementAst* get_body() = 0;

    virtual ~WhileAst() { }
};

/**
 * An AssignmentAst assigns the result of an expression to a single variable.
 */
class AssignmentAst : public StatementAst {
  public:
    /**
     * Get the name of the variable being assigned at the left side.
     */
    virtual SimpleVariable* get_variable() = 0;

    /**
     * Get the expression node at the right hand side of the assignment.
     */
    virtual ExprAst* get_expr() = 0;

    virtual ~AssignmentAst() { }
};

/**
 * A CallAst represents the call statement to a procedure. In Simple 
 * the procedure call does not allow any argument, so in this class 
 * there is no argument list.
 */
class CallAst : public StatementAst {
  public:

    /**
     * Get the procedure node of the call statement that is calling.
     */
    virtual ProcAst* get_proc_called() = 0;

    virtual ~CallAst() { }
};


/**
 * ExprAst represents the expression at the right hand side of an assignment 
 * statement. Unfortunately in Simple, not everything is expression and 
 * expression is only used in assignments.
 */
class ExprAst {
  public:
    /**
     * accept an ExprVisitor to determine the type of the expression node.
     */
    virtual void accept_expr_visitor(ExprVisitor*) = 0;
    virtual ExprAst* clone() = 0;

    virtual ~ExprAst() { }
};

/**
 * VariableAst represents a variable within an expression. VariableAst is 
 * not used for variables appear in other statements as the is no extra 
 * benefits. In other statements a variable is only be represented as a string.
 */
class VariableAst : public ExprAst {
  public:
    /**
     * Get the name of the variable.
     */
    virtual SimpleVariable* get_variable() = 0;

    virtual ~VariableAst() { }
};

/**
 * ConstAst represents a constant within an expression.
 */
class ConstAst : public ExprAst {
  public:
    /**
     * Get the integer value of the constant. Unfortunately in Simple the 
     * only data type available is int, so there will be no other constants 
     * such as double or string.
     */
    virtual SimpleConstant* get_constant() = 0;

    virtual ~ConstAst() { }
};

/**
 * BinaryOpAst represents a binary mathematical expression.
 */
class BinaryOpAst : public ExprAst {
  public:
    /**
     * Get the left hand side expression.
     */
    virtual ExprAst* get_lhs() = 0;

    /**
     * Get the right hand side expression.
     */
    virtual ExprAst* get_rhs() = 0;

    /**
     * Get the operand. In Simple only three kinds of operands
     * are possible: plus (+), minus (-), and multiply (*).
     */
    virtual char get_op() = 0;

    virtual ~BinaryOpAst() { }
};

class ContainerVisitor {
  public:
    virtual void visit_conditional(ConditionalAst*) = 0;
    virtual void visit_while(WhileAst*) = 0;

    virtual ~ContainerVisitor() { }
};

class StatementVisitor : public ContainerVisitor {
  public:
    virtual void visit_assignment(AssignmentAst*) = 0;
    virtual void visit_call(CallAst*) = 0;

    virtual ~StatementVisitor() { }
};

class ExprVisitor {
  public:
    virtual void visit_const(ConstAst*) = 0;
    virtual void visit_variable(VariableAst*) = 0;
    virtual void visit_binary_op(BinaryOpAst*) = 0;

    virtual ~ExprVisitor() { }
};


} // namespace simple
