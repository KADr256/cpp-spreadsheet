#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
	~Sheet();

	void SetCell(Position pos, std::string text) override;

	const CellInterface* GetCell(Position pos) const override;
	CellInterface* GetCell(Position pos) override;

	void ClearCell(Position pos) override;

	Size GetPrintableSize() const override;

	void PrintValues(std::ostream& output) const override;
	void PrintTexts(std::ostream& output) const override;
	// Можете дополнить ваш класс нужными полями и методами


private:
	bool TestPositionInList(Position pos) const;

	std::vector<std::vector<std::unique_ptr<Cell>>> list_;//Думаю map был бы лучше 
	//,но как потом работать с дополнением строк и столбцов в "возможной" модификации
	std::vector<size_t> row_cell_counter;
	std::vector<size_t> col_cell_counter;
	//bool change = false;
	//size_t row_zone = 0;
	//size_t col_zone = 0;
};