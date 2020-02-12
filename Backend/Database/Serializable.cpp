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
#include "Serializable.h"
#include "GList.h"
#include "gtable.h"
#include "gtype.h"

using namespace shmea;

const char Serializable::ESC_CHAR = '%';
const std::string Serializable::NEED_ESCAPING = "%,\\|";

/*!
 * @brief escape separators
 * @details escape all "separators", i.e. delimiters in the regular text, so that they aren't
 * processed as delimiters on the receiving end
 * @param item a pointer to the item to serialize (pass by reference)
 * @param size the size of the item (pass by reference)
 * @return whether or not the operation succeeded
 */
bool Serializable::escapeSeparators(char** item, unsigned int& size)
{
	char* contents = *item;
	for (unsigned int i = 0; i < size; ++i)
	{
		if (Serializable::NEED_ESCAPING.find(contents[i]) == std::string::npos)
			continue;

		char* newContents = (char*)realloc(contents, size + 1);
		if (!newContents)
			return false;

		contents = newContents;
		*item = contents;
		memcpy(&contents[i + 1], &contents[i], size);
		contents[i] = Serializable::ESC_CHAR;
		++i;
		++size;
	}

	return true;
}

/*!
 * @brief add delimiter
 * @details adds the delimiter to the end of the item.
 *
 * the delimiter depends on whether or not this is the final item in the bundle.
 * if it is, the delimiter is `\|`. If not, the delimiter is `|`.
 * @param item a pointer to the item to serialize (pass by reference)
 * @param size the size of the item (pass by reference)
 * @param isLastItem whether or not this is the last item in the bundle.
 * @return whether or not the operation succeeded
 */
bool Serializable::addDelimiter(char** item, unsigned int& size, bool isLastItem)
{
	int delimiterLen = isLastItem ? 2 : 1;
	char* contents = *item;
	char* newContents = (char*)realloc(contents, size + delimiterLen);
	if (!newContents)
		return false;

	contents = newContents;
	*item = contents;
	if (delimiterLen == 1)
		contents[size] = '|';
	else if (delimiterLen == 2)
	{
		contents[size] = '\\';
		contents[size + 1] = '|';
	}

	size += delimiterLen;
	return true;
}

/*!
 * @brief add item to serial
 * @details adds an item (GType components) to a serial
 * the item should look like:
 * type,size,block| or type,size,block\|
 * and should be appended to the current serial
 * @param serialRef a pointer to the current serial
 * @param serialLen the length of the current serial
 * @param block the contents of the GType
 * @param itemSize the size of the contents, including escaped characters
 * @param realSize the actual size of the contents
 * @param itemType the type of the contents (INT_TYPE, STRING_TYPE, etc.)
 * @return the GTable version of the bundle
 */
bool Serializable::addItemToSerial(char** serialRef, unsigned int& serialLen, char* block,
								   int itemSize, int realSize, int itemType)
{
	// increase the size of the retblock (2 for the commas)
	char* serial = *serialRef;
	char* newSerial =
		(char*)realloc(serial, serialLen + itemSize + sizeof(int) + sizeof(int64_t) + 2);
	if (!newSerial)
		return false;

	// append the type
	serial = newSerial;
	*serialRef = serial;
	memcpy(&serial[serialLen], &itemType, sizeof(int));
	serialLen += sizeof(int);

	// add the first comma
	serial[serialLen] = ',';
	++serialLen;

	// append the cBlock size (with no escape characters)
	unsigned int lenMostHalf = realSize / 0x100000000;
	unsigned int lenLeastHalf = realSize - (lenMostHalf * 0x100000000);
	memcpy(&serial[serialLen], &lenLeastHalf, sizeof(int));
	serialLen += sizeof(int);
	memcpy(&serial[serialLen], &lenMostHalf, sizeof(int));
	serialLen += sizeof(int);

	// add the last comma
	serial[serialLen] = ',';
	++serialLen;

	// append the cBlock
	memcpy(&serial[serialLen], block, itemSize);
	serialLen += itemSize;
	return true;
}

