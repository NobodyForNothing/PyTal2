#pragma once
#include "Tier1.hpp"

#include "Offsets.hpp"

namespace Tier1
{
	struct ConCommandArgs {
		const void* Ptr;

		ConCommandArgs(const void* ptr) : Ptr(ptr) {
		}
		int Count() const {
			return ((const CCommand*)Ptr)->m_nArgc;
		}
		const char* At(int index) const {
			return ((const CCommand*)Ptr)->m_ppArgv[index];
		}
		const char* FullArgs() const {
			return ((const CCommand*)Ptr)->m_pArgSBuffer;
		}
	};
}