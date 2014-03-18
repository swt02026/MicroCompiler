#include <iostream>
#include <cstdio>
#include <cctype>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
using namespace std;
enum token {
	BEGIN,
	END,
	READ,
	WRITE,
	ID,
	INTLITERAL,
	LPAREN,
	RPAREN,
	SEMICOLON,
	COMMA,
	ASSIGNOP,
	PLUSOP,
	MINUSOP,
	SCANEOF,
};
struct op_rec {
	enum op {
		PLUS, MINUS
	} Operator;
	string extract() const {
		return Operator == PLUS ? "Add" : "Sub";
	}
};
enum expr {
	IDEXPR, LITERALEXPR, TEMPEXPR
};
struct expr_rec {
	enum expr kind;
	string name;
	string val;
	string extract() const {
		return kind == expr::LITERALEXPR ? val : name;
	}
};
string token_buffer;
token CurrentToken = token::SCANEOF;
token check_reserved() {
	const static string ReserveWord[4] = { "begin", "end", "read", "write" };
	for (int i = 0; i < 4; i++)
		if (token_buffer == ReserveWord[i])
			return static_cast<token>(i);
	return ID;
}
void lexical_error(char c) {
	cout << "lexical error : can\'t recognize " << c << " this character"
			<< endl;
}
void syntax_error(token tok) {
	cout << "syntax error " << tok << endl;
}
token scanner() {
	int in_char = 0, c;
	for (token_buffer.clear(); (in_char = getchar()) != EOF;) {
		if (isspace(in_char))
			continue;
		else if (isalpha(in_char)) {
			for (token_buffer += in_char, c = getchar(); isalnum(c) || c == '_';
					c = getchar())
				token_buffer += c;
			ungetc(c, stdin);
			return check_reserved();
		} else if (isdigit(in_char)) {
			for (token_buffer += in_char, c = getchar(); isdigit(c); c =
					getchar())
				token_buffer += c;
			ungetc(c, stdin);
			return INTLITERAL;
		} else if (in_char == '(') {
			token_buffer += in_char;
			return LPAREN;
		} else if (in_char == ')') {
			token_buffer += in_char;
			return RPAREN;
		} else if (in_char == ';') {
			token_buffer += in_char;
			return SEMICOLON;
		} else if (in_char == ',') {
			token_buffer += in_char;
			return COMMA;
		} else if (in_char == '+') {
			token_buffer += in_char;
			return PLUSOP;
		} else if (in_char == ':') {
			if ((c = getchar()) == '=')
				return ASSIGNOP;
			else {
				ungetc(c, stdin);
				lexical_error(c);
			}
		} else if (in_char == '-') {
			if ((c = getchar()) == '-')
				for (; (in_char = getchar()) != '\n';)
					;
			else {
				ungetc(c, stdin);
				token_buffer += in_char;
				return MINUSOP;
			}
		}
		lexical_error(in_char);
	}
	if (feof(stdin))
		return SCANEOF;
}

token next_token() {
	token tmp = scanner();
	for (int i = token_buffer.length(); ~--i;)
		ungetc(token_buffer[i], stdin);
	return tmp;
}
void match(token MatchToken) {
	token tmp = scanner();
	if (tmp == MatchToken)
		CurrentToken = tmp;
	else
		cout << "match error:", syntax_error(MatchToken);
}
void generate(const string& op, const string& lhs, const string& rhs,
		const string& result) {
	cout << op << ' ' << lhs << (result.empty() ? '\0' : ',') << rhs << (result.empty() ? '\0' : ',')
			<< result << endl;
}
void check_id(const string& s) {
	static vector<string> table;
	if (find(table.begin(), table.end(), s) == table.end()) {
		table.push_back(s);
		generate("Declare", s, "Integer", "");
	}
}
void process_id(expr_rec& Operand) {
	check_id(token_buffer);
	Operand.kind = IDEXPR;
	Operand.name = token_buffer;
}
void process_op(op_rec& op) {
	op.Operator = CurrentToken == PLUSOP ? op_rec::PLUS : op_rec::MINUS;
}
void process_literal(expr_rec& Operand)
{
	check_id(token_buffer);
	Operand.kind = expr::LITERALEXPR;
	Operand.val = token_buffer;
}
void add_op(op_rec& op) {
	token tok = next_token();
	if (tok == PLUSOP || tok == MINUSOP) {
		match(tok);
		process_op(op);
	} else
		cout << "add op error:" << token_buffer << '\t', syntax_error(tok);
}
void assign(expr_rec source,expr_rec target)
{
	generate("Store",source.extract(),target.extract(),"");
}
void primary(expr_rec& Operand);
void expression(expr_rec& result);
string get_temp() {
	static int max_temp = 0;
	static string tempname;
	static stringstream sstm;
	sstm.str("");
	sstm.clear();
	sstm << max_temp++;
	tempname = "Temp&";
	tempname += sstm.str();
	check_id(tempname);
	return tempname;
}
expr_rec gen_infix(expr_rec e1, op_rec op, expr_rec e2) {
	expr_rec tmp_rec;
	tmp_rec.kind = TEMPEXPR;
	tmp_rec.name = get_temp();
	generate(op.extract(), e1.extract(), e2.extract(), tmp_rec.name);
	return tmp_rec;
}
void start()
{

}
void finish()
{
	generate("Halt","","","");
}
void statement() {
	token tok = next_token();
	expr_rec source,target;
	switch (tok) {
	case token::ID:
		match(token::ID);
		process_id(target);
		match(token::ASSIGNOP);
		expression(source);
		match(token::SEMICOLON);
		assign(source,target);
		break;
	default:
		syntax_error(tok);
		break;
	}
}
void statement_list() {
	for (statement();;)
		switch (next_token()) {
		case token::ID:
		case token::READ:
		case token::WRITE:
			statement();
			break;
		default:
			return;
		}
}
void program() {
	match(token::BEGIN);
	start();
	statement_list();
	match(token::END);
	finish();
}
void sysytem_goal() {
	program();
	match(token::SCANEOF);
}
int main() {
	sysytem_goal();
}
void expression(expr_rec& result) {
	expr_rec LOperand, ROperand;
	op_rec op;
	for (primary(LOperand);
			next_token() == token::PLUSOP || next_token() == token::MINUSOP;
			LOperand = gen_infix(LOperand, op, ROperand)) {
		add_op(op);
		primary(ROperand);
	}
	result = LOperand;
}
void primary(expr_rec& Operand) {
	switch (next_token()) {
	case token::LPAREN:
		match(LPAREN);
		expression(Operand);
		match(RPAREN);
		break;
	case token::ID:
		match(ID);
		process_id(Operand);
		break;
	case token::INTLITERAL:
		match(INTLITERAL);
		process_literal(Operand);
		break;
	default:
		syntax_error(next_token());
		break;
	}
}
