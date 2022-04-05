// Confidential, unpublished property of Robert Carneiro

// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "GString-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GString.h"

// === This is the primary unit testing function:
// void G_assert(const char* fileName, int lineNo, const char* failureMsg, bool expr)

void GStringUnitTest()
{
	const char* str_c = "Test123!\0";
	shmea::GString str = "Test123!";
	shmea::GString str2 = "Test123!";
	shmea::GString str3 = "Test456???";
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", strcmp(str, "Test123!") == 0);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str == "Test123!");
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str == str2);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str == str_c);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str.length() == 8);

	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str != str3);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str2 != str3);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str_c != str3);

	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str < str3);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str2 < str3);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str_c < str3);

	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str <= "Test123!");
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str <= str2);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str <= str_c);
	G_assert (__FILE__, __LINE__, "==============GString Failed==============", str <= str3);

	//printf("GStr: %s\n", str.c_str());
	for(unsigned int i=0;i<str.length();++i)
	{
		//printf("GStr[%d]: 0x%02X:%c\n", i, str[i], str[i]);
		G_assert (__FILE__, __LINE__, "==============GString[i] Failed==============", str[i] == str_c[i]);
	}

	str = "abc" + str;
	G_assert (__FILE__, __LINE__, "==============GString::operator+ Failed==============", str == "abcTest123!");
	str += "?!";
	G_assert (__FILE__, __LINE__, "==============GString::operator+= Failed==============", str == "abcTest123!?!");
	str = str + "!";
	G_assert (__FILE__, __LINE__, "==============GString::operator+ Failed==============", str == "abcTest123!?!!");

	G_assert (__FILE__, __LINE__, "==============GString::substr Failed==============", str.substr(0,0).length() == 0);
	G_assert (__FILE__, __LINE__, "==============GString::substr Failed==============", str.substr(3) == "Test123!?!!");
	G_assert (__FILE__, __LINE__, "==============GString::substr Failed==============", str.substr(3, 4) == "Test");

	G_assert (__FILE__, __LINE__, "==============GString::isWhitespace Failed==============", shmea::GString::isWhitespace(' '));
	G_assert (__FILE__, __LINE__, "==============GString::isWhitespace Failed==============", shmea::GString::isWhitespace('\t'));
	G_assert (__FILE__, __LINE__, "==============GString::isWhitespace Failed==============", shmea::GString::isWhitespace('\r'));
	G_assert (__FILE__, __LINE__, "==============GString::isWhitespace Failed==============", shmea::GString::isWhitespace('\n'));

	G_assert (__FILE__, __LINE__, "==============GString::isInteger Failed==============", shmea::GString::isInteger("0"));
	G_assert (__FILE__, __LINE__, "==============GString::isInteger Failed==============", shmea::GString::isInteger("-10"));
	G_assert (__FILE__, __LINE__, "==============GString::isInteger Failed==============", shmea::GString::isInteger("11-") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isInteger Failed==============", shmea::GString::isInteger("4.0") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isInteger Failed==============", shmea::GString::isInteger("abc") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isInteger Failed==============", shmea::GString::isInteger("-5.9") == false);

	G_assert (__FILE__, __LINE__, "==============GString::isFloat Failed==============", shmea::GString::isFloat("0"));
	G_assert (__FILE__, __LINE__, "==============GString::isFloat Failed==============", shmea::GString::isFloat("-10"));
	G_assert (__FILE__, __LINE__, "==============GString::isFloat Failed==============", shmea::GString::isFloat("11-") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isFloat Failed==============", shmea::GString::isFloat("4.0"));
	G_assert (__FILE__, __LINE__, "==============GString::isFloat Failed==============", shmea::GString::isFloat("abc") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isFloat Failed==============", shmea::GString::isFloat("-5.9"));

	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper('a') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper('B'));
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper('!') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper(' ') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper('4') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("ABC"));
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("dEf") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("XyZ") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("5") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("345Ab") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("345AB") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("Ab345") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("ab345") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("AB35") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isUpper Failed==============", shmea::GString::isUpper("A5c") == false);

	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower('a'));
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower('B') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower('!') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower(' ') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower('4') == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("abc"));
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("dEf") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("XyZ") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("POP") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("5") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("345Ab") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("345AB") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("Ab345") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("ab345") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("AB35") == false);
	G_assert (__FILE__, __LINE__, "==============GString::isLower Failed==============", shmea::GString::isLower("A5c") == false);

	G_assert (__FILE__, __LINE__, "==============GString::toggleCase Failed==============", shmea::GString::toggleCase('a') == 'A');
	G_assert (__FILE__, __LINE__, "==============GString::toggleCase Failed==============", shmea::GString::toggleCase('B') == 'b');
	G_assert (__FILE__, __LINE__, "==============GString::toggleCase Failed==============", shmea::GString::toggleCase('!') == '!');
	G_assert (__FILE__, __LINE__, "==============GString::toggleCase Failed==============", shmea::GString::toggleCase(' ') == ' ');
	G_assert (__FILE__, __LINE__, "==============GString::toggleCase Failed==============", shmea::GString::toggleCase('4') == '4');

	G_assert (__FILE__, __LINE__, "==============GString::trim Failed==============", shmea::GString::trim("Abc  ") == "Abc");
	G_assert (__FILE__, __LINE__, "==============GString::trim Failed==============", shmea::GString::trim("  Abc") == "Abc");
	G_assert (__FILE__, __LINE__, "==============GString::trim Failed==============", shmea::GString::trim("  Abc  ") == "Abc");

	/*static GString charTOstring(char);
	static GString shortTOstring(short);
	static GString intTOstring(int);
	static GString longTOstring(int64_t);
	static GString floatTOstring(float);
	static GString doubleTOstring(double);
	static GString datetimeTOstring(int64_t);
	static GString dateTOstring(int64_t);
	static GString timeTOstring(int64_t);*/

	G_assert (__FILE__, __LINE__, "==============GString::floatTOstring Failed==============", shmea::GString::intTOstring(4) == "4");
	G_assert (__FILE__, __LINE__, "==============GString::floatTOstring Failed==============", shmea::GString::intTOstring(-16) == "-16");
	G_assert (__FILE__, __LINE__, "==============GString::floatTOstring Failed==============", shmea::GString::intTOstring(2.8f) == "2"); // It truncates

	G_assert (__FILE__, __LINE__, "==============GString::floatTOstring Failed==============", shmea::GString::floatTOstring(0.5) == "0.500000");
	G_assert (__FILE__, __LINE__, "==============GString::floatTOstring Failed==============", shmea::GString::floatTOstring(0.05) == "0.050000");
	G_assert (__FILE__, __LINE__, "==============GString::floatTOstring Failed==============", shmea::GString::floatTOstring(-0.0943159163) == "-0.094316"); // It rounds
}
