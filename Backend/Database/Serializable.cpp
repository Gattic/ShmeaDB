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
#include "GTable.h"
#include "GType.h"
#include "GObject.h"

using namespace shmea;

const char Serializable::ESC_CHAR = '%';
const GString Serializable::NEED_ESCAPING = "%,\\|";

/*!
 * @brief escape separators
 * @details escape all "separators", i.e. delimiters in the regular text, so that they aren't
 * processed as delimiters on the receiving end
 * @param item a pointer to the item to serialize (pass by reference)
 * @param size the size of the item (pass by reference)
 * @return whether or not the operation succeeded
 */
GString Serializable::escapeSeparators(const GType& serial)
{
	GString newSerial = serial;
	for (unsigned int i = 0; i < newSerial.size(); ++i)
	{
		if (Serializable::NEED_ESCAPING.cfind(newSerial[i]) == GString::npos)
			continue;

		newSerial = newSerial.substr(0, i) + Serializable::ESC_CHAR + newSerial.substr(i);
		++i;
	}

	return newSerial;
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
GString Serializable::addDelimiter(const GString& serial, bool isLastItem)
{
	if(isLastItem)
		return (serial + "\\|");
	else
		return (serial + "|");
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
GString Serializable::addItemToSerial(unsigned int originalSize, const GType& cItem)
{
	//GString retSerial = GType((int)cItem.getType()) + GType((int)originalSize) + cItem.c_str();
	//return retSerial;

	GString retSerial = GString((int)cItem.getType()) + GString((int)originalSize);
	retSerial +=  GString(cItem.c_str(), cItem.size());
	return retSerial;
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
GString Serializable::serializeItem(const GType& cItem, bool isLastItem)
{
	GString escapedItem = escapeSeparators(cItem);
	GString delimittedItem = addDelimiter(cItem, isLastItem);
	return addItemToSerial(cItem.size(), delimittedItem);
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
 * @param text.length() the length of the string to search
 * @param delimiter.length() the length of the delimiter to search
 * @return the index of text where delimiter appears, or -1 if it doesn't appear
 */
int Serializable::findNextDelimiterIndex(int startIdx, const GString& text, const GString& delimiter)
{
	for (unsigned int i = startIdx; i < text.length() - delimiter.length() + 1; ++i)
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
 * @param text.length() the length of the string to search
 * @param delimiter the delimiter for which to search
 * @param delimiter.length() the length of the delimiter
 * @param ecaped should we be looking for an escaped delimiter? or an unescaped delimiter?
 * @return the first index of the text where the desired delimiter appears, or -1 if it doesn't
 */
int Serializable::getDelimiterIdx(const GString& text, const GString& delimiter, bool escaped)
{
	int breakPoint = -1 * delimiter.length();
	if ((text.length() == 0) || (delimiter.length() == 0))
		return -1;

	do
	{
		breakPoint = findNextDelimiterIndex(breakPoint + delimiter.length(), text, delimiter);
	} while ((escaped != isEscaped(breakPoint, text.c_str())) && (breakPoint >= 0));

	return breakPoint;
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
GString Serializable::deserializeContent(const GString& serial)
{
	// escape all characters
	GString newSerial = serial;
	for (unsigned int j = 0; j < newSerial.length(); ++j)
	{
		if(newSerial[j] != ESC_CHAR)
			continue;

		//Cut out the escape character
		newSerial = newSerial.substr(0, j) + newSerial.substr(j+1);
	}

	return newSerial;
}

/*!
 * @brief Create a new serial from GList
 * @details Turn the GList into a serial
 * @param newList the table to serialize
 * @param serial the new serial
 * @return the length of the new serial
 */
GString Serializable::Serialize(const GList& itemizedTable, bool overrideLast)
{
	// Serialize the Itemized List (GList)
	GString retStr = "";
	for (unsigned int i = 0; i < itemizedTable.size(); ++i)
	{
		bool isLastItem = (i == itemizedTable.size() - 1) && (!overrideLast);
		retStr += serializeItem(itemizedTable[i], isLastItem);
	}

	return retStr;
}

/*!
 * @brief Create a new serial from GTable
 * @details Turn the GTable into a serial
 * @param cTable the table to serialize
 * @param serial the new serial
 * @return the length of the new serial
 */
GString Serializable::Serialize(const GTable& cTable, bool overrideLast)
{
	int rows = cTable.numberOfRows();
	int columns = cTable.numberOfCols();
	int r, c;

	// metadata at the front
	GList cList;
	cList.addInt(rows);
	cList.addInt(columns);
	cList.addChar(cTable.delimiter);
	cList.addFloat(cTable.xMin);
	cList.addFloat(cTable.xMax);
	cList.addFloat(cTable.xRange);

	// the header
	for (c = 0; c < columns; ++c)
		cList.addString(cTable.getHeader(c));

	// the output columns
	for (c = 0; c < columns; ++c)
		cList.addBoolean(cTable.isOutput(c));

	// the contents
	for (r = 0; r < rows; ++r)
	{
		for (c = 0; c < columns; ++c)
			cList.addGType(cTable.getCell(r, c));
	}

	return Serialize(cList, overrideLast);
}

/*!
 * @brief Create a new serial from GObject
 * @details Turn the GObject into a serial
 * @param cObject the table to serialize
 * @param serial the new serial
 * @return the length of the new serial
 */
GString Serializable::Serialize(const shmea::GObject& cObject, bool overrideLast)
{
	unsigned int memberTablesCount = cObject.memberTables.size();

	GList cList;
	cList.addInt(memberTablesCount);

	// Add the member table
	const GTable& members = cObject.members;
	unsigned int rows = members.numberOfRows();
	unsigned int columns = members.numberOfCols();

	// metadata at the front
	cList.addInt(rows);
	cList.addInt(columns);
	cList.addChar(members.delimiter);
	cList.addFloat(members.xMin);
	cList.addFloat(members.xMax);
	cList.addFloat(members.xRange);

	// the header
	for (unsigned int c = 0; c < columns; ++c)
		cList.addString(members.getHeader(c));

	// the output columns
	for (unsigned int c = 0; c < columns; ++c)
		cList.addBoolean(members.isOutput(c));

	// the contents
	for (unsigned int r = 0; r < rows; ++r)
	{
		for (unsigned int c = 0; c < columns; ++c)
			cList.addGType(members.getCell(r, c));
	}

	// Add the member's tables
	for(unsigned int i = 0; i < memberTablesCount; ++i)
	{
		const GTable& cTable = cObject.memberTables[i];
		unsigned int rows = cTable.numberOfRows();
		unsigned int columns = cTable.numberOfCols();

		// metadata at the front
		cList.addInt(rows);
		cList.addInt(columns);
		cList.addChar(cTable.delimiter);
		cList.addFloat(cTable.xMin);
		cList.addFloat(cTable.xMax);
		cList.addFloat(cTable.xRange);

		// the header
		for (unsigned int c = 0; c < columns; ++c)
			cList.addString(cTable.getHeader(c));

		// the output columns
		for (unsigned int c = 0; c < columns; ++c)
			cList.addBoolean(cTable.isOutput(c));

		// the contents
		for (unsigned int r = 0; r < rows; ++r)
		{
			for (unsigned int c = 0; c < columns; ++c)
				cList.addGType(cTable.getCell(r, c));
		}
	}

	return Serialize(cList, overrideLast);
}

/*!
 * @brief Create a new serial from ServiceData
 * @details Turn the ServiceData into a serial
 * @param cData the table to serialize
 * @param serial the new serial
 * @return the length of the new serial
 */
GString Serializable::Serialize(const ServiceData* cData)
{
	// Metadata at the front
	GList metaList;
	metaList.addString(cData->getSID());
	metaList.addInt(cData->getType());
	metaList.addString(cData->getCommand());

	GString metaData = Serialize(metaList, true);

	GString repData = "";
	switch(cData->getType())
	{
		case ServiceData::TYPE_NETWORK_POINTER:
		{
			// {GOBJECT}
			printf("---SS Object---\n");
			repData = Serialize(*(cData->getObj()));

			break;
		}

		case ServiceData::TYPE_TABLE:
		{
			// {GTable}
			printf("---SS Table---\n");
			repData = Serialize(*(cData->getTable()));

			break;
		}

		case ServiceData::TYPE_LIST:
		{
			// {GList}
			printf("---SS List: %d---\n", cData->getList()->size());
			repData = Serialize(*(cData->getList()));
			//cData->getList()->print();

			break;
		}

		case ServiceData::TYPE_ACK:
		default:
		{
			// Write nothing
			printf("---SS Nothing---\n");
			break;
		}
	}

			//printf("WRITE-metaData[%d]: %s\n", metaData.length(), metaData.c_str());
			/*for(unsigned int rCounter=0;rCounter<metaData.length();++rCounter)
			{
				printf("WRITE[%u]: 0x%02X:%c\n", rCounter, metaData[rCounter], metaData[rCounter]);
				if(metaData[rCounter] == 0x7C)
					printf("-------------------------------\n");
			}*/

			//printf("WRITE-repData[%d]: %s\n", repData.length(), repData.c_str());
			/*for(unsigned int rCounter=0;rCounter<repData.length();++rCounter)
			{
				printf("WRITE[%u]: 0x%02X:%c\n", rCounter, repData[rCounter], repData[rCounter]);
				if(repData[rCounter] == 0x7C)
					printf("-------------------------------\n");
			}*/


	// Combine the header and body into one string
	GString serial = metaData + repData;
	/*for(unsigned int rCounter=0;rCounter<serial.length();++rCounter)
	{
		printf("Serialize[%u]: 0x%02X:%c\n", rCounter, serial[rCounter], serial[rCounter]);
		if(serial[rCounter] == 0x7C)
			printf("-------------------------------\n");
	}*/
	return serial;
}

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
int Serializable::Deserialize(GList& retList, const GString& serial, int maxItems)
{
	if (serial.length() == 0)
		return 0;

	// copy the serial (keep the original intact)
	GString serialCopy = serial;

	int retLen = 0;
	int itemCounter = 0;
	int nextDel = 0; // delimiter
	const bool NOT_ESCAPED = false;

	do
	{
		//printf("Des-Loop0\n");
		nextDel = getDelimiterIdx(serialCopy, GString("|"), NOT_ESCAPED);
		int lastDel = getDelimiterIdx(serialCopy, GString("\\|"), NOT_ESCAPED);

		bool isLastBlock = (nextDel == lastDel+1);
		int delimiterLen = ((isLastBlock) && (lastDel > 0)) ? 2 : 1;

		// Strip the current block off
		GString cBlock = "";
		//printf("Des-Loop1: %d\n", serialCopy.size());
		if((isLastBlock) && (lastDel > 0))
		{
			//printf("Des-Loop1A\n");
			cBlock = serialCopy.substr(0, lastDel);
			serialCopy = "";
			
		}
		else
		{
			//printf("Des-Loop1B\n");
			cBlock = serialCopy.substr(0, nextDel);
			serialCopy = serialCopy.substr(nextDel+1);
		}

		retLen = serialCopy.size();
		//printf("Des-Loop2: %d\n", serialCopy.size());

		// Get the Type from the buffer
		int newType = cBlock.substr(0, sizeof(int)).getInt();
		cBlock = cBlock.substr(sizeof(int)); // plus the comma
		//printf("Des-Loop3: %d\n", newType);

		// Get the Size from the buffer
		unsigned int newSize = (unsigned int)(cBlock.substr(0, sizeof(int)).getInt());
		cBlock = cBlock.substr(sizeof(int)); // plus the comma
		//printf("Des-Loop4: %d\n", newSize);

		// Get the Body from the buffer
		GString newBlock = deserializeContent(cBlock);
		if(newBlock.length() != newSize)
			break;

		//printf("Des-Loop5: %d:%d\n", newType, newSize);
		cBlock = cBlock.substr(newSize+delimiterLen); // plus the delimiter
		retList.addObject(newType, newBlock, newSize);
		++itemCounter;

	} while ((nextDel > 0) && (maxItems>0? (itemCounter < maxItems):true));

	return retLen;
}

/*!
 * @brief GList to GTable
 * @details Creates a GTable from a bundle
 * @return the GTable version of the bundle
 */
void Serializable::Deserialize(GTable& retTable, const GString& serial)
{
	GList cList;
	Deserialize(cList, serial);

	// metadata
	int rows = cList.getInt(0), columns = cList.getInt(1);
	char delimiter = cList.getChar(2);
	float min = cList.getFloat(3), max = cList.getFloat(4), range = cList.getFloat(5);
	int bundleIndex = 6; // Index to mark the end of the input args
	int cIndex = bundleIndex;

	// the header
	std::vector<GString> header;
	for (int i = 0; i < columns; ++i)
		header.push_back(cList.getString(cIndex + i));

	// Because we cycled through the header
	cIndex += columns;

	// the output columns
	std::vector<int> outputColumns;
	for (int i = 0; i < columns; ++i)
	{
		int isOutputCol = cList.getString(cIndex + i) == GString("True");
		outputColumns.push_back(isOutputCol);
	}

	// Because we cycled through the output columns
	cIndex += columns;

	// Create the GTable schema
	GTable cTable(delimiter);
	cTable.setMin(min);
	cTable.setMax(max);
	cTable.setRange(range);
	cTable.setHeaders(header);

	for (unsigned int i = 0; i < outputColumns.size(); ++i)
	{
		if(outputColumns[i])
			cTable.toggleOutput(i);
	}

	// the contents
	for (int i = 0; i < rows; ++i)
	{
		GList row;
		for (int j = 0; j < columns; ++j)
			row.addGType(cList[cIndex + j]);

		cTable.addRow(row);

		// Because we cycled through another row
		cIndex += columns;
	}

	int actual_size = cList.size();
	int expected_size = (rows + 2) * columns + bundleIndex;
	if (expected_size != actual_size)
	{
		printf("[SER] Bad GTable: Sizes(%d != %d)\n", actual_size, expected_size);
		return;
	}

	retTable = cTable;
}

/*!
 * @brief GList to GObject
 * @details Creates a GObject from a bundle
 * @return the GObject version of the bundle
 */
void Serializable::Deserialize(GObject& retObj, const GString& serial)
{
	//TODO: REPLACE THIS BLOCK WITH GTABLE DSERIALIZE CALL
	// Add the members
	GList cList;
	Deserialize(cList, serial);
	unsigned int memberTablesCount = cList.getInt(0);
	int cIndex = 1;

	// metadata
	int rows = cList.getInt(cIndex + 0), columns = cList.getInt(cIndex + 1);
	char delimiter = cList.getChar(cIndex + 2);
	float min = cList.getFloat(cIndex + 3), max = cList.getFloat(cIndex + 4), range = cList.getFloat(cIndex + 5);
	int bundleIndex = cIndex + 6; // Index to mark the end of the input args
	cIndex = bundleIndex;

	// the header
	std::vector<GString> header;
	for (int i = 0; i < columns; ++i)
		header.push_back(cList.getString(cIndex + i));

	// Because we cycled through the header
	cIndex += columns;

	// the output columns
	std::vector<int> outputColumns;
	for (int i = 0; i < columns; ++i)
	{
		int isOutputCol = cList.getString(cIndex + i) == GString("True");
		outputColumns.push_back(isOutputCol);
	}

	// Because we cycled through the output columns
	cIndex += columns;

	// Create the GTable schema
	GTable members(delimiter);
	members.setMin(min);
	members.setMax(max);
	members.setRange(range);
	members.setHeaders(header);

	for (unsigned int i = 0; i < outputColumns.size(); ++i)
	{
		if(outputColumns[i])
			members.toggleOutput(i);
	}

	// the contents
	for (int i = 0; i < rows; ++i)
	{
		GList row;
		for (int j = 0; j < columns; ++j)
			row.addGType(cList[cIndex + j]);

		members.addRow(row);

		// Because we cycled through another row
		cIndex += columns;
	}

	int top_actual_size = cIndex;
	int top_expected_size = (rows + 2) * columns + bundleIndex;
	if (top_expected_size != top_actual_size)
	{
		printf("[SER] Bad GObject: Sizes(%d != %d)\n", top_actual_size, top_expected_size);
		return;
	}

	//TODO: REPLACE THIS BLOCK WITH GTABLE DSERIALIZE CALL IN THE LOOP
	// Create the cObject we will return from the members
	GObject cObject;
	cObject.setMembers(members);

	// Now we add the member GTables
	for(unsigned int mCounter = 0; mCounter < memberTablesCount; ++mCounter)
	{
		// metadata
		int rows = cList.getInt(cIndex + 0), columns = cList.getInt(cIndex + 1);
		char delimiter = cList.getChar(cIndex + 2);
		float min = cList.getFloat(cIndex + 3), max = cList.getFloat(cIndex + 4), range = cList.getFloat(cIndex + 5);

		// the header
		std::vector<GString> header;
		for (int i = 0; i < columns; ++i)
			header.push_back(cList.getString(cIndex + i));

		// Because we cycled through the header
		cIndex += columns;

		// the output columns
		std::vector<int> outputColumns;
		for (int i = 0; i < columns; ++i)
		{
			int isOutputCol = cList.getString(cIndex + i) == GString("True");
			outputColumns.push_back(isOutputCol);
		}

		// Because we cycled through the output columns
		cIndex += columns;

		// Create the GTable schema
		GTable cTable(delimiter);
		cTable.setMin(min);
		cTable.setMax(max);
		cTable.setRange(range);
		cTable.setHeaders(header);

		for (unsigned int i = 0; i < outputColumns.size(); ++i)
		{
			if(outputColumns[i])
				cTable.toggleOutput(i);
		}

		// the contents
		for (int i = 0; i < rows; ++i)
		{
			GList row;
			for (int j = 0; j < columns; ++j)
				row.addGType(cList[cIndex + j]);
	
			cTable.addRow(row);

			// Because we cycled through another row
			cIndex += columns;
		}

		int actual_size = cIndex;
		int expected_size = (rows + 2) * columns + top_actual_size;
		top_actual_size = expected_size; // for the next iteration of the mCounter
		if (expected_size != actual_size)
		{
			printf("[SER] Bad GObject GTable[%d]: Sizes(%d != %d)\n", mCounter, actual_size, expected_size);
			return;
		}
	}

	retObj = cObject;
}

void Serializable::Deserialize(ServiceData* retData, const GString& serial)
{
	if ((!serial) || (serial.length() == 0))
		return;

	GList metaList;
	int repLen = Deserialize(metaList, serial, 3);//we want only 3 GItems
	GString repData = serial.substr(serial.length()-repLen);//TODO: MAKE THIS LINE WORK
	/*for(unsigned int rCounter=0;rCounter<serial.length();++rCounter)
	{
		printf("Deserialize[%u]: 0x%02X:%c\n", rCounter, serial[rCounter], serial[rCounter]);
		if(serial[rCounter] == 0x7C)
			printf("-------------------------------\n");
	}*/

	// metadata
	metaList.print();
	GString sdSID = metaList.getString(0);
	retData->setSID(sdSID);

	int sdType = metaList.getInt(1);
	retData->setType(sdType);

	GString sdCommand = metaList.getString(2);
	retData->setCommand(sdCommand);

	switch(sdType)
	{
		case ServiceData::TYPE_NETWORK_POINTER:
		{
			// {GOBJECT}
			GObject cObj;
			Deserialize(cObj, repData);
			retData->setObj(new GObject(cObj));
			printf("---SD Object---\n");

			break;
		}

		case ServiceData::TYPE_TABLE:
		{
			// {GTable}
			GTable cTable;
			Deserialize(cTable, repData);
			retData->setTable(new GTable(cTable));
			printf("---SD Table---\n");

			break;
		}

		case ServiceData::TYPE_LIST:
		{
			// {GList}

			GList cList;
			Deserialize(cList, repData);
			retData->setList(new GList(cList));
			cList.print();
			printf("---SD List---\n");

			break;
		}

		case ServiceData::TYPE_ACK:
		default:
		{
			// Write nothing
			printf("---SD Nothing---\n");
			break;
		}
	}
}
