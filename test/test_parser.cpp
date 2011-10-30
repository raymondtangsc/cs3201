
#include <list>
#include <memory>
#include "gtest/gtest.h"
#include "simple/util/ast_utils.h"
#include "impl/parser/tokenizer.h"
#include "impl/parser/parser.h"
#include "impl/parser/iterator_tokenizer.h"

namespace simple {
namespace test {

using namespace simple;
using namespace simple::parser;
using namespace simple::util;

class MockTokenizer : public SimpleTokenizer {
  public:
    MockTokenizer() : 
        _tokens(), _first_token(true)
    { }
    
    void insert(SimpleToken *token) {
        _tokens.push_back(std::unique_ptr<SimpleToken>(token));

        if(_first_token) {
            _first_token = false;
            _token_it = _tokens.begin();
        }
    }

    SimpleToken* next_token() {
        if(_token_it == _tokens.end()) {
            throw ParserError();
        } else {
            return (_token_it++)->get();
        }
    }

    virtual ~MockTokenizer() { }

  private:
    std::list< std::unique_ptr<SimpleToken> > _tokens;

    std::list< std::unique_ptr<SimpleToken> >::iterator
    _token_it;

    bool _first_token;
};

TEST(ParserTest, TestCallParser) {
    /*
     * call test_proc;
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("call"));
    tokenizer->insert(new IdentifierToken("test_proc"));
    tokenizer->insert(new SemiColonToken());
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);

    std::unique_ptr<SimpleCallAst> call(parser.parse_call());

    ASSERT_FALSE(parser.get_proc("test_proc") == NULL);
    std::unique_ptr<SimpleProcAst> proc(parser.get_proc("test_proc"));

    EXPECT_EQ(call->get_proc_called(), proc.get());

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, TestAssignmentParser1) {
    /*
     * x = 1;
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(1));
    tokenizer->insert(new SemiColonToken());
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);
    std::unique_ptr<SimpleAssignmentAst> assign(parser.parse_assignment());

    EXPECT_EQ(assign->get_variable()->get_name(), std::string("x"));
    
    SimpleConstAst value(1);
    EXPECT_TRUE((is_same_expr<ExprAst, ExprAst>(assign->get_expr(), &value)));

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, TestAssignmentParser2) {
    /*
     * x = y+2*z;
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new OperatorToken('+'));
    tokenizer->insert(new IntegerToken(2));
    tokenizer->insert(new OperatorToken('*'));
    tokenizer->insert(new IdentifierToken("z"));
    tokenizer->insert(new SemiColonToken());
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);
    std::unique_ptr<SimpleAssignmentAst> assign(parser.parse_assignment());

    EXPECT_EQ(assign->get_variable()->get_name(), std::string("x"));
    
    SimpleBinaryOpAst *two_times_z = new SimpleBinaryOpAst('*', 
            new SimpleConstAst(2), new SimpleVariableAst("z"));
    SimpleBinaryOpAst expected_expr('+', new SimpleVariableAst("y"), two_times_z);
    EXPECT_TRUE((is_same_expr<ExprAst, ExprAst>(assign->get_expr(), &expected_expr)));

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, ExprParser1) {
    /*
     * x+y*z
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new OperatorToken('+'));
    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new OperatorToken('*'));
    tokenizer->insert(new IdentifierToken("z"));
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);
    std::unique_ptr<ExprAst> expr(parser.parse_expr());

    SimpleBinaryOpAst *y_times_z = new SimpleBinaryOpAst('*', 
            new SimpleVariableAst("y"), new SimpleVariableAst("z"));
    SimpleBinaryOpAst expected_expr('+', new SimpleVariableAst("x"), y_times_z);
    EXPECT_TRUE((is_same_expr<ExprAst, ExprAst>(expr.get(), &expected_expr)));

    SimpleBinaryOpAst *x_plus_y = new SimpleBinaryOpAst('+', 
            new SimpleVariableAst("x"), new SimpleVariableAst("y"));
    SimpleBinaryOpAst wrong_expr('*', x_plus_y, new SimpleVariableAst("z"));
    EXPECT_FALSE((is_same_expr<ExprAst, ExprAst>(expr.get(), &wrong_expr)));

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, ExprParser2) {
    /*
     * (x+y)*z
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new OpenBracketToken());
    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new OperatorToken('+'));
    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new CloseBracketToken());
    tokenizer->insert(new OperatorToken('*'));
    tokenizer->insert(new IdentifierToken("z"));
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);
    std::unique_ptr<ExprAst> expr(parser.parse_expr());

    SimpleBinaryOpAst *x_plus_y = new SimpleBinaryOpAst('+', 
            new SimpleVariableAst("x"), new SimpleVariableAst("y"));
    SimpleBinaryOpAst expected_expr('*', x_plus_y, new SimpleVariableAst("z"));
    EXPECT_TRUE((is_same_expr<ExprAst, ExprAst>(expr.get(), &expected_expr)));

    SimpleBinaryOpAst *y_times_z = new SimpleBinaryOpAst('*', 
            new SimpleVariableAst("y"), new SimpleVariableAst("z"));
    SimpleBinaryOpAst wrong_expr('+', new SimpleVariableAst("x"), y_times_z);
    EXPECT_FALSE((is_same_expr<ExprAst, ExprAst>(expr.get(), &wrong_expr)));

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, WhileParser) {
    /*
     * while i {
     *   x = 1;
     *   y = 2;
     * }
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("while"));
    tokenizer->insert(new IdentifierToken("i"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(1));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(2));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);
    
    SimpleProcAst proc("test");
    SimpleVariable var_i("i");
    SimpleVariable var_x("x");
    SimpleVariable var_y("y");

    std::unique_ptr<SimpleWhileAst> loop( parser.parse_while(&proc) );
    EXPECT_EQ(*loop->get_variable(), var_i);

    SimpleWhileAst expected_loop;
    expected_loop.set_variable(var_i);
    
    SimpleAssignmentAst *assign1 = new SimpleAssignmentAst();
    assign1->set_variable(var_x);
    assign1->set_expr(new SimpleConstAst(1));
    set_while_body(assign1, &expected_loop);

    SimpleAssignmentAst *assign2 = new SimpleAssignmentAst();
    assign2->set_variable(var_y);
    assign2->set_expr(new SimpleConstAst(2));
    set_next(assign1, assign2);

    EXPECT_TRUE((is_same_statement<WhileAst, WhileAst>(loop.get(), &expected_loop)));

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, ConditionalParser) {
    /*
     * if i {
     *   x = 1;
     * } else {
     *   y = 2;
     * }
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("if"));
    tokenizer->insert(new IdentifierToken("i"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(1));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new IdentifierToken("else"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(2));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);

    SimpleProcAst proc("test");
    SimpleVariable var_i("i");
    SimpleVariable var_x("x");
    SimpleVariable var_y("y");

    std::unique_ptr<SimpleConditionalAst> condition( parser.parse_conditional(&proc) );
    EXPECT_EQ(*condition->get_variable(), var_i);

    SimpleConditionalAst expected_condition;
    expected_condition.set_variable(var_i);
    
    SimpleAssignmentAst *assign1 = new SimpleAssignmentAst();
    assign1->set_variable(var_x);
    assign1->set_expr(new SimpleConstAst(1));
    set_then_branch(assign1, &expected_condition);

    SimpleAssignmentAst *assign2 = new SimpleAssignmentAst();
    assign2->set_variable(var_y);
    assign2->set_expr(new SimpleConstAst(2));
    set_else_branch(assign2, &expected_condition);

    EXPECT_TRUE((is_same_statement<ConditionalAst, ConditionalAst>(condition.get(), &expected_condition)));

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, ProcParser) {
    /*
     * proc test {
     *   x = 1;
     *   y = 2;
     * }
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("proc"));
    tokenizer->insert(new IdentifierToken("test"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(1));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(2));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);
    std::unique_ptr<SimpleProcAst> proc( parser.parse_proc() );
    
    EXPECT_EQ(proc->get_name(), "test");

    SimpleProcAst expected_proc("test");
    SimpleVariable var_x("x");
    SimpleVariable var_y("y");

    SimpleAssignmentAst *assign1 = new SimpleAssignmentAst();
    assign1->set_variable(var_x);
    assign1->set_expr(new SimpleConstAst(1));
    set_proc(assign1, &expected_proc);

    SimpleAssignmentAst *assign2 = new SimpleAssignmentAst();
    assign2->set_variable(var_y);
    assign2->set_expr(new SimpleConstAst(2));
    set_next(assign1, assign2);
    
    EXPECT_TRUE((is_same_statement_list(assign1, proc->get_statement())));

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, ProgramParser) {
    /*
     * proc test {
     *   x = 1;
     *   y = 2;
     * }
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("proc"));
    tokenizer->insert(new IdentifierToken("test"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(1));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(2));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new EOFToken());

    SimpleParser parser(tokenizer);
    SimpleRoot root = parser.parse_program();
    
    EXPECT_TRUE(root.get_proc("test") != NULL);

    parser.current_token_as<EOFToken>();
}

TEST(ParserTest, IntegratedParserTest) {
    /*
     * proc test1 {             // proc1
     *   a = 1;                 // assign1
     *   while i {              // loop
     *     call test2;          // call
     *     if j {               // condition
     *       x = (x+y)*(3+z);   // assign2
     *     } else {
     *       y = 2;             // assign3
     *     }
     *   }
     *   b = 4;                 // assign4
     * }
     *
     * proc test2 {             // proc2
     *   c = 3;                 // assign5
     * }
     */
    MockTokenizer *tokenizer = new MockTokenizer();
    tokenizer->insert(new IdentifierToken("proc"));
    tokenizer->insert(new IdentifierToken("test1"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("a"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(1));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new IdentifierToken("while"));
    tokenizer->insert(new IdentifierToken("i"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("call"));
    tokenizer->insert(new IdentifierToken("test2"));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new IdentifierToken("if"));
    tokenizer->insert(new IdentifierToken("j"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new OpenBracketToken());
    tokenizer->insert(new IdentifierToken("x"));
    tokenizer->insert(new OperatorToken('+'));
    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new CloseBracketToken());
    tokenizer->insert(new OperatorToken('*'));
    tokenizer->insert(new OpenBracketToken());
    tokenizer->insert(new IntegerToken(3));
    tokenizer->insert(new OperatorToken('+'));
    tokenizer->insert(new IdentifierToken("z"));
    tokenizer->insert(new CloseBracketToken());
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new IdentifierToken("else"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("y"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(2));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new CloseBraceToken());

    tokenizer->insert(new IdentifierToken("b"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(4));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());

    tokenizer->insert(new IdentifierToken("proc"));
    tokenizer->insert(new IdentifierToken("test2"));
    tokenizer->insert(new OpenBraceToken());

    tokenizer->insert(new IdentifierToken("c"));
    tokenizer->insert(new EqualToken());
    tokenizer->insert(new IntegerToken(3));
    tokenizer->insert(new SemiColonToken());

    tokenizer->insert(new CloseBraceToken());
    tokenizer->insert(new EOFToken());


    SimpleParser parser(tokenizer);
    SimpleRoot root = parser.parse_program();

    EXPECT_TRUE(root.get_proc("test1") != NULL);
    EXPECT_TRUE(root.get_proc("test2") != NULL);

    parser.current_token_as<EOFToken>();

    SimpleVariable var_a("a");
    SimpleVariable var_b("b");
    SimpleVariable var_c("c");
    SimpleVariable var_i("i");
    SimpleVariable var_j("j");
    SimpleVariable var_x("x");
    SimpleVariable var_y("y");
    SimpleVariable var_z("z");

    SimpleProcAst proc1("test1");
    SimpleProcAst proc2("test2");

    SimpleAssignmentAst *assign1 = new SimpleAssignmentAst();
    assign1->set_variable(var_a);
    assign1->set_expr(new SimpleConstAst(1));
    set_proc(assign1, &proc1);
    
    SimpleWhileAst *loop = new SimpleWhileAst();
    loop->set_variable(var_i);
    set_next(assign1, loop);

    SimpleCallAst *call = new SimpleCallAst();
    call->set_proc_called(&proc2);
    set_while_body(call, loop);

    SimpleConditionalAst *condition = new SimpleConditionalAst();
    condition->set_variable(var_j);
    set_next(call, condition);

    SimpleAssignmentAst *assign2 = new SimpleAssignmentAst();
    assign2->set_variable(var_x);
    assign2->set_expr(new SimpleBinaryOpAst('*',
           new SimpleBinaryOpAst('+',
               new SimpleVariableAst(var_x),
               new SimpleVariableAst(var_y)),
           new SimpleBinaryOpAst('+',
               new SimpleConstAst(3),
               new SimpleVariableAst(var_z))));
    set_then_branch(assign2, condition);

    SimpleAssignmentAst *assign3 = new SimpleAssignmentAst();
    assign3->set_variable(var_y);
    assign3->set_expr(new SimpleConstAst(2));
    set_else_branch(assign3, condition);

    SimpleAssignmentAst *assign4 = new SimpleAssignmentAst();
    assign4->set_variable(var_b);
    assign4->set_expr(new SimpleConstAst(4));
    set_next(loop, assign4);

    SimpleAssignmentAst *assign5 = new SimpleAssignmentAst();
    assign5->set_variable(var_c);
    assign5->set_expr(new SimpleConstAst(3));
    set_proc(assign5, &proc2);

    EXPECT_TRUE((is_same_statement_list(
                    root.get_proc("test1")->get_statement(),
                    assign1)));
}

TEST(ParserTest, FullItegrationTest) {
    std::string source = 
        "proc test1 { "
        "   a = 1; "
        "   while i { "
        "       call test2; "
        "       if j { "
        "           x = (x+y)*(3+z); "
        "       } else { "
        "           y = 2; } } "
        "   b = 4; } "
        "proc test2 { "
        "   c = 3; } ";

    SimpleParser parser(new IteratorTokenizer<
            std::string::iterator>(source.begin(), source.end()));

    SimpleRoot root = parser.parse_program();

}


}
}
