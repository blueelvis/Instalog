#include "pch.hpp"
#include <cassert>
#include <type_traits>
#include <limits>
#include <iterator>
#include <iomanip>
#include <boost/io/ios_state.hpp>
#include "Win32Exception.hpp"
#include "RuntimeDynamicLinker.hpp"
#include "Registry.hpp"

namespace Instalog { namespace SystemFacades {

	static NtOpenKeyFunc PNtOpenKey = GetNtDll().GetProcAddress<NtOpenKeyFunc>("NtOpenKey");
	static NtCreateKeyFunc PNtCreateKey = GetNtDll().GetProcAddress<NtCreateKeyFunc>("NtCreateKey");
	static NtCloseFunc PNtClose = GetNtDll().GetProcAddress<NtCloseFunc>("NtClose");
	static NtDeleteKeyFunc PNtDeleteKey = GetNtDll().GetProcAddress<NtCloseFunc>("NtDeleteKey");
	static NtQueryKeyFunc PNtQueryKey = GetNtDll().GetProcAddress<NtQueryKeyFunc>("NtQueryKey");
	static NtEnumerateKeyFunc PNtEnumerateKey = GetNtDll().GetProcAddress<NtEnumerateKeyFunc>("NtEnumerateKey");
	static NtEnumerateValueKeyFunc PNtEnumerateValueKeyFunc = GetNtDll().GetProcAddress<NtEnumerateValueKeyFunc>("NtEnumerateValueKey");
	static NtQueryValueKeyFunc PNtQueryValueKeyFunc = GetNtDll().GetProcAddress<NtQueryValueKeyFunc>("NtQueryValueKey");

	RegistryKey::~RegistryKey()
	{
		if (hKey_ != INVALID_HANDLE_VALUE)
		{
			PNtClose(hKey_);
		}
	}

	HANDLE RegistryKey::GetHkey() const
	{
		return hKey_;
	}

	RegistryKey::RegistryKey( HANDLE hKey )
	{
		hKey_ = hKey;
	}

	RegistryKey::RegistryKey( RegistryKey && other )
		: hKey_(other.hKey_)
	{
		other.hKey_ = INVALID_HANDLE_VALUE;
	}

	RegistryKey::RegistryKey()
		: hKey_(INVALID_HANDLE_VALUE)
	{ }

	RegistryValue const RegistryKey::operator[]( std::wstring name ) const
	{
		return GetValue(std::move(name));
	}

	RegistryValue const RegistryKey::GetValue( std::wstring name ) const
	{
		UNICODE_STRING valueName(WstringToUnicodeString(name));
		std::vector<unsigned char> buff(MAX_PATH);
		NTSTATUS errorCheck;
		do 
		{
			ULONG resultLength = 0;
			errorCheck = PNtQueryValueKeyFunc(
				hKey_,
				&valueName,
				KeyValuePartialInformation,
				buff.data(),
				static_cast<ULONG>(buff.size()),
				&resultLength
				);
			if ((errorCheck == STATUS_BUFFER_TOO_SMALL || errorCheck == STATUS_BUFFER_OVERFLOW) && resultLength != 0)
			{
				buff.resize(resultLength);
			}
		} while (errorCheck == STATUS_BUFFER_TOO_SMALL || errorCheck == STATUS_BUFFER_OVERFLOW);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		auto partialInfo = reinterpret_cast<KEY_VALUE_PARTIAL_INFORMATION const*>(buff.data());
		DWORD type = partialInfo->Type;
		ULONG len = partialInfo->DataLength;
		buff.erase(buff.begin(), buff.begin() + 3*sizeof(ULONG));
		buff.resize(len);
		return RegistryValue(type, std::move(buff));
	}