/*!
 * @brief serialize item
 * @details turn a GType into a char and append it to a serial
 * @param item the GType to serialize
 * @param isLastItem whether or not this item is the last item to add
 * @param serial a pointer to the serial (pass by reference)
 * @param serialLen the length of the serial to which we're adding the GItem (pass by reference)
 * @return whether or not the operation succeeded
 */
bool Serializable::serializeItem(const GType& item, bool isLastItem, char** serial,
								 unsigned int& serialLen)
{
	int cType = item.getType();
	unsigned int cBlockSize = item.size();
	char* cBlock = item.getBlockCopy();

	if (!escapeSeparators(&cBlock, cBlockSize) || !addDelimiter(&cBlock, cBlockSize, isLastItem) ||
		!addItemToSerial(serial, serialLen, cBlock, cBlockSize, item.size(), cType))
	{
		free(cBlock);
		return false;
	}

	free(cBlock);
	return true;
}

/*!
 * @brief delimiter checker
 * @details checks whether or not the character sequence beginning at text[start] is the specified
 * delimiter (basic string matching from the middle of a string)
 * @param text the text to search
 * @param start the starting idex in text
 * @param delimiter the delimiter to search for
 * @return whether or not delimiter is present at text[start]
 */
bool Serializable::isDelimiterAt(const char* text, int start, const char* delimiter)
{
	for (int i = 0; delimiter[i] != '\0'; ++i)
	{
		if (text[start + i] != delimiter[i])
			return false;
	}

	return true;
}

/*!
 * @brief finds next delimiter
 * @details finds the index of the next appearance of `delimiter` in `text` after startIdx
 * @param text the string to search
 * @param startIdx the starting point for the search
 * @param delimiter the delimiter to find
 * @param textLen the length of the string to search
 * @param delLen the length of the delimiter to search
 * @return the index of text where delimiter appears, or -1 if it doesn't appear
 */
int Serializable::findNextDelimiterIndex(const char* text, int startIdx, const char* delimiter,
										 const int textLen, const int delLen)
{
	for (int i = startIdx; i < textLen - delLen + 1; ++i)
	{
		if (isDelimiterAt(text, i, delimiter))
			return i;
	}

	return -1;
}

/*!
 * @brief ???
 * @details needs better description
 * @param index the starting index
 * @param text the text to search (??)
 * @return whether or not the character at text[index] is escaped?
 */
bool Serializable::isEscaped(const int index, const char* text)
{
	int counter = 0;
	while ((index > 0) && (text[index - counter - 1] == Serializable::ESC_CHAR))
		++counter;

	return (counter % 2 != 0);
}

/*!
 * @brief get delimiter index
 * @details finds the first index of the of the (escaped or unescaped) delimiter in the string
 * @param text the string to seach
 * @param textLen the length of the string to search
 * @param delimiter the delimiter for which to search
 * @param delLen the length of the delimiter
 * @param ecaped should we be looking for an escaped delimiter? or an unescaped delimiter?
 * @return the first index of the text where the desired delimiter appears, or -1 if it doesn't
 */
int Serializable::getDelimiterIdx(const char* text, const int textLen, const char* delimiter,
								  const int delLen, bool escaped)
{
	int breakPoint = -1 * delLen;
	if ((!text) || (!delimiter) || (textLen <= 0) || (delLen <= 0))
		return -1;

	do
	{
		breakPoint = findNextDelimiterIndex(text, breakPoint + delLen, delimiter, textLen, delLen);
	} while ((escaped != isEscaped(breakPoint, text)) && (breakPoint >= 0));

	return breakPoint;
}

