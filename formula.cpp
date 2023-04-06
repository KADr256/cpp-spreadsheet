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
		// Реализуйте следующие методы:
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
			return {};
		}
	private:
		FormulaAST ast_;
	};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	if (expression.size()== 0) {
		throw FormulaException("size=0");
	}
	int brackets = 0;
	bool exp_num = true;
	bool last_letter = false;
	//bool exp_comma = false;
	//bool used_comma = false;
	for (char el : expression) {
		if (el < 91 && el > 64) {
			last_letter = true;
			exp_num = true;
			continue;
		}
		if (el < 58 && el> 48) {
			last_letter = false;
			exp_num = false;
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
			break;
		case ')':
			if (exp_num) {
				throw FormulaException("expect num, but )");
			}
			--brackets;
			break;
		case '-':
			/*
			if (exp_num && !first) {
				throw FormulaException("expect num, but -");
			}
			*/
			exp_num = true;
			break;
		case '+':
			/*
			if (exp_num) {
				throw FormulaException("expect num, but +");
			}
			*/
			exp_num = true;
			break;
		case '*':
			if (exp_num) {
				throw FormulaException("expect num, but *");
			}
			exp_num = true;
			break;
		case '/':
			if (exp_num) {
				throw FormulaException("expect num, but /");
			}
			exp_num = true;
			break;
		case '0'://
			exp_num = false;
			break;		
		case',':
			exp_num = true;
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

	return std::make_unique<Formula>(std::move(expression));
}