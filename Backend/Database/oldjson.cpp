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
#include "oldjson.h"
#include "GType.h"

using namespace shmea;

const std::string GJson::KEY_CHARS = "[]{}\":,";

GJson::GJson(int newType, int newDepth)
{
	type = newType;
	depth = newDepth;
	key = "";
	value = "";
}

GJson::~GJson()
{
	type = -1;
	depth = 1;
	mapValues.clear();
	vecValues.clear();
	key = "";
	value = "";
}

GJson* GJson::stringToJsonHelper(std::string& input, int cDepth, GJson* container, GJson* cObject,
								 int prevMode, int keyValueToggle)
{

	std::string readBlock = "";
	int keyIndex = -1;
	int cMode = -1;

	// Get the input buffer
	// Read the block
	readBlock = input;
	// Get the next key character
	for (unsigned int i = 0; i < KEY_CHARS.length(); ++i)
	{
		int breakPoint = readBlock.find(KEY_CHARS[i]);
		if (breakPoint == -1)
			continue;

		if (keyIndex == -1)
		{
			keyIndex = breakPoint;
			cMode = i;
		}
		else
		{
			if (breakPoint < keyIndex)
			{
				keyIndex = breakPoint;
				cMode = i;
			}
		}
	}

	//+1 to trim the character
	readBlock = readBlock.substr(0, (keyIndex + 1));
	input = input.substr(readBlock.length());
	// printf("input: %s\n", input.c_str());
	// Cleanup

	// //Settings
	// if(container)
	// 	printf("container->type: %d\n", container->type);

	// Check for a number in the value field
	if ((prevMode == 5) && (cMode != 4))
	{
		if (keyValueToggle == TOGGLE_VALUE)
		{
			// Extract the number
			cObject->value = readBlock.substr(0, keyIndex);
			cObject->value = GType::trim(cObject->value);
			readBlock = readBlock.substr(keyIndex);
			input = readBlock + input;
			// printf("cObject->value 2: %s\n", cObject->value.c_str());
			// printf("readBlock 2: %s\n", readBlock.c_str());
			// printf("input 2: %s\n", input.c_str());
			return cObject;
		}
	}

	// printf("--------------------\n");
	// printf("readBlock: %s\n", readBlock.c_str());
	// printf("prevMode:cMode; %d:%d\n", prevMode, cMode);
	// printf("keyIndex: %d\n", keyIndex);
	// if(keyValueToggle > -1)
	// 	printf("keyValueToggle: %d\n", keyValueToggle);
	// printf("========================================\n");

	// Check the key character
	GJson* newObject = NULL;
	switch (cMode)
	{
	case 0: //[
	{
		//
		newObject = new GJson(VECTOR_TYPE, cDepth);
		GJson* retObject = NULL;
		while (retObject != newObject)
		{
			//
			retObject = stringToJsonHelper(input, cDepth + 1, newObject, newObject, cMode);
			if (retObject != newObject)
			{
				newObject->vecValues.push_back(retObject);
				// if(cObject)
				// 	printf("INSERTED cObjectType: %d\n", cObject->type);
				// printf("INSERTED vecObjectType: %d\n", newObject->type);
				// printf("INSERTED vecValuesSize: %lld\n", newObject->vecValues.size());
				// printf("INSERTED retObjectType: %d\n", retObject->type);
				// printf("INSERTED retObjectVecValuesSize: %lld\n", retObject->vecValues.size());
			}
		}

		// printf("FINAL vecValuesSize: %lld\n", newObject->vecValues.size());
		return newObject;

		break;
	}
	case 1: //]
	{
		//
		return cObject;

		break;
	}
	case 2: //{
	{
		//
		newObject = new GJson(MAP_TYPE, cDepth);
		GJson* retObject = NULL;
		while (retObject != newObject)
		{
			//
			retObject = stringToJsonHelper(input, cDepth + 1, newObject, newObject, cMode);
			if (retObject != newObject)
			{
				newObject->mapValues.insert(
					std::pair<std::string, GJson*>(retObject->key, retObject));
				// printf("INSERTED cObjectType: %d\n", cObject->type);
				// printf("INSERTED mapObjectType: %d\n", newObject->type);
				// printf("INSERTED mapValuesSize: %lld\n", newObject->mapValues.size());
				// printf("INSERTED retObjectType: %d\n", retObject->type);
				// printf("INSERTED retObjectMapValuesSize: %lld\n", retObject->mapValues.size());
			}
		}

		// printf("FINAL mapValuesSize: %lld\n", newObject->mapValues.size());
		return newObject;

		break;
	}
	case 3: //}
	{
		//
		return cObject;

		break;
	}
	case 4: //"
	{
		if (container)
		{
			if (container->type == PAIR_TYPE)
			{
				std::string newBlock = readBlock.substr(0, keyIndex);
				if (keyValueToggle == TOGGLE_KEY)
				{
					// Second Quote
					cObject->key = newBlock;
					return stringToJsonHelper(input, cDepth, container, cObject, cMode,
											  TOGGLE_VALUE);
				}
				else if (keyValueToggle == TOGGLE_VALUE)
				{
					if (prevMode == 4)
					{
						// Last Quote
						cObject->value = newBlock;
						cObject->value = GType::trim(newBlock);
						return cObject;
					}
				}

				//
				return stringToJsonHelper(input, cDepth, container, cObject, cMode, keyValueToggle);
			}
			else
			{
				// First Quote
				newObject = new GJson(PAIR_TYPE, cDepth);
				return stringToJsonHelper(input, cDepth + 1, newObject, newObject, cMode,
										  TOGGLE_KEY);
			}
		}

		break;
	}
	case 5: //:
	{
		return stringToJsonHelper(input, cDepth, container, cObject, cMode, TOGGLE_VALUE);

		break;
	}
	case 6: //,
	{
		return stringToJsonHelper(input, cDepth, container, cObject, cMode);

		break;
	}
	}

	if ((prevMode == -1) && (cMode > -1))
	{
		if ((input.length() > 0) && (newObject))
			return stringToJsonHelper(input, cDepth, container, newObject);
	}

	return NULL;
}

