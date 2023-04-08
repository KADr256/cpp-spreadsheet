#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <exception>
#include <algorithm>


// Реализуйте следующие методы
/*
Cell::Cell()
{}
*/

Cell::~Cell()
{
	sheet = nullptr;
}

void Cell::Set(std::string text, SheetInterface* sheet_in, Position pos_in)
{
	sheet = sheet_in;
	pos = pos_in;
	std::vector<Position> old = std::move(dependence);
	if (text.size() == 0) {
		impl_ = std::make_unique<EmptyImpl>(text);
		ClearValue();
		return;
	}
	switch (text[0])
	{
	case '=':
	{
		if (text.size() == 1) {
			impl_ = std::make_unique<TextImpl>(std::forward<std::string>(text));
		}
		else {
			std::unique_ptr<FormulaInterface> buf1;
			try {
				buf1 = ParseFormula(text.substr(1));
			}
			catch (FormulaException&) {
				dependence = std::move(old);
				throw;
			}

			dependence = std::move(buf1.get()->GetReferencedCells());
			try {
				SearchCyclicAndAddDependent();
			}
			catch (CircularDependencyException&) {
				dependence = std::move(old);
				throw;
			}

			for (auto& el : old) {
				Cell* buf1 = dynamic_cast<Cell*>(sheet->GetCell(el));
				auto buf2 = std::find(buf1->dependent.begin(), buf1->dependent.end(), pos);

				//std::swap(buf2, buf1->dependent.end() - 1);
				auto buf3 = *buf2;
				*buf2 = *(buf1->dependent.end() - 1);
				*(buf1->dependent.end() - 1) = buf3;

				buf1->dependent.pop_back();
			}

			impl_ = std::make_unique<FormulaImpl>(std::forward<std::string>('=' + buf1.get()->GetExpression()), sheet);
		}
		break;
	}
	case '\'':
		impl_ = std::make_unique<TextImpl>(std::forward<std::string>(text));
		break;
	default:
		impl_ = std::make_unique<TextImpl>(std::forward<std::string>(text));
		break;
	}
	ClearValue();
}

void Cell::Clear()
{
	for (auto& el : dependence) {
		auto buf1 = sheet->GetCell(el);
		Cell* buf2 = dynamic_cast<Cell*>(buf1);
		buf2->dependent.erase(std::find(dependent.begin(), dependent.end(), pos));
	}
	impl_ = std::make_unique<EmptyImpl>(static_cast<std::string>(""));
}

void Cell::ClearValue()
{
	std::unordered_set<Position>* cleared = new std::unordered_set<Position>;
	if (impl_) {
		impl_.get()->value = "";
		impl_.get()->change = true;
	}
	for (auto& el : dependent) {
		cleared->insert(el);
		auto buf1 = sheet->GetCell(el);
		Cell* buf2 = dynamic_cast<Cell*>(buf1);
		buf2->ClearValueNext(cleared);
	}
}

void Cell::ClearValueNext(std::unordered_set<Position>* cleared)
{
	if (impl_) {
		impl_.get()->value = "";
		impl_.get()->change = true;
	}
	for (auto& el : dependent) {
		if (!cleared->count(el)) {
			cleared->insert(el);
			auto buf1 = sheet->GetCell(el);
			Cell* buf2 = dynamic_cast<Cell*>(buf1);
			buf2->ClearValueNext(cleared);
		}
	}
}

Cell::Value Cell::GetValue() const
{
	return impl_.get()->GetValue();
}

std::string Cell::GetText() const
{
	return impl_.get()->GetText();
}

Cell::Impl::Impl(std::string text) :text(text)
{
}

Cell::EmptyImpl::EmptyImpl(std::string text) :Impl(text)
{
}

CellInterface::Value Cell::EmptyImpl::GetValue()
{
	return "";
}

std::string Cell::EmptyImpl::GetText() const
{
	return "";
}

Cell::TextImpl::TextImpl(std::string text) :Impl(text)
{
}

CellInterface::Value Cell::TextImpl::GetValue()
{
	switch (text[0])
	{
	case '\'':
		return text.substr(1);
	default:
		return text;
	};
}

std::string Cell::TextImpl::GetText() const
{
	return text;
}

Cell::FormulaImpl::FormulaImpl(std::string text, SheetInterface* sheet_in) :Impl(text), sheet(sheet_in)
{
}

CellInterface::Value Cell::FormulaImpl::GetValue()
{
	if (change) {
		change = false;
		auto buf1 = ParseFormula(text.substr(1));
		auto buf2 = buf1.get()->Evaluate(*sheet);//
		if (buf2.index() == 0) {
			value = std::get<double>(buf2);
		}
		else {
			value = std::get<FormulaError>(buf2);
		}
	}
	return value;
}

std::string Cell::FormulaImpl::GetText() const
{
	return text;
}

std::vector<Position> Cell::GetReferencedCells() const
{
	return dependence;
}

void Cell::SearchCyclicAndAddDependent()
{
	std::unordered_set<Position>* tested = new std::unordered_set<Position>;
	std::unordered_set<Position>* add = new std::unordered_set<Position>;
	try {
		for (auto& el : dependence) {
			if (el == pos) {
				throw std::invalid_argument("");
			}
			/*
			if (tested->count(el)) {
				continue;
			}
			*/
			tested->insert(el);
			auto buf1 = sheet->GetCell(el);
			if (buf1 != nullptr) {
				Cell* buf2 = dynamic_cast<Cell*>(buf1);
				buf2->dependent.push_back(pos);
				add->insert(el);
				buf2->SearchCyclicNext(pos, tested);
			}
			else {
				sheet->SetCell(el, "");
				buf1 = sheet->GetCell(el);
				Cell* buf2 = dynamic_cast<Cell*>(buf1);
				buf2->dependent.push_back(pos);
				add->insert(el);
			}
		}
	}
	catch (std::invalid_argument&) {
		ClearSearchCyclic(add);
		throw CircularDependencyException("");
	}
	delete tested;
	delete add;
	//std::unordered_set<Position>* tested = new std::unordered_set<Position>;
}

void Cell::SearchCyclicNext(const Position pos_in, std::unordered_set<Position>* tested)
{
	for (auto& el : dependence) {
		if (el == pos_in) {
			throw std::invalid_argument("");
		}
		if (tested->count(el)) {
			continue;
		}
		tested->insert(el);
		auto buf1 = sheet->GetCell(el);
		if (buf1 != nullptr) {
			Cell* buf2 = dynamic_cast<Cell*>(buf1);
			buf2->SearchCyclicNext(pos_in, tested);
		}
	}
}

void Cell::ClearSearchCyclic(const std::unordered_set<Position>* add)
{
	for (auto& el : *add) {
		Cell* buf2 = dynamic_cast<Cell*>(sheet->GetCell(el));
		buf2->dependent.pop_back();
	}
}