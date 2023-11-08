#include "GVector-test.h"
#include "../../unit-test.h"
#include "../../../Backend/Database/GVector.h"
#include "../../../Backend/Database/GType.h"
#include "../../../Backend/Database/GString.h"
#include <iostream>
#include <vector>

void GVectorUnitTest()
{
	{
		using namespace shmea;
		GVector<int> vec;

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

		shmea::GVector<int> vec0(10);

		ASSERT("GVector::capacity() failed", vec0.capacity() == 10);
		ASSERT("GVector::size() failed", vec0.size() == 0);
		ASSERT("GVector::empty() failed", vec0.empty());

		vec0.push_back(5);
		vec0.push_back(6);
		vec0.push_back(7);
		vec0.push_back(8);

		ASSERT("GVector::size() failed", vec0.size() == 4);

		vec0.insert(2, 69);

		ASSERT("GVector::size() failed", vec0.size() == 5);
		ASSERT("vec[0] failed", vec0[0] == 5);
		ASSERT("vec[1] failed", vec0[1] == 6);
		ASSERT("vec[2] failed", vec0[2] == 69);
		ASSERT("vec[3] failed", vec0[3] == 7);
		ASSERT("vec[4] failed", vec0[4] == 8);

		GVector<int> vec1 = vec0;

		ASSERT("GVector::size() failed", vec1.size() == 5);
		ASSERT("vec1[0] failed", vec1[0] == 5);
		ASSERT("vec1[1] failed", vec1[1] == 6);
		ASSERT("vec1[2] failed", vec1[2] == 69);
		ASSERT("vec1[3] failed", vec1[3] == 7);
		ASSERT("vec1[4] failed", vec1[4] == 8);

		vec0.clear();

		ASSERT("GVector::clear() failed", vec0.empty());
		ASSERT("Copy shouldn't be cleared", !vec1.empty());

		GVector<GVector<int> > tab0;
		tab0.push_back(vec);
		tab0.push_back(vec1);
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

		GVector<GType> vec_copy = vec;

		// check if copy works properly
		ASSERT("vec_copy.size() failed", vec_copy.size() == 5);

		ASSERT("vec_copy[0] failed", vec_copy[0].getType() == GType::INT_TYPE);
		ASSERT("vec_copy[0] failed", vec_copy[0].getInt() == 4);
		ASSERT("vec_copy[1] failed", vec_copy[1].getType() == GType::FLOAT_TYPE);
		ASSERT("vec_copy[1] failed", vec_copy[1].getFloat() == 4.5f);
		ASSERT("vec_copy[2] failed", vec_copy[2].getType() == GType::DOUBLE_TYPE);
		ASSERT("vec_copy[2] failed", vec_copy[2].getDouble() == 10.6);
		ASSERT("vec_copy[3] failed", vec_copy[3].getType() == GType::CHAR_TYPE);
		ASSERT("vec_copy[3] failed", vec_copy[3].getChar() == 'a');
		ASSERT("vec_copy[4] failed", vec_copy[4].getType() == GType::STRING_TYPE);
		ASSERT("vec_copy[4] failed", (GString&)vec_copy[4] == "foo");

		// check if copy accidentally mutates original
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

		vec.clear();

		// Checks if vec_copy is intact
		ASSERT("vec_copy[0] failed", vec_copy[0].getType() == GType::INT_TYPE);
		ASSERT("vec_copy[0] failed", vec_copy[0].getInt() == 4);
		ASSERT("vec_copy[1] failed", vec_copy[1].getType() == GType::FLOAT_TYPE);
		ASSERT("vec_copy[1] failed", vec_copy[1].getFloat() == 4.5f);
		ASSERT("vec_copy[2] failed", vec_copy[2].getType() == GType::DOUBLE_TYPE);
		ASSERT("vec_copy[2] failed", vec_copy[2].getDouble() == 10.6);
		ASSERT("vec_copy[3] failed", vec_copy[3].getType() == GType::CHAR_TYPE);
		ASSERT("vec_copy[3] failed", vec_copy[3].getChar() == 'a');
		ASSERT("vec_copy[4] failed", vec_copy[4].getType() == GType::STRING_TYPE);
		ASSERT("vec_copy[4] failed", (GString&)vec_copy[4] == "foo");

		GVector<GType> vec1;
		vec.push_back(8.5f);
		vec.push_back(7);
		vec.push_back('h');
		vec.push_back("bar");
		vec.push_back(16.6);

		/* std::vector<GVector<GType> > tab; */
		GVector<GVector<GType> > tab;

		tab.push_back(vec_copy);
		tab.push_back(vec1);

		ASSERT("tab[0][0] failed", tab[0][0].getType() == GType::INT_TYPE);
		ASSERT("tab[0][0] failed", tab[0][0].getInt() == 4);
		ASSERT("tab[0][1] failed", tab[0][1].getType() == GType::FLOAT_TYPE);
		ASSERT("tab[0][1] failed", tab[0][1].getFloat() == 4.5f);
		ASSERT("tab[0][2] failed", tab[0][2].getType() == GType::DOUBLE_TYPE);
		ASSERT("tab[0][2] failed", tab[0][2].getDouble() == 10.6);
		ASSERT("tab[0][3] failed", tab[0][3].getType() == GType::CHAR_TYPE);
		ASSERT("tab[0][3] failed", tab[0][3].getChar() == 'a');
		ASSERT("tab[0][4] failed", tab[0][4].getType() == GType::STRING_TYPE);
	}
}