void GJson::run(std::string fname)
{
	/*FILE* fd=fopen(fname.c_str(), "r");
	printf("[JSON] %c%s\n", (fd != NULL)? '+':'-', fname.c_str());
	if(fd != NULL)
	{
		//get the file size
		fseek(fd, 0, SEEK_END);
		int64_t fSize=ftell(fd);
		fseek(fd, 0, SEEK_SET);

		std::string text="";
		GJson* head=fileToJson(fd, fSize, text, 0);

		//
		printf("Print Test\n--------------------\n");
		print(head);
		//print((*head)[1]);
		std::cout<<"\nSize: "<<head->getObjectSize()<<std::endl;
		//std::cout<<"Size: "<<(*(*head)[0])["symbol"]->getObjectSize()<<std::endl;
		std::cout<<"depth: "<<head->getCurrentDepth()<<std::endl;
		//std::cout<<"depth: "<<(*head)[1]->getCurrentDepth()<<std::endl;
		//std::cout<<"depth: "<<(*(*head)[0])["symbol"]->getCurrentDepth()<<std::endl;
		//std::cout<<"max depth: "<<(*(*head)[0])["symbol"]->getMaxDepth()<<std::endl;
		fclose(fd);
	}*/
	// printf("fname: %s\n", fname.c_str());
	GJson* head = stringToJsonHelper(fname, 0);
	printf("Print Test\n--------------------\n");
	print(head);
}

