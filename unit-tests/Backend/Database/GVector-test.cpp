#include "GVector-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GVector.h"
#include "../../../Backend/Database/GType.h"
#include "../../../Backend/Database/GString.h"
#include <iostream>

#define ASSERT(failmsg, predicate) \
	G_assert(__FILE__, __LINE__, failmsg, predicate)

void GVectorUnitTest()
{
	{
		shmea::GVector<int> vec;

		ASSERT("GVector::empty() failed", vec.empty());

		vec.push_back(5);
		vec.push_back(10);

		ASSERT("GVector::size() failed", vec.size() == 2);
		ASSERT("vec[0] failed", vec[0] == 5);
		ASSERT("vec[1] failed", vec[1] == 10);

		ASSERT("GVector::empty() failed", !vec.empty());

		ASSERT("vec.pop_back() failed", vec.pop_back() == 10);
		ASSERT("vec.pop_back() failed", vec.pop_back() == 5);

		ASSERT("GVector::empty() failed", vec.empty());
		ASSERT("GVector::capacity() failed", vec.capacity() == 2);
	}
	{
		shmea::GVector<int> vec(10);

		ASSERT("GVector::capacity() failed", vec.capacity() == 10);
		ASSERT("GVector::size() failed", vec.size() == 0);
		ASSERT("GVector::empty() failed", vec.empty());

		vec.push_back(5);
		vec.push_back(6);
		vec.push_back(7);
		vec.push_back(8);

		ASSERT("GVector::size() failed", vec.size() == 4);

		vec.insert(2, 69);

		ASSERT("GVector::size() failed", vec.size() == 5);
		ASSERT("vec[0] failed", vec[0] == 5);
		ASSERT("vec[1] failed", vec[1] == 6);
		ASSERT("vec[2] failed", vec[2] == 69);
		ASSERT("vec[3] failed", vec[3] == 7);
		ASSERT("vec[4] failed", vec[4] == 8);

		shmea::GVector<int> vec1 = vec;

		ASSERT("GVector::size() failed", vec1.size() == 5);
		ASSERT("vec1[0] failed", vec1[0] == 5);
		ASSERT("vec1[1] failed", vec1[1] == 6);
		ASSERT("vec1[2] failed", vec1[2] == 69);
		ASSERT("vec1[3] failed", vec1[3] == 7);
		ASSERT("vec1[4] failed", vec1[4] == 8);

		vec.clear();

		ASSERT("GVector::clear() failed", vec.empty());
		ASSERT("Copy shouldn't be cleared", !vec1.empty());
	}
	{
		using namespace shmea;

		GVector<GType> vec;
		ASSERT("GVector::capacity() failed", vec.capacity() == 0);
		ASSERT("GVector::size() failed", vec.size() == 0);
		ASSERT("GVector::empty() failed", vec.empty());

		vec.push_back(4);
		vec.push_back(4.5f);
		vec.push_back(10.6);
		vec.push_back('a');
		vec.push_back("foo");

		ASSERT("vec[0] failed", vec[0].getType() == GType::INT_TYPE);
		ASSERT("vec[0] failed", vec[0].getInt() == 4);

		ASSERT("vec[1] failed", vec[1].getType() == GType::FLOAT_TYPE);
		ASSERT("vec[1] failed", vec[1].getFloat() == 4.5f);

		ASSERT("vec[2] failed", vec[2].getType() == GType::DOUBLE_TYPE);
		ASSERT("vec[2] failed", vec[2].getDouble() == 10.6);

		ASSERT("vec[3] failed", vec[3].getType() == GType::CHAR_TYPE);
		ASSERT("vec[3] failed", vec[3].getChar() == 'a');

		ASSERT("vec[4] failed", vec[4].getType() == GType::STRING_TYPE);
		ASSERT("vec[4] failed", (GString&)vec[4] == "foo");
	}
}