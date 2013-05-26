// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

template <int Pow2> class Test{
public:
	const static int64_t LoBound = ~0i64 << ((1 << Pow2) * 8 - 1);
	const static int64_t HiBound = -1i64 - LoBound;
};

int _tmain(int argc, _TCHAR* argv[])
{
	
	int64_t LoBound3 = Test<3>::LoBound;
	int64_t HiBound3 = Test<3>::HiBound;
	int LoBound2 = (int)Test<2>::LoBound;
	int HiBound2 = (int)Test<2>::HiBound;
	int LoBound1 = (int)Test<1>::LoBound;
	int HiBound1 = (int)Test<1>::HiBound;
	int LoBound0 = (int)Test<0>::LoBound;
	int HiBound0 = (int)Test<0>::HiBound;
	std::cout<<std::hex<<LoBound2<<'\n';
	std::cout<<std::hex<<HiBound2<<'\n';
	std::cout<<std::hex<<LoBound1<<'\n';
	std::cout<<std::hex<<HiBound1<<'\n';
	std::cout<<std::hex<<LoBound0<<'\n';
	std::cout<<std::hex<<HiBound0<<'\n';
	return 0;
}

