#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category) {
	category_ = category;
}

FormulaError::Category FormulaError::GetCategory() const
{
	return category_;
}

bool FormulaError::operator==(FormulaError rhs) const
{
	return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const
{
	switch (category_)
	{
	case FormulaError::Category::Ref:
		return "#REF!";
		break;
	case FormulaError::Category::Value:
		return "#VALUE!";
		break;
	case FormulaError::Category::Div0:
		return "#DIV/0!";
		break;
	default:
		throw std::invalid_argument("unknown enum category");
		break;
	}
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
	return output << fe.ToString();
}

namespace {
	class Formula : public FormulaInterface {
	public:
		explicit Formula(std::string expression):ast_(ParseFormulaAST(expression)) {
		}

		Value Evaluate(const SheetInterface& sheet) const override {
			Value result;
			try
			{
				result = ast_.Execute(sheet);
			}
			catch (FormulaError& er)
			{
				result = er;
			}
			return result;
		}
		std::string GetExpression() const override {
			std::ostringstream out;
			ast_.PrintFormula(out);
			return out.str();
		}
		std::vector<Position> GetReferencedCells() const override {
			std::vector<Position> result = { ast_.GetCells().begin(), ast_.GetCells().end() };
			auto buf1 = std::unique(result.begin(), result.end());
			result.erase(buf1, result.end());
			return result;
		}
	private:
		FormulaAST ast_;
	};
}  // namespace

bool TestColWord(bool col_word,std::string& word) {
	if (col_word) {
		auto buf1 = Position::FromString(word);
		if (!buf1.IsValid()) {
			throw FormulaException("Not Valid Position");
		}
		word.clear();
		return false;
	}
	else {
		word.clear();
		return false;
	}
}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	if (expression.size()== 0) {
		throw FormulaException("size=0");
	}
	int brackets = 0;
	int num = 0;
	bool exp_num = true;
	bool last_letter = false;
	std::string word;
	bool col_word = false;
	for (char el : expression) {
		if (el < 91 && el > 64) {
			last_letter = true;
			exp_num = true;
			word += el;
			col_word = true;
			continue;
		}
		if (el < 58 && el> 48) {
			last_letter = false;
			exp_num = false;
			num++;
			if (col_word) {
				word += el;
			}
			continue;
		}
		switch (el)
		{
		case '(':
			if (last_letter && exp_num) {
				throw FormulaException("expect num after letter, but (");
			}
			exp_num = false;
			++brackets;
			col_word = TestColWord(col_word, word);
			break;
		case ')':
			if (exp_num) {
				throw FormulaException("expect num, but )");
			}
			--brackets;
			col_word = TestColWord(col_word, word);
			break;
		case '-':
			exp_num = true;
			col_word = TestColWord(col_word, word);
			break;
		case '+':
			exp_num = true;
			col_word = TestColWord(col_word, word);
			break;
		case '*':
			if (exp_num) {
				throw FormulaException("expect num, but *");
			}
			exp_num = true;
			col_word = TestColWord(col_word, word);
			break;
		case '/':
			if (exp_num) {
				throw FormulaException("expect num, but /");
			}
			exp_num = true;
			col_word = TestColWord(col_word, word);
			break;
		case '0'://
			exp_num = false;
			num++;
			if (col_word) {
				word += '0';
			}
			break;
		case'.':
			exp_num = true;
			col_word = TestColWord(col_word, word);
			break;
		case' ':
			col_word = TestColWord(col_word, word);
			break;
		case 'e':
			break;
		default:
			throw FormulaException("unexpected symbol");
		}
	}

	if (exp_num) {
		throw FormulaException("expect num, but end");
	}
	if (brackets != 0) {
		throw FormulaException("all brackets not closed");
	}
	if (num == 0) {
		throw FormulaException("not found any number");
	}
	if (col_word) {
		TestColWord(col_word,word);
	}

	return std::make_unique<Formula>(std::move(expression));
}