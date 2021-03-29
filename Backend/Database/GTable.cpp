// Copyright 2020 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "GTable.h"
#include "../Networking/main.h"
#include "GList.h"
#include "GType.h"

using namespace shmea;

/*!
 * @brief GTable default constructor
 * @details creates a GTable object
 */
GTable::GTable()
{
	clear();
	delimiter = ',';
}

/*!
 * @brief GTable delimiter constructor
 * @details creates a GTable object with a given delimiter
 * @param newDelimiter the specified table delimiter
 */
GTable::GTable(char newDelimiter)
{
	clear();
	delimiter = newDelimiter;
}

/*!
 * @brief GTable delimiter/header constructor
 * @details creates a GTable object with a given delimiter and headers
 * @param newDelimiter the specified table delimiter
 * @param newHeaders the desired table headers
 */
GTable::GTable(char newDelimiter, const std::vector<std::string>& newHeaders)
{
	clear();
	delimiter = newDelimiter;
	header = newHeaders;
}
/*!
 * @brief GTable path/delimiter/flag constructor
 * @details Creates a GTable from a given path, delimiter, and import flag.
 * @param fname the path (URL, file path, or string) containing the data
 * @param newDelimiter the specified table delimiter
 * @param importFlag an import flag, specifying fname as either a file path, URL, or raw string
 */
GTable::GTable(const std::string& fname, char newDelimiter, int importFlag)
{
	clear();
	delimiter = newDelimiter;
	if (importFlag == TYPE_FILE)
		importFromFile(fname);
	else if (importFlag == TYPE_STRING)
		importFromString(fname);
}

/*!
 * @brief GTable copy constructor
 * @details Creates a GTable from another GTable's data
 * @param gtable2 a reference to the GTable we want to copy
 */
GTable::GTable(const GTable& gtable2)
{
	copy(gtable2);
}

/*!
 * @brief GTable destructor
 * @details Destroys a GTable object
 */
GTable::~GTable()
{
	clear();
}

/*!
 * @brief GTable data copy
 * @details copies data from another GTable into this GTable
 * @param gtable2 the reference to the GTable we want to copy
 */
void GTable::copy(const GTable& gtable2)
{
	delimiter = gtable2.delimiter;
	header = gtable2.header;
	cells = gtable2.cells;
	xMin = gtable2.xMin;
	xMax = gtable2.xMax;
	xRange = gtable2.xRange;
	outputColumns = gtable2.outputColumns;
}

/*!
 * @brief GTable file import
 * @details imports GTable data from a file
 * @param fname the file path to the desired data
 */
void GTable::importFromFile(const std::string& fname)
{
	if (fname.length() == 0)
		return;

	FILE* fd = fopen(fname.c_str(), "r");
	printf("[CSV] %c%s\n", (fd != NULL) ? '+' : '-', fname.c_str());

	if (!fd)
		return;

	// get the file size
	fseek(fd, 0, SEEK_END);
	int64_t fSize = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	// Allocate a buffer
	int rowCounter = 0;
	int MAX_LINE_SIZE = 256;
	int linesRead = 0; // Are the lines read, not how many lines read
	char* buffer = (char*)malloc(MAX_LINE_SIZE * sizeof(char));

	do
	{
		shmea::GList newRow;
		bzero(buffer, MAX_LINE_SIZE);

		// get the current line
		char readBuffer[MAX_LINE_SIZE];
		if (fgets(readBuffer, sizeof(readBuffer), fd) != 0)
			linesRead = sscanf(readBuffer, "%[^\n]s", buffer);
		else
			linesRead = 0;

		// EOF or error
		if (linesRead <= 0)
			break;

		// Read each column
		int colCounter = 0;
		bool lastCol = false;
		std::string line(buffer);
		int breakPoint = line.find(delimiter);
		while (breakPoint != -1)
		{
			// get the cell
			std::string word = line.substr(0, breakPoint);
			if (!lastCol)
				line = line.substr(breakPoint + 1);

			// add the col to the row
			if (rowCounter == 0)
				header.push_back(word);
			else
			{
				GType newCell(word);
				newRow.addGType(newCell);
			}

			// for the next column
			breakPoint = line.find(delimiter);
			++colCounter;

			// last column?
			if ((breakPoint == -1) && (line.length() > 0) && (!lastCol))
			{
				breakPoint = line.length();
				lastCol = true;
			}
		}

		// add the row to the object
		if (rowCounter > 0)
			addRow(newRow);
		++rowCounter;

	} while ((linesRead > 0) && (ftell(fd) < fSize));

	// EOF
	free(buffer);
	fclose(fd);
}