GJson* GJson::stringToJson(std::string input)
{
	if (input.length() > 0)
	{
		GJson* head = stringToJsonHelper(input, 0);
		return head;
	}
	else
		return NULL;
}
GJson* GJson::fileToJson(std::string fname)
{
	FILE* fd = fopen(fname.c_str(), "r");
	printf("[JSON] %c%s\n", (fd != NULL) ? '+' : '-', fname.c_str());
	if (fd != NULL)
	{
		// get the file size
		fseek(fd, 0, SEEK_END);
		unsigned int fSize = ftell(fd);
		fseek(fd, 0, SEEK_SET);

		std::string text = "";
		GJson* head = fileToJsonHelper(fd, fSize, text, 0);
		printf("Print Test\n--------------------\n");
		fclose(fd);
		return head;
	}
	else
		printf("Error Opening File: %s\n", fname.c_str());
	return NULL;
}
GJson* GJson::fileToJsonHelper(FILE* fd, unsigned int fSize, std::string& text, int cDepth,
							   GJson* container, GJson* cObject, int prevMode, int keyValueToggle)
{
	if (text.length() >= fSize)
		return NULL;

	// printf("========================================\n");
	std::string readBlock = "";
	unsigned int cSize = 0;
	int keyIndex = -1;
	int cMode = -1;

	// Get the input buffer
	int bufferBlockMagnitude = 1;
	bool reloop = true;
	while ((text.length() == 0) || (reloop))
	{
		fseek(fd, text.length(), SEEK_SET);
		cSize = MAX_READ_BLOCK * bufferBlockMagnitude;
		if (cSize > fSize - text.length())
			cSize = fSize;
		// printf("LOOP\t%lld:%lld:%ld\n", text.length(), fSize, cSize);

		// read the file
		char* buffer = (char*)malloc(cSize * sizeof(char));
		bzero(buffer, cSize);
		int64_t newCSize = fread(buffer, 1, cSize, fd);
		if (newCSize <= 0)
			return NULL;
		cSize = newCSize;

		// Read the block
		readBlock = buffer;
		// GType::trim(readBlock);
		cSize = readBlock.length();
		// printf("readBlock\n%s\n", readBlock.c_str());
		// printf("--------------------\n");

		// Get the next key character
		for (unsigned int i = 0; i < KEY_CHARS.length(); ++i)
		{
			int breakPoint = readBlock.find(KEY_CHARS[i]);
			if (breakPoint == -1)
				continue;

			if (keyIndex == -1)
			{
				keyIndex = breakPoint;
				cMode = i;
			}
			else
			{
				if (breakPoint < keyIndex)
				{
					keyIndex = breakPoint;
					cMode = i;
				}
			}
		}

		// need more input data to do something?
		if ((keyIndex == -1) || (cMode == -1))
		{
			++bufferBlockMagnitude;
			reloop = true;
		}
		else
		{
			//+1 to trim the character
			cSize -= readBlock.size() - (keyIndex + 1);
			readBlock = readBlock.substr(0, (keyIndex + 1));
			text += readBlock;
			reloop = false;
		}

		// Cleanup
		if (buffer)
			free(buffer);
	}

	// Settings
	// printf("text: %ld\n%s\n", text.length(), text.c_str());
	// printf("--------------------\n");
	// if(container)
	//	printf("container->type: %d\n", container->type);

	// Check for a number in the value field
	if ((prevMode == 5) && (cMode != 4))
	{
		if (keyValueToggle == TOGGLE_VALUE)
		{
			// Extract the number
			cObject->value = readBlock.substr(0, keyIndex);
			cObject->value = GType::trim(cObject->value);
			readBlock = readBlock.substr(keyIndex);
			text = text.substr(0, text.size() - 1);
			// printf("--------------------\n");
			// printf("newText: %ld\n%s\n", text.length(), text.c_str());
			// printf("--------------------\n");
			return cObject;
		}
	}

	// printf("--------------------\n");
	// printf("readBlock: %s\n", readBlock.c_str());
	// printf("prevMode:cMode; %d:%d\n", prevMode, cMode);
	// printf("keyIndex: %d\n", keyIndex);
	// if(keyValueToggle > -1)
	// 	printf("keyValueToggle: %d\n", keyValueToggle);
	// printf("========================================\n");

	// Check the key character
	GJson* newObject = NULL;
	switch (cMode)
	{
	case 0: //[
	{
		//
		newObject = new GJson(VECTOR_TYPE, cDepth);
		GJson* retObject = NULL;
		while (retObject != newObject)
		{
			//
			retObject = fileToJsonHelper(fd, fSize, text, cDepth + 1, newObject, newObject, cMode);
			if (retObject != newObject)
			{
				newObject->vecValues.push_back(retObject);
				// if(cObject)
				// 	printf("INSERTED cObjectType: %d\n", cObject->type);
				// printf("INSERTED vecObjectType: %d\n", newObject->type);
				// printf("INSERTED vecValuesSize: %lld\n", newObject->vecValues.size());
				// printf("INSERTED retObjectType: %d\n", retObject->type);
				// printf("INSERTED retObjectVecValuesSize: %lld\n", retObject->vecValues.size());
			}
		}

		// printf("FINAL vecValuesSize: %lld\n", newObject->vecValues.size());
		return newObject;

		break;
	}
	case 1: //]
	{
		//
		return cObject;

		break;
	}
	case 2: //{
	{
		//
		newObject = new GJson(MAP_TYPE, cDepth);
		GJson* retObject = NULL;
		while (retObject != newObject)
		{
			//
			retObject = fileToJsonHelper(fd, fSize, text, cDepth + 1, newObject, newObject, cMode);
			if (retObject != newObject)
			{
				newObject->mapValues.insert(
					std::pair<std::string, GJson*>(retObject->key, retObject));
				// if(cObject)
				// {
				// 	printf("INSERTED cObjectType: %d\n", cObject->type);
				// 	printf("INSERTED mapObjectType: %d\n", newObject->type);
				// 	printf("INSERTED mapValuesSize: %lld\n", newObject->mapValues.size());
				// 	printf("INSERTED retObjectType: %d\n", retObject->type);
				// 	//printf("INSERTED retObjectMapValuesSize: %lld\n",
				// retObject->mapValues.size());
				// }
			}
		}

		// printf("FINAL mapValuesSize: %lld\n", newObject->mapValues.size());
		return newObject;

		break;
	}
	case 3: //}
	{
		//
		return cObject;

		break;
	}
	case 4: //"
	{
		if (container)
		{
			if (container->type == PAIR_TYPE)
			{
				std::string newBlock = readBlock.substr(0, keyIndex);
				if (keyValueToggle == TOGGLE_KEY)
				{
					// Second Quote
					cObject->key = newBlock;
					return fileToJsonHelper(fd, fSize, text, cDepth, container, cObject, cMode,
											TOGGLE_VALUE);
				}
				else if (keyValueToggle == TOGGLE_VALUE)
				{
					if (prevMode == 4)
					{
						// Last Quote
						cObject->value = newBlock;
						cObject->value = GType::trim(newBlock);
						return cObject;
					}
				}

				//
				return fileToJsonHelper(fd, fSize, text, cDepth, container, cObject, cMode,
										keyValueToggle);
			}
			else
			{
				// First Quote
				newObject = new GJson(PAIR_TYPE, cDepth);
				return fileToJsonHelper(fd, fSize, text, cDepth + 1, newObject, newObject, cMode,
										TOGGLE_KEY);
			}
		}

		break;
	}
	case 5: //:
	{
		return fileToJsonHelper(fd, fSize, text, cDepth, container, cObject, cMode, TOGGLE_VALUE);

		break;
	}
	case 6: //,
	{
		return fileToJsonHelper(fd, fSize, text, cDepth, container, cObject, cMode);

		break;
	}
	}

	if ((prevMode == -1) && (cMode > -1))
	{
		// printf("++++++++++++++++++++\n");
		// printf("end textinfo: %ld\n%s\n", text.length(), text.c_str());
		// printf("++++++++++++++++++++\n");

		if ((text.length() < fSize) && (newObject))
			return fileToJsonHelper(fd, fSize, text, cDepth, container, newObject);
	}

	return NULL;
}

