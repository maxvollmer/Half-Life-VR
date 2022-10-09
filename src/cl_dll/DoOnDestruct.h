#pragma once

#include <functional>

// Does the thing on destruct - Max Vollmer, 2021-03-09
class DoOnDestruct
{
public:
	DoOnDestruct(std::function<void()> dothething) : dothething{ dothething } {}
	~DoOnDestruct() { dothething(); }
private:
	std::function<void()> dothething;
};