/*!
 * @brief GTable String import
 * @details imports data from a raw string
 * @param content the GTable data as a string
 */
void GTable::importFromString(const std::string& newContent)
{
	if (!(newContent.length() > 0))
	{
		printf("[CSV] Load from string error.\n");
		return;
	}

	// Allocate a buffer
	std::string content = newContent;
	int rowCounter = 0;
	do
	{
		shmea::GList newRow;
		int breakPoint = content.find('\n');
		if (breakPoint == -1)
			break;

		// get the current line
		std::string line = content.substr(0, breakPoint);
		content = content.substr(breakPoint + 1);

		// Read each column
		int colCounter = 0;
		bool lastCol = false;
		int breakPoint2 = line.find(delimiter);
		do
		{
			// last column?
			if ((breakPoint2 == -1) && (line.length() > 0) && (!lastCol))
			{
				breakPoint2 = line.length();
				lastCol = true;
			}

			// get the cell
			std::string word = line.substr(0, breakPoint2);
			if (!lastCol)
			{
				line = line.substr(breakPoint2 + 1);
				breakPoint2 = line.find(delimiter);
			}

			// add the col to the row
			if (rowCounter == 0)
				header.push_back(word);
			else
				newRow.addString(word);

			// for the next column
			++colCounter;

		} while (!lastCol);

		// add the row to the object
		if (rowCounter > 0)
			addRow(newRow);
		++rowCounter;
	} while (content.length() > 0);
}

/*!
 * @brief get GTable delimiter
 * @details retrieve the GTable's delimiter
 * @return the GTable's delimiter
 */
char GTable::getDelimiter() const
{
	return delimiter;
}

/*!
 * @brief get all GTable headers
 * @details retrieve the GTable's headers
 * @return the header vector
 */
std::vector<std::string> GTable::getHeaders() const
{
	return header;
}

/*!
 * @brief get a GTable header
 * @details retrieve the GTable's header for a specific column
 * @param index the index at which the desired header resides
 * @return the header for the given index
 */
std::string GTable::getHeader(unsigned int index) const
{
	if (index >= header.size())
		return " "; // Keep this a space character

	return header[index];
}

/*!
 * @brief get GTable cell's value
 * @details retrieve the value at a GTable cell specified by a row number and column number
 * @param row the row number of the desired cell
 * @param col the column number of the desired cell
 * @return a pointer to a GTable cell of type GType
 */
GType GTable::getCell(unsigned int row, unsigned int col) const
{
	if (row >= numberOfRows())
		return 0;

	if (col >= numberOfCols())
		return 0;

	return cells[row].getGType(col);
}

/*!
 * @brief get GTable row
 * @details retrieve a full GTable row specified by a row number
 * @param row the row number of the row to retrieve
 * @return the GTable row, a vector of GTable values
 */
shmea::GList GTable::getRow(unsigned int rowCounter) const
{
	shmea::GList emptyRow;
	if (rowCounter >= numberOfRows())
		return emptyRow;

	return cells[rowCounter];
}

/*!
 * @brief number of columns
 * @details the number of columns in a GTable
 * @return the number of columns in a GTable
 */