	static RegistryKey RegistryKeyOpen( HANDLE hRoot, UNICODE_STRING& key, REGSAM samDesired )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = hRoot;
		attribs.ObjectName = &key;
		attribs.Attributes = OBJ_CASE_INSENSITIVE;
		attribs.SecurityDescriptor = NULL;
		attribs.SecurityQualityOfService = NULL;
		NTSTATUS errorCheck = PNtOpenKey(&hOpened, samDesired, &attribs);
		if (!NT_SUCCESS(errorCheck))
		{
			::SetLastError(errorCheck);
			hOpened = INVALID_HANDLE_VALUE;
		}
		return RegistryKey(hOpened);
	}

	static RegistryKey RegistryKeyOpen( HANDLE hRoot, std::wstring const& key, REGSAM samDesired )
	{
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		return RegistryKeyOpen(hRoot, ustrKey, samDesired);
	}

	RegistryKey RegistryKey::Open( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(0, key, samDesired);
	}

	RegistryKey RegistryKey::Open( RegistryKey const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent.GetHkey(), key, samDesired);
	}

	RegistryKey RegistryKey::Open( RegistryKey const& parent, UNICODE_STRING& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent.GetHkey(), key, samDesired);
	}

	static RegistryKey RegistryKeyCreate( HANDLE hRoot, std::wstring const& key, REGSAM samDesired, DWORD options )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = hRoot;
		attribs.ObjectName = &ustrKey;
		attribs.Attributes = OBJ_CASE_INSENSITIVE;
		attribs.SecurityDescriptor = NULL;
		attribs.SecurityQualityOfService = NULL;
		NTSTATUS errorCheck = PNtCreateKey(&hOpened, samDesired, &attribs, NULL, NULL, options, NULL);
		if (!NT_SUCCESS(errorCheck))
		{
			::SetLastError(errorCheck);
			hOpened = INVALID_HANDLE_VALUE;
		}
		return RegistryKey(hOpened);
	}

	RegistryKey RegistryKey::Create( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(0, key, samDesired, options);
	}

	RegistryKey RegistryKey::Create( RegistryKey const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(parent.GetHkey(), key, samDesired, options);
	}

	void RegistryKey::Delete()
	{
		NTSTATUS errorCheck = PNtDeleteKey(GetHkey());
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
	}

	RegistryKeySizeInformation RegistryKey::GetSizeInformation() const
	{
		auto const buffSize = 32768ul;
		unsigned char buffer[buffSize];
		auto keyFullInformation = reinterpret_cast<KEY_FULL_INFORMATION const*>(&buffer[0]);
		auto bufferPtr = reinterpret_cast<void *>(&buffer[0]);
		ULONG resultLength = 0;
		NTSTATUS errorCheck = PNtQueryKey(GetHkey(), KeyFullInformation, bufferPtr, buffSize, &resultLength);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return RegistryKeySizeInformation(
			keyFullInformation->LastWriteTime.QuadPart,
			keyFullInformation->SubKeys,
			keyFullInformation->Values
			);
	}

	std::wstring RegistryKey::GetName() const
	{
		auto const buffSize = 32768ul;
		unsigned char buffer[buffSize];
		auto keyBasicInformation = reinterpret_cast<KEY_NAME_INFORMATION const*>(&buffer[0]);
		auto bufferPtr = reinterpret_cast<void *>(&buffer[0]);
		ULONG resultLength = 0;
		NTSTATUS errorCheck = PNtQueryKey(GetHkey(), KeyNameInformation, bufferPtr, buffSize, &resultLength);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return std::wstring(keyBasicInformation->Name, keyBasicInformation->NameLength / sizeof(wchar_t));
	}

	std::vector<std::wstring> RegistryKey::EnumerateSubKeyNames() const
	{
		const auto bufferLength = 32768;
		std::vector<std::wstring> subkeys;
		NTSTATUS errorCheck;
		ULONG index = 0;
		ULONG resultLength = 0;
		unsigned char buff[bufferLength];
		auto basicInformation = reinterpret_cast<KEY_BASIC_INFORMATION const*>(buff);
		for(;;)
		{
			errorCheck = PNtEnumerateKey(
				GetHkey(),
				index++,
				KeyBasicInformation,
				buff,
				bufferLength,
				&resultLength
			);
			if (!NT_SUCCESS(errorCheck))
			{
				break;
			}
			subkeys.emplace_back(
				std::wstring(basicInformation->Name,
				             basicInformation->NameLength / sizeof(wchar_t)
							)
			);
		}
		if (errorCheck != STATUS_NO_MORE_ENTRIES)
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return std::move(subkeys);
	}

	RegistryKey& RegistryKey::operator=( RegistryKey other )
	{
		std::swap(hKey_, other.hKey_);
		return *this;
	}

	std::vector<RegistryKey> RegistryKey::EnumerateSubKeys(REGSAM samDesired /* = KEY_ALL_ACCESS */) const
	{
		std::vector<std::wstring> names(EnumerateSubKeyNames());
		std::vector<RegistryKey> result(names.size());
		std::transform(names.cbegin(), names.cend(), result.begin(), 
			[this, samDesired] (std::wstring const& name) -> RegistryKey {
			return Open(*this, name, samDesired);
		});
		return std::move(result);
	}

	bool RegistryKey::Valid() const
	{
		return hKey_ != INVALID_HANDLE_VALUE;
	}

	bool RegistryKey::Invalid() const
	{
		return !Valid();
	}

	std::vector<std::wstring> RegistryKey::EnumerateValueNames() const
	{
		std::vector<std::wstring> result;
		ULONG index = 0;
		const ULONG valueNameStructSize = 16384 * sizeof(wchar_t) +
			sizeof(KEY_VALUE_BASIC_INFORMATION);
		std::aligned_storage<valueNameStructSize,
			std::alignment_of<KEY_VALUE_BASIC_INFORMATION>::value>::type buff;
		auto basicValueInformation = reinterpret_cast<KEY_VALUE_BASIC_INFORMATION*>(&buff);
		for(;;)
		{
			ULONG resultLength;
			NTSTATUS errorCheck = PNtEnumerateValueKeyFunc(
				hKey_,
				index++,
				KeyValueBasicInformation,
				basicValueInformation,
				valueNameStructSize,
				&resultLength);
			if (NT_SUCCESS(errorCheck))
			{
				result.emplace_back(std::wstring(basicValueInformation->Name,
					basicValueInformation->NameLength / sizeof(wchar_t)));
			}
			else if (errorCheck == STATUS_NO_MORE_ENTRIES)
			{
				break;
			}
			else
			{
				Win32Exception::ThrowFromNtError(errorCheck);
			}
		}
		return std::move(result);
	}

	std::vector<RegistryValueAndData> RegistryKey::EnumerateValues() const
	{
		std::vector<RegistryValueAndData> result;
		std::vector<unsigned char> buff;
		NTSTATUS errorCheck = 0;
		for (ULONG index = 0; NT_SUCCESS(errorCheck); ++index)
		{
			ULONG elementSize = 260; // MAX_PATH
			do
			{
				buff.resize(elementSize);
				errorCheck = PNtEnumerateValueKeyFunc(
					hKey_,
					index,
					KeyValueFullInformation,
					buff.data(),
					static_cast<ULONG>(buff.size()),
					&elementSize);
			} while (errorCheck == STATUS_BUFFER_OVERFLOW || errorCheck == STATUS_BUFFER_TOO_SMALL);
			if (NT_SUCCESS(errorCheck))
			{
				result.emplace_back(RegistryValueAndData(std::move(buff)));
			}
		}
		if (errorCheck != STATUS_NO_MORE_ENTRIES)
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return std::move(result);
	}

	RegistryKeySizeInformation::RegistryKeySizeInformation( unsigned __int64 lastWriteTime, unsigned __int32 numberOfSubkeys, unsigned __int32 numberOfValues ) : lastWriteTime_(lastWriteTime)
		, numberOfSubkeys_(numberOfSubkeys)
		, numberOfValues_(numberOfValues)
	{ }

	unsigned __int32 RegistryKeySizeInformation::GetNumberOfSubkeys() const
	{
		return numberOfSubkeys_;
	}

	unsigned __int32 RegistryKeySizeInformation::GetNumberOfValues() const
	{
		return numberOfValues_;
	}

	unsigned __int64 RegistryKeySizeInformation::GetLastWriteTime() const
	{
		return lastWriteTime_;
	}

	RegistryValue::RegistryValue( DWORD type, std::vector<unsigned char> && data )
		: type_(type)
		, data_(data)
	{ }

	RegistryValue::RegistryValue( RegistryValue && other )
		: type_(other.type_)
		, data_(std::move(other.data_))
	{ }

	DWORD RegistryValue::GetType() const
	{
		return type_;
	}

	std::vector<unsigned char>::const_iterator RegistryValue::cbegin() const
	{
		return data_.cbegin();
	}

	std::vector<unsigned char>::const_iterator RegistryValue::cend() const
	{
		return data_.cend();
	}

	std::size_t RegistryValue::size() const
	{
		return data_.size();
	}


	RegistryValueAndData::RegistryValueAndData( std::vector<unsigned char> && buff )
		: innerBuffer_(buff)
	{ }

	RegistryValueAndData::RegistryValueAndData( RegistryValueAndData && other )
		: innerBuffer_(other.innerBuffer_)
	{ }

	std::wstring RegistryValueAndData::GetName() const
	{
		auto casted = Cast();
		return std::wstring(casted->Name, casted->NameLength / sizeof(wchar_t));
	}

	KEY_VALUE_FULL_INFORMATION const* RegistryValueAndData::Cast() const
	{
		return reinterpret_cast<KEY_VALUE_FULL_INFORMATION const*>(innerBuffer_.data());
	}

	std::vector<unsigned char>::const_iterator RegistryValueAndData::cbegin() const
	{
		return innerBuffer_.cbegin() + Cast()->DataOffset;
	}

	std::vector<unsigned char>::const_iterator RegistryValueAndData::cend() const
	{
		auto casted = Cast();
		return innerBuffer_.cbegin() + casted->DataOffset + casted->DataLength;
	}

	bool RegistryValueAndData::operator<( RegistryValueAndData const& rhs ) const
	{
		auto casted = Cast();
		auto rhsCasted = rhs.Cast();
		return std::lexicographical_compare(casted->Name, casted->Name + (casted->NameLength/sizeof(wchar_t)),
			                                rhsCasted->Name, rhsCasted->Name + (rhsCasted->NameLength/sizeof(wchar_t)));
	}

	DWORD RegistryValueAndData::GetType() const
	{
		return Cast()->Type;
	}

	std::size_t RegistryValueAndData::size() const
	{
		return Cast()->DataLength;
	}

	__int32 BytestreamToDword( std::vector<unsigned char>::const_iterator first, std::vector<unsigned char>::const_iterator last )
	{
		static_assert(sizeof(__int32) == sizeof(unsigned char) * 4, "This conversion assumes a 32 bit integer is 4 characters.");
		union
		{
			__int32 converted;
			unsigned char toConvert[4];
		};
		if (std::distance(first, last) < 4)
		{
			throw ErrorInvalidParameterException();
		}
		std::copy(first, first + 4, toConvert);
		return converted;
	}

	__int64 BytestreamToQword( std::vector<unsigned char>::const_iterator first, std::vector<unsigned char>::const_iterator last )
	{
		static_assert(sizeof(__int64) == sizeof(unsigned char) * 8, "This conversion assumes a 64 bit integer is 8 characters.");
		union
		{
			__int64 converted;
			unsigned char toConvert[8];
		};
		if (std::distance(first, last) < 8)
		{
			throw ErrorInvalidParameterException();
		}
		std::copy(first, first + 8, toConvert);
		return converted;
	}

}}
