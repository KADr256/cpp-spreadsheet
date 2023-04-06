#pragma once

#include "common.h"
#include "formula.h"
#include "unordered_set"
//#include "sheet.h"

class Cell : public CellInterface {
public:
	//Cell();
	~Cell();

	class Impl {
	public:
		Impl(std::string text);
		virtual CellInterface::Value GetValue() = 0;
		virtual std::string GetText() const = 0;
		std::string text = "";
		CellInterface::Value value;
		bool change = true;
	};
	class EmptyImpl : public Impl {
	public:
		EmptyImpl(std::string text);
		CellInterface::Value GetValue() override;
		std::string GetText() const override;
	};
	class TextImpl : public Impl {
	public:
		TextImpl(std::string text);
		CellInterface::Value GetValue() override;
		std::string GetText() const override;
	};
	class FormulaImpl : public Impl {
	public:
		FormulaImpl(std::string text, SheetInterface* sheet_in);
		CellInterface::Value GetValue() override;
		std::string GetText() const override;
		SheetInterface* sheet;
	};


	void Set(std::string text, SheetInterface* sheet,Position pos);
	void Clear();

	Value GetValue() const override;
	std::string GetText() const override;

	void ClearValue();

	std::vector<Position> GetReferencedCells() const override;
	bool SearchCyclic(Position pos);
	bool SearchCyclicNext(Position pos, std::unordered_set<Position>* tested);
	SheetInterface* sheet;
	Position pos {-1,-1};
private:
	std::unique_ptr<Impl> impl_;
	std::vector<Position> dependence;
	std::vector<Position> dependent;
};

template<>
struct std::hash<Position>
{
	size_t
		operator()(const Position& obj) const
	{
		return hash<int>()(obj.col * 20000 + obj.row);
	}
};