/*!
 * @brief deserialize type
 * @details deserialize the current item's type.
 *
 * The inbound data stream should look like:
 * type,size,contents|(rest of items)
 *
 * afterwards, it should look like:
 * size,contents|(rest of items)
 *
 * the process goes as follows:
 * - find the separator, i.e. ','
 * - pull all content before that (i.e. the size) out of the data
 * - calculate type
 * - shift the serial to remove the type + separator
 *
 * @param serialRef a pointer to the serial to deserialize
 * @param len length of the serial (pass by reference)
 * @param newBlockSize block size for the current item (pass by reference)
 * @return the type parameter for the item currently being extracted
 */
int Serializable::deserializeType(char** serialRef, int& len, int64_t& newBlockSize)
{
	const bool NOT_ESCAPED = false;
	char* serialBuffer = *serialRef;

	int subBreakPoint = getDelimiterIdx(serialBuffer, len, ",", 1, NOT_ESCAPED);
	if (subBreakPoint != sizeof(int))
		return GType::NULL_TYPE;

	int type;
	memcpy(&type, serialBuffer, subBreakPoint);

	// shift the serialBuffer (1 is for the comma)
	len -= subBreakPoint + 1;
	newBlockSize -= subBreakPoint + 1;
	memcpy(serialBuffer, &serialBuffer[subBreakPoint + 1], len);
	char* tempserialBuffer = (char*)realloc(serialBuffer, sizeof(char) * len);
	if (!tempserialBuffer)
		return GType::NULL_TYPE;

	serialBuffer = tempserialBuffer;
	*serialRef = serialBuffer;
	tempserialBuffer = NULL;

	return type;
}

/*!
 * @brief deserialize size
 * @details deserialize the current item's size from the serial.
 *
 * The inbound data stream should look like:
 * size,contents|(rest of items)
 *
 * afterwards, it should look like:
 * contents|(rest of items)
 *
 * the process goes as follows:
 * - find the separator, i.e. ','
 * - pull all content before that (i.e. the size) out of the serial
 * - calculate size
 * - shift the serial to remove the size + separator
 *
 * @param serialRef a pointer to the serial to deserialize
 * @param len length of the serial (updatd here)
 * @param newBlockSize block size for the current item (pass by reference)
 * @return the size parameter for the item currently being extracted
 */
int64_t Serializable::deserializeSize(char** serialRef, int& len, int64_t& newBlockSize)
{
	char* serialBuffer = *serialRef;
	const bool NOT_ESCAPED = false;

	int subBreakPoint = getDelimiterIdx(serialBuffer, len, ",", 1, NOT_ESCAPED);
	if (subBreakPoint != sizeof(int64_t))
		return -1;

	int64_t newSize = 0;
	unsigned int newSizeLeastHalf = *((unsigned int*)(serialBuffer));
	unsigned int newSizeMostHalf = *((unsigned int*)(&serialBuffer[sizeof(int)]));
	newSize += newSizeMostHalf * 0x100000000;
	newSize += newSizeLeastHalf;

	// shift the serialBuffer (1 is for the comma)
	len -= subBreakPoint + 1;
	newBlockSize -= subBreakPoint + 1;
	memcpy(serialBuffer, &serialBuffer[subBreakPoint + 1], len);
	char* tempserialBuffer = (char*)realloc(serialBuffer, sizeof(char) * len);
	if (!tempserialBuffer)
		return -1;

	serialBuffer = tempserialBuffer;
	*serialRef = serialBuffer;
	tempserialBuffer = NULL;
	return newSize;
}

/*!
 * @brief unescape character
 * @details unescape the current character in the block
 * @param blockRef a pointer to the block
 * @param newBlockSize the block size (pass by reference)
 * @param currentIdx the index of the escape character to remove
 * @return the number of escape characters removed, or -1 if the operation failed
 */
int Serializable::unescapeCharacter(char** blockRef, int64_t& newBlockSize, int currentIdx)
{
	char* unescapedBlock = *blockRef;
	if (unescapedBlock[currentIdx] != Serializable::ESC_CHAR)
		return 0;

	char* newBlock = (char*)malloc(newBlockSize - 1);
	if (!newBlock)
	{
		free(unescapedBlock);
		return -1;
	}

	// remove the escape character (assume its escaping something)
	memcpy(newBlock, unescapedBlock, currentIdx);
	memcpy(&newBlock[currentIdx], &unescapedBlock[currentIdx + 1], newBlockSize - currentIdx - 1);
	--newBlockSize;
	free(unescapedBlock);
	unescapedBlock = newBlock;

	*blockRef = unescapedBlock;
	return 1;
}