unsigned int GTable::numberOfCols() const
{
	if (numberOfRows() > 0)
		return cells[0].size();

	return 0;
}

/*!
 * @brief number of rows
 * @details the number of rows in a GTable
 * @return the number of rows in a GTable
 */
unsigned int GTable::numberOfRows() const
{
	return cells.size();
}

/*!
 * @brief GTable
 * @details prints a GTable
 */
void GTable::print() const
{
	printHeaders();

	// print the data
	for (unsigned int r = 0; r < numberOfRows(); ++r)
	{
		for (unsigned int c = 0; c < numberOfCols(); ++c)
		{
			// get the data by cell
			GType cCell = getCell(r, c);
			if (cCell.getType() == GType::STRING_TYPE)
				printf("%s", cCell.c_str());
			else if (cCell.getType() == GType::CHAR_TYPE)
				printf("%c", cCell.getChar());
			else if (cCell.getType() == GType::SHORT_TYPE)
				printf("%d", cCell.getShort());
			else if (cCell.getType() == GType::INT_TYPE)
				printf("%d", cCell.getInt());
			else if (cCell.getType() == GType::LONG_TYPE)
				printf("%ld", cCell.getLong());
			else if (cCell.getType() == GType::FLOAT_TYPE)
				printf("%f", cCell.getFloat());
			else if (cCell.getType() == GType::DOUBLE_TYPE)
				printf("%f", cCell.getDouble());
			else if (cCell.getType() == GType::BOOLEAN_TYPE)
				printf("%s", cCell.getBoolean() ? "True" : "False");

			// comma?
			if (c < numberOfCols() - 1)
				printf(",");
		}
		printf("\n");
	}
}

/*!
 * @brief GTable Headers
 * @details prints GTable headers
 */
void GTable::printHeaders() const
{
	for (unsigned int i = 0; i < header.size(); ++i)
	{
		std::string word = header[i];
		if (isOutput(i))
			printf("*");

		printf("%s", word.c_str());
		if (i < header.size() - 1)
			printf(",");
	}
	printf("\n");
}

/*!
 * @brief set GTable cell value
 * @details sets a GTable cell's value to the provided new value
 * @param row the row number of the cell to change
 * @param col the column number of the cell to change
 * @param newVal the new value to store in the cell
 */
void GTable::setCell(unsigned int row, unsigned int col, const GType& newVal)
{
	if ((row < numberOfRows()) && (col < numberOfCols()))
		cells[row].setGType(col, newVal);
}

/*!
 * @brief add GTable row
 * @details adds a row of values to the end of a GTable
 * @param newRow the new row to add
 */
void GTable::addRow(const shmea::GList& newRow)
{
	cells.push_back(newRow);
}

/*!
 * @brief remove GTable row
 * @details remove a row (specified by row number) from the GTable
 * @param index the row number to erase
 */
void GTable::removeRow(unsigned int index)
{
	// error checking
	if (index >= cells.size())
		return;

	cells.erase(cells.begin() + index);
}

/*!
 * @brief clear a GTable
 * @details remove all data from a GTable
 */
void GTable::clear()
{
	delimiter = ',';
	header.clear();
	cells.clear();
	xMin = 0.0f;
	xMax = 0.0f;
	xRange = 0.0f;
	clearOutputs();
}

/*!
 * @brief add GTable column
 * @details add a column to a GTable with a given header
 * @param headerName the header name for the new column
 * @param newCol the new column data as a vector
 * @param index the index at which to add the GTable column
 */
