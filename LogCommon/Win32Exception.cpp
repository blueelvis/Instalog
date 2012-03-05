//         Copyright Billy Robert O'Neal III 2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <memory>
#include "Win32Exception.hpp"

namespace Instalog { namespace SystemFacades {

	void WindowsApiException::Throw(DWORD lastError)
	{
		switch(lastError)
		{
		case ERROR_SUCCESS: throw ErrorSuccessException();
		case ERROR_FILE_NOT_FOUND: throw ErrorFileNotFoundException();
		case ERROR_PATH_NOT_FOUND: throw ErrorPathNotFoundException();
		case ERROR_ACCESS_DENIED: throw ErrorAccessDeniedException();
		case ERROR_ALREADY_EXISTS: throw ErrorAlreadyExistsException();
		case ERROR_INVALID_PARAMETER: throw ErrorInvalidParameterException();
		default: throw GenericException(lastError);
		}
	}

	struct LocalFreeHelper
	{
		void operator()(void * toFree)
		{
			::LocalFree(reinterpret_cast<HLOCAL>(toFree));
		};
	};

	std::wstring WindowsApiException::GetWideMessage() const
	{
		std::unique_ptr<void, LocalFreeHelper> buff;
		LPWSTR buffPtr;
		DWORD bufferLength = ::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetErrorCode(),
			0,
			reinterpret_cast<LPWSTR>(&buffPtr),
			0,
			NULL);
		buff.reset(buffPtr);
		return std::wstring(buffPtr, bufferLength);
	}

	std::string WindowsApiException::GetCharMessage() const
	{
		std::unique_ptr<void, LocalFreeHelper> buff;
		LPSTR buffPtr;
		DWORD bufferLength = ::FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetErrorCode(),
			0,
			reinterpret_cast<LPSTR>(&buffPtr),
			0,
			NULL);
		buff.reset(buffPtr);
		return std::string(buffPtr, bufferLength);
	}

}} // namespace Instalog::SystemFacades