/*!
 * @brief deserialize content
 * @details deserialize the current item's contents from the serial.
 *
 * The inbound data stream should look like:
 * contents|(rest of items)
 *
 * afterwards, it should look like:
 * (rest of items)
 *
 * The process goes as follows:
 * - extract the contents from the full data
 * - ensure all characters in contents are escaped. If not, escape them.
 * - shift the rest of the contents in the data over to clear the just-extracted contents.
 *
 * @param serialRef a pointer to the serial to deserialize
 * @param len length of the serial (pass by reference)
 * @param newBlockSize block size for the current item (pass by reference)
 * @param newSize the expected size for the contents
 * @param delimiterLen the length of the ending delimiter (1 = |, 2 = \\|)
 * @return the contents of the item currently being extracted
 */
char* Serializable::deserializeContent(char** serialRef, int& len, int64_t& newBlockSize,
									   int64_t newSize, int delimiterLen)
{
	char* unescapedBlock = (char*)malloc(newBlockSize);
	char* serialBuffer = *serialRef;
	memcpy(unescapedBlock, serialBuffer, newBlockSize);
	int escCount = 0;

	// escape all characters
	for (int j = 0; j < newBlockSize; ++j)
	{
		int escChars = unescapeCharacter(&unescapedBlock, newBlockSize, j);
		if (escChars == -1)
		{
			free(unescapedBlock);
			return NULL;
		}
		escCount += escChars;
	}

	if (newSize != newBlockSize)
		return NULL;

	len -= newBlockSize + delimiterLen;
	if (len < 0)
		return NULL;
	else if (len == 0)
		return unescapedBlock;

	memcpy(serialBuffer, &serialBuffer[newBlockSize + escCount + delimiterLen], len);
	char* tempserialBuffer = (char*)realloc(serialBuffer, sizeof(char) * len);
	if (!tempserialBuffer)
		return NULL;

	serialBuffer = tempserialBuffer;
	*serialRef = serialBuffer;

	return unescapedBlock;
}

/*!
 * @brief Create a new serial from GList
 * @details Turn the GList into a serial
 * @param newList the table to serialize
 * @param serial the new serial
 * @return the length of the new serial
 */
int Serializable::Serialize(const GList& itemizedTable, char** serial)
{
	// Serialize the Itemized List (GList)
	unsigned int len = 0;
	char* retBlock = (char*)malloc(sizeof(char) * len);
	for (unsigned int i = 0; i < itemizedTable.size(); ++i)
	{
		if (!serializeItem((itemizedTable)[i], i == itemizedTable.size() - 1, &retBlock, len))
		{
			free(retBlock);
			return 0;
		}
	}

	*serial = retBlock;
	return len;
}

/*!
 * @brief Create a new serial from GTable
 * @details Turn the GTable into a serial
 * @param cTable the table to serialize
 * @param serial the new serial
 * @return the length of the new serial
 */
/*int Serializable::Serialize(const GTable& cTable, char** serial)
{
	int rows = cTable->numberOfRows();
	int columns = cTable->numberOfCols();
	int r, c;

	// metadata at the front
	GList cList;
	cList.addChar(cTable->delimiter);
	cList.addFloat(cTable->xMin);
	cList.addFloat(cTable->xMax);
	cList.addFloat(cTable->xRange);
	cList.addBoolean(cTable->outputsSelected);
	cList.addInt(rows);
	cList.addInt(columns);

	// the header
	for (c = 0; c < columns; ++c)
		cList.addString(cTable->getHeader(c));

	// the output columns
	for (c = 0; c < columns; ++c)
		cList.addBoolean(cTable->isOutput(c));

	// the contents
	for (r = 0; r < rows; ++r)
	{
		for (c = 0; c < columns; ++c)
			cList.addGType(cTable->getCell(r, c));
	}

	return Serialize(cList, serial);
}*/