void GTable::addCol(const std::string& headerName, const shmea::GList& newCol, unsigned int index)
{
	// error checking
	if ((newCol.size() != numberOfRows()) && (numberOfRows() > 0))
	{
		printf("[CSV] Invalid col size: %u != %s:%lu\n", numberOfRows(), headerName.c_str(),
			   newCol.size());
		return;
	}

	// Add to the back by default
	if (index > numberOfCols())
		index = numberOfCols();

	// add the column
	if (numberOfRows() > 0)
	{
		for (unsigned int r = 0; r < numberOfRows(); ++r)
		{
			if (index == numberOfCols())
				cells[r].addGType(newCol.getGType(r));
			else
				cells[r].insertGType(index, newCol.getGType(r));
		}
	}
	else
	{
		// if this is the first col in the table
		for (unsigned int i = 0; i < newCol.size(); ++i)
		{
			shmea::GList cCol;
			cCol.addGType(newCol.getGType(i));
			cells.push_back(cCol);
		}
	}

	// add the header
	addHeader(index, headerName);
}

/*!
 * @brief remove GTable column
 * @details remove a column at the given index from the GTable
 * @param index the index of the column to remove
 */
void GTable::removeCol(unsigned int index)
{
	// error checking
	if (index >= numberOfCols())
		return;

	// remove column by row
	for (unsigned int r = 0; r < numberOfRows(); ++r)
		cells[r].remove(index);

	// remove header
	if (index < header.size())
		header.erase(header.begin() + index);

	// Remove the output column
	for (unsigned int i = 0; i < outputColumns.size(); ++i)
	{
		if (index == outputColumns[i])
		{
			outputColumns.erase(outputColumns.begin() + i);
			break;
		}
	}
}

/*!
 * @brief swap GTable columns
 * @details swap columns between two indices
 * @param index the index of the first column
 * @param index2 the index of the second column
 */
void GTable::swapCol(unsigned int index, unsigned int index2)
{
	// error checking
	if (index == index2)
		return;

	if (index >= numberOfCols() || index2 >= numberOfCols())
		return;

	// Make sure index is smaller than index2
	if (index > index2)
	{
		int tempIndex = index;
		index = index2;
		index2 = tempIndex;
	}

	// swap headers
	std::string cHeader1 = "";
	if (!(index >= header.size()))
		cHeader1 = header[index];

	std::string cHeader2 = "";
	if (!(index2 >= header.size()))
		cHeader2 = header[index2];

	// swap columns
	shmea::GList col1 = getCol(index);
	shmea::GList col2 = getCol(index2);
	removeCol(index);
	removeCol(index2);
	addCol(cHeader1, col1, index2);
	addCol(cHeader2, col2, index);
}

/*!
 * @brief move GTable column
 * @details move column from index to index2
 * @param index the index of the column to be moved
 * @param index2 the new index of the column
 */
void GTable::moveCol(unsigned int index, unsigned int index2)
{
	// error checking
	if (index >= numberOfCols())
		return;

	if (index2 >= numberOfCols())
		return;

	// swap headers
	std::string cHeader1 = "";
	if (!(index >= header.size()))
		cHeader1 = header[index];

	std::string cHeader2 = "";
	if (!(index2 >= header.size()))
		cHeader2 = header[index2];

	// goes nowhere
	if (index == index2)
		return;

	// copy the column
	addCol(cHeader1, getCol(index), index2);

	// which way did it shift, remove the old
	if (index > index2)
		removeCol(index + 1);
	else
		removeCol(index);
}

/*!
 * @brief append two GTables
 * @details append table2 rows onto this
 * @param table2 the table to append onto this
 */
void GTable::append(const GTable& table2)
{
	if ((table2.numberOfRows() <= 0) || (table2.numberOfCols() <= 0))
		return;

	// Short circuiting
	if ((numberOfRows() > 0) && (numberOfCols() != table2.numberOfCols()))
		return;

	if (numberOfRows() == 0)
	{
		copy(table2);
		return;
	}

	// add each row, we should change this to copy a block of mem
	for (unsigned int r = 0; r < table2.numberOfRows(); ++r)
		addRow(table2.getRow(r));

	// add the headers
	setHeaders(table2.getHeaders());

	// add the outputs
	/*for (unsigned int i = numberOfCols(); i < table2.numberOfCols(); ++i)
	{
		if (table2.isOutput(i))
			toggleOutput(i);
	}*/
}