void GJson::print(GJson* cObject)
{
	if (!cObject)
		return;

	cObject->printHelper();
}

void GJson::printHelper()
{
	if (type == VECTOR_TYPE)
	{
		printf("[\n");
		for (unsigned int i = 0; i < vecValues.size(); ++i)
		{
			print(vecValues[i]);
			if (i < vecValues.size() - 1)
				printf(",\n");
		}
		printf("]");
		fflush(stdout);
	}
	else if (type == MAP_TYPE)
	{
		printf("{\n");
		unsigned int counter = 0;
		std::map<std::string, GJson*>::const_iterator itr = mapValues.begin();
		for (; itr != mapValues.end(); ++itr)
		{
			print(itr->second);
			if (counter < mapValues.size() - 1)
				printf(",\n");
			++counter;
		}
		printf("\n}");
		fflush(stdout);
	}
	else if (type == PAIR_TYPE)
	{
		printf("%s:%s", key.c_str(), value.c_str());
		fflush(stdout);
	}
}

GJson* GJson::BFS(GJson* cObject, std::string searchKey)
{
	if (!cObject)
		return NULL;

	if (searchKey == cObject->key)
	{
		if (cObject->type == VECTOR_TYPE)
			return cObject;
	}
	return NULL;
}

GJson* GJson::operator[](std::string key)
{
	if (mapValues.size() == 0)
		return NULL;
	for (std::map<std::string, GJson*>::const_iterator itr = mapValues.begin();
		 itr != mapValues.end(); ++itr)
	{
		if (itr->first == key)
		{
			return itr->second;
		}
	}
	return NULL;
}

GJson* GJson::operator[](unsigned int i)
{
	if (type == VECTOR_TYPE)
	{
		if (i >= vecValues.size())
			return NULL;
		else
			return vecValues[i];
	}
	else
		return NULL;
}

int GJson::getObjectSize() const
{
	if (type == VECTOR_TYPE)
		return vecValues.size();
	if (type == MAP_TYPE)
		return mapValues.size();
	if (type == PAIR_TYPE)
		return 1;
	else
		return -1;
}

int GJson::getCurrentDepth() const
{
	return depth;
}

int GJson::getMaxDepthHelper(GJson* cObject) const
{
	if (!cObject)
		return -1;

	return cObject->getMaxDepth();
}

int GJson::getMaxDepth() const
{
	int cDepth = getCurrentDepth();
	int tDepth = 0;

	if (type == VECTOR_TYPE)
	{
		for (unsigned int i = 0; i < vecValues.size(); ++i)
		{
			tDepth = getMaxDepthHelper(vecValues[i]);
			if (tDepth > cDepth)
			{
				cDepth = tDepth;
			}
		}
		return cDepth;
	}
	else if (type == MAP_TYPE)
	{
		for (std::map<std::string, GJson*>::const_iterator itr = mapValues.begin();
			 itr != mapValues.end(); ++itr)
		{
			tDepth = getMaxDepthHelper(itr->second);
			if (tDepth > cDepth)
			{
				cDepth = tDepth;
			}
		}
		return cDepth;
	}
	else if (type == PAIR_TYPE)
	{
		return cDepth;
	}
	return 0;
}