/*!
 * @brief serial to GList
 * @details deserializes a serial into a GList
 *
 * We expect the serial to look like this:
 * type,size,contents|type,size,contents|...|type,size,contents\|
 *
 * this function goes through, one item at a time, and extracts
 * the GType components (type, size, contents) and puts them in the list
 *
 * if any of the extractions fail, the loop quits and the partial list is returned
 *
 * @param serial the serial to extract
 * @param the length of the serial
 * @return the full list with all contents
 */
GList Serializable::DeserializeHelper(const char* serial, int len)
{
	GList cList;
	if ((!serial) || (len == 0))
		return cList;

	// copy the serial (keep the original intact)
	char* serialBuffer = (char*)malloc(sizeof(char) * (len));
	memcpy(serialBuffer, serial, len);

	int breakPoint;
	const bool NOT_ESCAPED = false;

	do
	{
		breakPoint = getDelimiterIdx(serialBuffer, len, "|", 1, NOT_ESCAPED);
		int bpEndOfserial = getDelimiterIdx(serialBuffer, len, "\\|", 2, NOT_ESCAPED);

		if ((breakPoint <= 0) || (bpEndOfserial <= 0))
			break;

		bool isLastBlock = (breakPoint == bpEndOfserial + 1);
		int64_t newBlockSize = isLastBlock ? bpEndOfserial : breakPoint;
		int delimiterLen = isLastBlock ? 2 : 1;

		int newType = deserializeType(&serialBuffer, len, newBlockSize);
		if (newType == GType::NULL_TYPE)
			break;

		int64_t newSize = deserializeSize(&serialBuffer, len, newBlockSize);
		if (newSize == -1)
			break;

		char* unescapedBlock =
			deserializeContent(&serialBuffer, len, newBlockSize, newSize, delimiterLen);

		if (!unescapedBlock)
			break;

		cList.addObject(newType, unescapedBlock, newBlockSize);
	} while (breakPoint > 0);

	// free the serialBuffer
	if (serialBuffer)
		free(serialBuffer);

	return cList;
}

/*!
 * @brief GList to GTable
 * @details Creates a GTable from a bundle
 * @return the GTable version of the bundle
 */
/*GTable Serializable::Deserialize(const char* serial, int len)
{
	GList cList = DeserializeHelper(serial, len);

	// metadata
	char delimiter = cList->getChar(0);
	float min = cList->getFloat(1), max = cList->getFloat(2), range = cList->getFloat(3);
	bool outputsSelected = cList->getBoolean(4);
	int rows = cList->getInt(5), columns = cList->getInt(6);
	int bundleIndex = 7;

	// the header
	std::vector<std::string> header;
	for (int i = 0; i < columns; ++i, ++i)
		header.push_back(cList->getString(i));

	// the output columns
	std::vector<bool> outputColumns;
	for (int i = 0; i < columns; ++i, ++i)
		outputColumns.push_back(cList->getBoolean(i));

	GTable* cTable = new GTable(delimiter);
	cTable->setMin(min);
	cTable->setMax(max);
	cTable->setRange(range);
	cTable->header = header;
	cTable->outputColumns = outputColumns;
	cTable->outputsSelected = outputsSelected;

	// the contents
	for (int i = 0; i < rows; ++i)
	{
		std::vector<GType*> row;
		for (int j = 0; j < columns; ++j, ++j)
			row.push_back((*cList)[j]);
		cTable->addRow(row);
	}

	int actual_size = cList->size();
	int expected_size = (rows + 2) * columns + bundleIndex;
	if (expected_size != actual_size)
	{
		printf("[SER] Bad GTable: Sizes(%d != %d)\n", actual_size, expected_size);
		if (!cTable)
			delete cTable;
		return NULL;
	}

	return cTable;
}*/