/*!
 * @brief append two GTables
 * @details append table2 rows onto this
 * @param table2 the table to append onto this
 */
void GTable::append(const GTable* table2)
{
	if (!table2)
		return;

	append(*table2);
}

/*!
 * @brief get GTable column with index
 * @details retrieve the column data at the given index in the GTable
 * @param index the index for the desired column data
 * @return the column's data
 */
shmea::GList GTable::getCol(unsigned int index) const
{
	shmea::GList col;
	if (index >= numberOfCols())
		return col;

	// get column
	for (unsigned int r = 0; r < numberOfRows(); ++r)
		col.addGType(cells[r].getGType(index));

	return col;
}

/*!
 * @brief get GTable column with header (C string)
 * @details retrieve the column data for the given header in the GTable
 * @param headerSearchText the desired-column's header as a C-style string
 * @return the column's data
 */
shmea::GList GTable::getCol(const char* headerSearchText) const
{
	std::string newHeaderStr(headerSearchText);
	shmea::GList retCol = getCol(newHeaderStr);
	return retCol;
}

/*!
 * @brief get GTable column with header (string)
 * @details retrieve the column data for the given header in the GTable
 * @param headerSearchText the desired-column's header as a string
 * @return the column's data
 */
shmea::GList GTable::getCol(const std::string& headerSearchText) const
{
	// does the column exist
	unsigned int index = -1;
	for (unsigned int i = 0; i < header.size(); ++i)
	{
		if (strcmp(headerSearchText.c_str(), header[i].c_str()) == 0)
		{
			index = i;
			break;
		}
	}

	// get the column if it exists
	if (index < numberOfCols())
	{
		shmea::GList retCol = getCol(index);
		return retCol;
	}

	shmea::GList emptyCol;
	return emptyCol;
}

/*!
 * @brief GTable bracket operator [C string]
 * @details retrieves a GTable column with a given header
 * @param headerSearchText the header for the desired column (as a C-string)
 * @return the desired column's data (if it exists)
 */
shmea::GList GTable::operator[](const char* headerSearchText) const
{
	std::string newHeaderStr(headerSearchText);
	shmea::GList retCol = getCol(newHeaderStr);
	return retCol;
}

/*!
 * @brief GTable bracket operator [string]
 * @details retrieves a GTable column with a given header
 * @param headerSearchText the header for the desired column (as a string)
 * @return the desired column's data (if it exists)
 */
shmea::GList GTable::operator[](const std::string& headerSearchText) const
{
	shmea::GList retCol = getCol(headerSearchText);
	return retCol;
}

void GTable::operator=(const GTable& gtable2)
{
	copy(gtable2);
}

/*!
 * @brief get GTable minimum value
 * @details retrieves the minimum value stored in the GTable's cells (calculated elsewhere)
 * @return the GTable's minimum value
 */
float GTable::getMin() const
{
	return xMin;
}

/*!
 * @brief get GTable maximum value
 * @details retrieves the maximum value stored in the GTable's cells (calculated elsewhere)
 * @return the GTable's maximum value
 */
float GTable::getMax() const
{
	return xMax;
}

/*!
 * @brief get GTable range
 * @details get the range (max value - min value) of the GTable's values
 * @return the GTable's range
 */
float GTable::getRange() const
{
	return xRange;
}

/*!
 * @brief set new GTable headers
 * @details set the headers of the GTable to new values
 * @param newHeader the new set of headers for the GTable
 */
void GTable::setHeaders(const std::vector<std::string>& newHeader)
{
	header = newHeader;
}

/*!
 * @brief set a new GTable header
 * @details set the header at a specified index of the GTable to a new value
 * @param index the column index at which to change the header string
 * @param newHeader the new header string
 */
void GTable::addHeader(unsigned int index, const std::string& newHeader)
{
	// error checking
	if (index >= header.size())
		header.push_back(newHeader);
	else
		header.insert(header.begin() + index, newHeader);
}

