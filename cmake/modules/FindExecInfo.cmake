include(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(__GNU_LIBRARY__ "features.h" GNU_LIBRARY_FOUND)

if(NOT GNU_LIBRARY_FOUND)
	find_library (LIBEXECINFO_LIBRARY NAMES execinfo)
	mark_as_advanced(LIBEXECINFO_LIBRARY)
endif()
