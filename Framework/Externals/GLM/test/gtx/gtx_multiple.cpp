///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2014 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// Restrictions:
///		By making use of the Software for military purposes, you choose to make
///		a Bunny unhappy.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @file test/gtx/gtx_multiple.cpp
/// @date 2012-11-19 / 2014-11-25
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include <glm/gtx/multiple.hpp>

int test_higher_uint()
{
	int Error(0);

	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(0), glm::uvec4(4)), glm::uvec4(0))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(1), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(2), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(3), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(4), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(5), glm::uvec4(4)), glm::uvec4(8))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(6), glm::uvec4(4)), glm::uvec4(8))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(7), glm::uvec4(4)), glm::uvec4(8))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(8), glm::uvec4(4)), glm::uvec4(8))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::higherMultiple(glm::uvec4(9), glm::uvec4(4)), glm::uvec4(12))) ? 0 : 1;

	return Error;
}

int test_Lower_uint()
{
	int Error(0);

	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(0), glm::uvec4(4)), glm::uvec4(0))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(1), glm::uvec4(4)), glm::uvec4(0))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(2), glm::uvec4(4)), glm::uvec4(0))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(3), glm::uvec4(4)), glm::uvec4(0))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(4), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(5), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(6), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(7), glm::uvec4(4)), glm::uvec4(4))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(8), glm::uvec4(4)), glm::uvec4(8))) ? 0 : 1;
	Error += glm::all(glm::equal(glm::lowerMultiple(glm::uvec4(9), glm::uvec4(4)), glm::uvec4(8))) ? 0 : 1;

	return Error;
}

int test_higher_int()
{
	int Error(0);

	Error += glm::higherMultiple(-5, 4) == -4 ? 0 : 1;
	Error += glm::higherMultiple(-4, 4) == -4 ? 0 : 1;
	Error += glm::higherMultiple(-3, 4) == 0 ? 0 : 1;
	Error += glm::higherMultiple(-2, 4) == 0 ? 0 : 1;
	Error += glm::higherMultiple(-1, 4) == 0 ? 0 : 1;
	Error += glm::higherMultiple(0, 4) == 0 ? 0 : 1;
	Error += glm::higherMultiple(1, 4) == 4 ? 0 : 1;
	Error += glm::higherMultiple(2, 4) == 4 ? 0 : 1;
	Error += glm::higherMultiple(3, 4) == 4 ? 0 : 1;
	Error += glm::higherMultiple(4, 4) == 4 ? 0 : 1;
	Error += glm::higherMultiple(5, 4) == 8 ? 0 : 1;
	Error += glm::higherMultiple(6, 4) == 8 ? 0 : 1;
	Error += glm::higherMultiple(7, 4) == 8 ? 0 : 1;
	Error += glm::higherMultiple(8, 4) == 8 ? 0 : 1;
	Error += glm::higherMultiple(9, 4) == 12 ? 0 : 1;

	return Error;
}

int test_Lower_int()
{
	int Error(0);

	Error += glm::lowerMultiple(-5, 4) == -8 ? 0 : 1;
	Error += glm::lowerMultiple(-4, 4) == -4 ? 0 : 1;
	Error += glm::lowerMultiple(-3, 4) == -4 ? 0 : 1;
	Error += glm::lowerMultiple(-2, 4) == -4 ? 0 : 1;
	Error += glm::lowerMultiple(-1, 4) == -4 ? 0 : 1;
	Error += glm::lowerMultiple(0, 4) == 0 ? 0 : 1;
	Error += glm::lowerMultiple(1, 4) == 0 ? 0 : 1;
	Error += glm::lowerMultiple(2, 4) == 0 ? 0 : 1;
	Error += glm::lowerMultiple(3, 4) == 0 ? 0 : 1;
	Error += glm::lowerMultiple(4, 4) == 4 ? 0 : 1;
	Error += glm::lowerMultiple(5, 4) == 4 ? 0 : 1;
	Error += glm::lowerMultiple(6, 4) == 4 ? 0 : 1;
	Error += glm::lowerMultiple(7, 4) == 4 ? 0 : 1;
	Error += glm::lowerMultiple(8, 4) == 8 ? 0 : 1;
	Error += glm::lowerMultiple(9, 4) == 8 ? 0 : 1;

	return Error;
}

int test_higher_double()
{
	int Error(0);

	Error += glm::higherMultiple(-9.0, 4.0) == -8.0 ? 0 : 1;
	Error += glm::higherMultiple(-5.0, 4.0) == -4.0 ? 0 : 1;
	Error += glm::higherMultiple(-4.0, 4.0) == -4.0 ? 0 : 1;
	Error += glm::higherMultiple(-3.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::higherMultiple(-2.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::higherMultiple(-1.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::higherMultiple(0.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::higherMultiple(1.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::higherMultiple(2.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::higherMultiple(3.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::higherMultiple(4.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::higherMultiple(5.0, 4.0) == 8.0 ? 0 : 1;
	Error += glm::higherMultiple(6.0, 4.0) == 8.0 ? 0 : 1;
	Error += glm::higherMultiple(7.0, 4.0) == 8.0 ? 0 : 1;
	Error += glm::higherMultiple(8.0, 4.0) == 8.0 ? 0 : 1;
	Error += glm::higherMultiple(9.0, 4.0) == 12.0 ? 0 : 1;

	return Error;
}

int test_Lower_double()
{
	int Error(0);

	Error += glm::lowerMultiple(-5.0, 4.0) == -8.0 ? 0 : 1;
	Error += glm::lowerMultiple(-4.0, 4.0) == -4.0 ? 0 : 1;
	Error += glm::lowerMultiple(-3.0, 4.0) == -4.0 ? 0 : 1;
	Error += glm::lowerMultiple(-2.0, 4.0) == -4.0 ? 0 : 1;
	Error += glm::lowerMultiple(-1.0, 4.0) == -4.0 ? 0 : 1;
	Error += glm::lowerMultiple(0.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::lowerMultiple(1.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::lowerMultiple(2.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::lowerMultiple(3.0, 4.0) == 0.0 ? 0 : 1;
	Error += glm::lowerMultiple(4.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::lowerMultiple(5.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::lowerMultiple(6.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::lowerMultiple(7.0, 4.0) == 4.0 ? 0 : 1;
	Error += glm::lowerMultiple(8.0, 4.0) == 8.0 ? 0 : 1;
	Error += glm::lowerMultiple(9.0, 4.0) == 8.0 ? 0 : 1;

	return Error;
}

int main()
{
	int Error(0);

	Error += test_higher_int();
	Error += test_Lower_int();
	Error += test_higher_uint();
	Error += test_Lower_uint();
	Error += test_higher_double();
	Error += test_Lower_double();

	return Error;
}