/*!
 * @brief save GTable
 * @details save the GTable to a file
 * @param fname the file path at which to save the GTable data
 */
void GTable::save(const std::string& fname) const
{
	if (fname.length() <= 0)
		return;

	FILE* fd = fopen(fname.c_str(), "w");
	if (fd != NULL)
	{
		//
		for (unsigned int i = 0; i < header.size(); ++i)
		{
			std::string word = header[i];
			fprintf(fd, "%s", word.c_str());
			if (i < header.size() - 1)
				fprintf(fd, ",");
		}
		fprintf(fd, "\n");

		// save it as the proper type
		for (unsigned int r = 0; r < numberOfRows(); ++r)
		{
			for (unsigned int c = 0; c < numberOfCols(); ++c)
			{
				GType cCell = getCell(r, c);
				if (cCell.getType() == GType::STRING_TYPE)
				{
					std::string word = getCell(r, c).getString();
					fprintf(fd, "%s", word.c_str());
				}
				else if (cCell.getType() == GType::CHAR_TYPE)
				{
					char word = getCell(r, c).getChar();
					fprintf(fd, "%c", word);
				}
				else if (cCell.getType() == GType::SHORT_TYPE)
				{
					short word = getCell(r, c).getShort();
					fprintf(fd, "%d", (int)word);
				}
				else if (cCell.getType() == GType::INT_TYPE)
				{
					int word = getCell(r, c).getInt();
					fprintf(fd, "%d", word);
				}
				else if (cCell.getType() == GType::LONG_TYPE)
				{
					int64_t word = getCell(r, c).getLong();
					fprintf(fd, "%ld", word);
				}
				else if (cCell.getType() == GType::FLOAT_TYPE)
				{
					float word = getCell(r, c).getFloat();
					fprintf(fd, "%f", word);
				}
				else if (cCell.getType() == GType::DOUBLE_TYPE)
				{
					double word = getCell(r, c).getDouble();
					fprintf(fd, "%f", word);
				}
				else if (cCell.getType() == GType::BOOLEAN_TYPE)
				{
					bool word = getCell(r, c).getBoolean();
					fprintf(fd, "%d", word ? 1 : 0);
				}

				if (c < numberOfCols() - 1)
					fprintf(fd, ",");
			}
			fprintf(fd, "\n");
		}

		fclose(fd);
	}
}

/*!
 * @brief single GTable stratification
 * @details stratify a GTable; that is, divide it into groups of a given number of rows
 * @param inputSet the GTable to stratify
 * @param k the number of rows per sub-grouping
 * @return the stratified subpgroups (a vector of GTables)
 */
std::vector<GTable*> GTable::stratify(const GTable& inputSet, unsigned int k)
{
	std::vector<GTable*> outputSet;
	// Initialize the outputSet
	// int blockSizeK=inputSet.numberOfRows()/k;
	for (unsigned int fold = 0; fold < k; ++fold)
	{
		GTable* newTable = new GTable(inputSet.getDelimiter());
		newTable->header = inputSet.header;
		newTable->outputColumns = inputSet.outputColumns;
		outputSet.push_back(newTable);
	}

	unsigned int rowCounter = 0;
	while (rowCounter < inputSet.numberOfRows())
	{
		for (unsigned int fold = 0; fold < k; ++fold)
		{

			// Last k can be less than k items long, this breaks at true end of inputSet
			if (rowCounter + fold >= inputSet.numberOfRows())
				break;

			outputSet[fold]->addRow(inputSet.getRow(rowCounter));
		}

		rowCounter += k;
	}

	return outputSet;
}

/*!
 * @brief multiple GTable stratification
 * @details stratify several GTables; that is, divide their collective rows into groups of a given
 * number of rows
 * @param inputSet the GTables to stratify
 * @param k the number of rows per sub-grouping
 * @return the stratified subpgroups (a vector of GTables)
 */
