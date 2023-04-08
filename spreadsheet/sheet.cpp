#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

std::ostream& operator<< (std::ostream& out, const CellInterface::Value val) {
	switch (val.index())
	{
	case 0:
		out << std::get<std::string>(val);
		break;
	case 1:
		out << std::get<double>(val);
		break;
	case 2:
		out << std::get<FormulaError>(val);
		break;
	}
	return out;
}

void TestPositionMinMax(Position pos) {
	if (pos.col < 0 || pos.row < 0) {
		throw InvalidPositionException("pos<0");
	}
	if (pos.col >= Position::MAX_COLS || pos.row >= Position::MAX_ROWS) {
		throw InvalidPositionException("pos>=MAX");
	}
}

bool Sheet::TestPositionInList(Position pos) const
{
	if (static_cast<int>(col_cell_counter.size()) <= pos.col || static_cast<int>(row_cell_counter.size()) <= pos.row) {
		return false;
	}
	else {
		return true;
	}
}

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
	TestPositionMinMax(pos);
	if (static_cast<int>(list_.size()) < pos.row + 1) {
		size_t list_past = list_.size();
		list_.resize(pos.row + 1);
		row_cell_counter.resize(pos.row + 1);
		for (size_t i = list_past; i < list_.size(); i++)
		{
			list_[i].resize(list_[0].size());
		}
	}
	if (static_cast<int>(list_[0].size()) < pos.col + 1) {
		col_cell_counter.resize(pos.col + 1);
		for (size_t i = 0; i < list_.size(); i++)
		{
			list_[i].resize(pos.col + 1);
		}
	}

	if (list_[pos.row][pos.col]) {
		if (text != list_[pos.row][pos.col].get()->GetText()) {
			list_[pos.row][pos.col].get()->Set(text, this, pos);
			/*
			try {
				auto old = std::move(list_[pos.row][pos.col].get());
				list_[pos.row][pos.col].get()->Set(text, this, pos);
			}
			catch (...) {

			}
			*/
		}
	}
	else {
		try {
			list_[pos.row][pos.col] = std::make_unique<Cell>();
			list_[pos.row][pos.col].get()->Set(text, this, pos);
		}
		catch (...) {
			list_[pos.row][pos.col].reset();
			throw;
		}
		++row_cell_counter[pos.row];
		++col_cell_counter[pos.col];
	}
	//ClearCell(pos);
	//change = true;
}

const CellInterface* Sheet::GetCell(Position pos) const {
	TestPositionMinMax(pos);
	if (TestPositionInList(pos) && list_[pos.row][pos.col]) {
		return list_[pos.row][pos.col].get();
	}
	else {
		return nullptr;
	}
}

CellInterface* Sheet::GetCell(Position pos) {
	TestPositionMinMax(pos);
	if (TestPositionInList(pos) && list_[pos.row][pos.col]) {
		return list_[pos.row][pos.col].get();
	}
	else {
		return nullptr;
	}
}

void Sheet::ClearCell(Position pos) {
	TestPositionMinMax(pos);
	if (TestPositionInList(pos) && list_[pos.row][pos.col]) {
		list_[pos.row][pos.col].reset();
		--row_cell_counter[pos.row];
		--col_cell_counter[pos.col];
	}
}

Size Sheet::GetPrintableSize() const {
	size_t row_counter;
	size_t col_counter;
	row_counter = row_cell_counter.size();
	col_counter = col_cell_counter.size();

	for (; row_counter > 0; --row_counter) {
		if (row_cell_counter[row_counter - 1] != 0) {
			break;
		}
	}
	for (; col_counter > 0; --col_counter) {
		if (col_cell_counter[col_counter - 1] != 0) {
			break;
		}
	}
	return { static_cast<int>(row_counter),static_cast<int>(col_counter) };
}

void Sheet::PrintValues(std::ostream& output) const {
	auto table_size = GetPrintableSize();
	if (table_size.cols != 0) {
		for (int i = 0; i < table_size.rows; ++i) {
			for (int j = 0; j < table_size.cols - 1; ++j) {
				if (list_[i][j]) {
					output << list_[i][j].get()->GetValue();
				}
				output << '\t';
			}
			if (list_[i][table_size.cols - 1]) {
				output << list_[i][table_size.cols - 1].get()->GetValue();
			}
			output << '\n';
		}
	}
}
void Sheet::PrintTexts(std::ostream& output) const {
	auto table_size = GetPrintableSize();
	if (table_size.cols != 0) {
		for (int i = 0; i < table_size.rows; ++i) {
			for (int j = 0; j < table_size.cols - 1; ++j) {
				if (list_[i][j]) {
					output << list_[i][j].get()->GetText();
				}
				output << '\t';
			}
			if (list_[i][table_size.cols - 1]) {
				output << list_[i][table_size.cols - 1].get()->GetText();
			}
			output << '\n';
		}
	}
}

std::unique_ptr<SheetInterface> CreateSheet() {
	return std::make_unique<Sheet>();
}