std::vector<GTable*> GTable::stratify(const std::vector<GTable*> inputSet, unsigned int k)
{
	std::vector<GTable*> outputSet;
	// nothing here
	if (inputSet.size() <= 0)
		return outputSet;

	// Initialize the outputSet
	unsigned int sampleSize = 0;
	for (unsigned int inputCounter = 0; inputCounter < inputSet.size(); ++inputCounter)
		sampleSize += inputSet[inputCounter]->numberOfRows();
	for (unsigned int fold = 0; fold < k; ++fold)
		// currently requires that all delimiters are the same as the first file's delimiter
		outputSet.push_back(new GTable(inputSet[0]->getDelimiter()));

	// Stratify into k-folds
	unsigned int completedSamples = 0;
	unsigned int inputCounter = 0;
	for (unsigned int sampleCount = 0; sampleCount < sampleSize; ++sampleCount)
	{
		// if we've exhausted a file, move on to the next
		printf("[CSV] Stratify File Check: %d\n",
			   ((sampleCount - completedSamples) >= inputSet[inputCounter]->numberOfRows()));
		if ((sampleCount - completedSamples) >= inputSet[inputCounter]->numberOfRows())
		{
			completedSamples = sampleCount;
			inputCounter = inputCounter + 1;
		}

		outputSet[sampleCount % k]->addRow(
			inputSet[inputCounter]->getRow(sampleCount - completedSamples));
	}

	// print new the csvs
	for (unsigned int outputCounter = 0; outputCounter < outputSet.size(); ++outputCounter)
	{
		outputSet[outputCounter]->print();
	}

	return outputSet;
}

/*!
 * @brief standardize GTable
 * @details standardize the values in a GTable; that is, map the values from their existing range to
 * the range of -1.0 to 1.0
 */
void GTable::standardize()
{
	// Standardize the initialization of the weights
	if ((numberOfRows() <= 0) || (numberOfCols() <= 0))
		return;

	// Set the min and max of the weights
	xMin = 0.0f;
	xMax = 0.0f;

	// iterate through the rows
	for (unsigned int r = 0; r < numberOfRows(); ++r)
	{
		// iterate through the cols
		for (unsigned int c = 0; c < numberOfCols(); ++c)
		{
			GType cCell = getCell(r, c);
			float cell = 0.0f;
			if (cCell.getType() == GType::STRING_TYPE)
			{
				// OHE: total unique words
			}
			else if (cCell.getType() == GType::CHAR_TYPE)
				cell = cCell.getChar();
			else if (cCell.getType() == GType::SHORT_TYPE)
				cell = cCell.getShort();
			else if (cCell.getType() == GType::INT_TYPE)
				cell = cCell.getInt();
			else if (cCell.getType() == GType::LONG_TYPE)
				cell = cCell.getLong();
			else if (cCell.getType() == GType::FLOAT_TYPE)
				cell = cCell.getFloat();
			else if (cCell.getType() == GType::DOUBLE_TYPE)
				cell = cCell.getDouble();
			else if (cCell.getType() == GType::BOOLEAN_TYPE)
				cell = cCell.getBoolean() ? 1.0f : 0.0f;

			if ((r == 0) && (c == 0))
			{
				xMin = cell;
				xMax = cell;
			}

			// Check the mins and maxes
			if (cell < xMin)
				xMin = cell;
			if (cell > xMax)
				xMax = cell;
		}
	}

	// standardize the weights
	xRange = xMax - xMin;
	if (xRange == 0.0f)
		return;
	// iterate through the rows
	for (unsigned int r = 0; r < numberOfRows(); ++r)
	{
		// iterate through the cols
		for (unsigned int c = 0; c < numberOfCols(); ++c)
		{
			// Adjust the children
			GType cCell = getCell(r, c);
			float cell = 0.0f;
			if (cCell.getType() == GType::STRING_TYPE)
			{
				// OHE: total unique words
			}
			else if (cCell.getType() == GType::CHAR_TYPE)
			{
				cell = cCell.getChar();
				cell = (((cell - xMin) / (xRange)) - 0.5f);
				cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
			}
			else if (cCell.getType() == GType::SHORT_TYPE)
			{
				cell = cCell.getShort();
				cell = (((cell - xMin) / (xRange)) - 0.5f);
				cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
			}
			else if (cCell.getType() == GType::INT_TYPE)
			{
				cell = cCell.getInt();
				cell = (((cell - xMin) / (xRange)) - 0.5f);
				cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
			}
			else if (cCell.getType() == GType::LONG_TYPE)
			{
				cell = cCell.getLong();
				cell = (((cell - xMin) / (xRange)) - 0.5f);
				cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
			}
			else if (cCell.getType() == GType::FLOAT_TYPE)
			{
				cell = cCell.getFloat();
				cell = (((cell - xMin) / (xRange)) - 0.5f);
				cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
			}
			else if (cCell.getType() == GType::DOUBLE_TYPE)
			{
				cell = cCell.getDouble();
				cell = (((cell - xMin) / (xRange)) - 0.5f);
				cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
			}
			else if (cCell.getType() == GType::BOOLEAN_TYPE)
			{
				cell = cCell.getBoolean() ? 1.0f : 0.0f; // 1 or 0 if sigmoid
				cell = (((cell - xMin) / (xRange)) - 0.5f);
				cCell.set(GType::FLOAT_TYPE, &cell, sizeof(float));
			}
		}
	}
}

/*!
 * @brief unstandardize GTable value
 * @details reverse the standardization for a given value; that is, take a given value (between -1.0
 * and 1.0) and map it within the original value range
 * @param value the standardized value (between -1.0 and 1.0) to map
 * @return the value mapped to the original value range
 */
float GTable::unstandardize(float value) const
{
	return ((value + 0.5f) * xRange) + xMin;
}

/*!
 * @brief mark an output column
 * @details mark a column as output for use in the neural net(s)
 * @param index of output column
 */
void GTable::toggleOutput(unsigned int column)
{
	if (column >= header.size())
		return;

	bool toggled = false;
	for (unsigned int i = 0; i < outputColumns.size(); ++i)
	{
		if (column == outputColumns[i])
		{
			outputColumns.erase(outputColumns.begin() + i);
			toggled = true;
			break;
		}
		else if (outputColumns[i] > column)
		{
			outputColumns.insert(outputColumns.begin() + i, column);
			toggled = true;
			break;
		}
	}

	if (!toggled)
		outputColumns.push_back(column);
}

/*!
 * @brief clears output column labels
 * @details clears any columns labeled as output for the neural net and returns gtable to default
 * output behavior
 */
void GTable::clearOutputs()
{
	outputColumns.clear();
}

void GTable::setMin(float newMin)
{
	xMin = newMin;
}
void GTable::setMax(float newMax)
{
	xMax = newMax;
}
void GTable::setRange(float newRange)
{
	xRange = newRange;
}
/*!
 * @brief checks if column is labeled output
 * @details checks if column at given index has been labeled as an output column for the neural net
 * @return bool of whether column is labeled output
 */
bool GTable::isOutput(unsigned int column) const
{
	if (column >= header.size())
		return false;

	// We manually set one
	for (unsigned int i = 0; i < outputColumns.size(); ++i)
	{
		if (column == outputColumns[i])
			return true;
	}

	// last col by default
	return (column == (header.size() - 1));
}

int GTable::numOutputColumns() const
{
	int numOutputCols = 0;
	for (unsigned int i = 0; i < outputColumns.size(); ++i)
		numOutputCols += outputColumns[i]; // 0 or 1
	return numOutputCols;
}

bool GTable::empty() const
{
	return !(numberOfRows() > 0);
}
