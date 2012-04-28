// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <vector>
#include <stack>
#include <boost/noncopyable.hpp>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

	/// @brief Represents a plain Win32 file handle.
	class File : boost::noncopyable
	{
		HANDLE hFile;
	public:
		/// @brief	Default constructor.
		File();

		///
		/// @param	filename		  	Filename of the file.
		/// @param	desiredAccess	  	(optional) the desired access.
		/// @param	shareMode		  	(optional) the share mode.
		/// @param	securityAttributes	(optional) the security attributes.
		/// @param	createdDisposition	(optional) the created disposition.
		/// @param	flags			  	(optional) the flags.
		///
		/// @throw	Win32Exception on failure
		File(
			std::wstring const& filename,
			DWORD desiredAccess = GENERIC_READ,
			DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,
			LPSECURITY_ATTRIBUTES securityAttributes = nullptr,
			DWORD createdDisposition = OPEN_EXISTING,
			DWORD flags = FILE_ATTRIBUTE_NORMAL
		);
		
		/// @brief	Move constructor
		///
		/// @param [in,out]	other	The other.
		File(File && other);

		/// @brief	Assignment operator.
		///
		/// @param	other	The other.
		///
		/// @return	A shallow copy of this instance.
		File& operator=(File other);

		/// @brief	Destructor.
		~File();

		/// @brief	Gets the size of the file in bytes
		///
		/// @return	The size.
		///
		/// @throw	Win32Exception on failure
		unsigned __int64 GetSize() const;

		/// @brief	Gets the attributes of the file as a DWORD
		///
		/// @return	The attributes.
		///
		/// @throw	Win32Exception on error
		DWORD GetAttributes() const;

		/// @brief	Gets the extended attributes of the file
		///
		/// @return	The extended attributes.
		///
		/// @throw	Win32Exception on error
		BY_HANDLE_FILE_INFORMATION GetExtendedAttributes() const;

		/// @brief	Reads the the specified amount of bytes into a vector 
		///
		/// @param	bytesToRead	The number of bytes to read.
		///
		/// @return	The bytes as a vector of chars.
		/// 
		/// @throw	Win32Exception on error
		std::vector<char> ReadBytes(unsigned int bytesToRead) const;

		/// @brief	Writes the specified bytes to the file
		///
		/// @param	bytes	The bytes.
		///
		/// @return	true if all of the bytes were written
		/// 
		/// @throw	Win32Exception on error
		bool WriteBytes(std::vector<char> const& bytes); 

		/// @brief	Gets the size of the specified file in bytes without opening a file handle
		///
		/// @return	The size.
		///
		/// @throw	Win32Exception on error
		static unsigned __int64 GetSize(std::wstring const& filename);

		/// @brief	Gets the attributes of the specified file as a DWORD without opening a file handle
		///
		/// @return	The attributes.
		///
		/// @throw	Win32Exception on error
		static DWORD GetAttributes(std::wstring const& filename);

		/// @brief	Gets the extended attributes of the specified file without opening a file handle
		///
		/// @return	The extended attributes.
		///
		/// @throw	Win32Exception on error
		static WIN32_FILE_ATTRIBUTE_DATA GetExtendedAttributes(std::wstring const& filename);

		/// @brief	Deletes the given file.
		///
		/// @param	filename	Filename of the file.
		/// 
		/// @throw	Win32Exception on error
		static void Delete(std::wstring const& filename);

		/// @brief	Determines if the given file exists
		///
		/// @param	filename	Filename of the file.
		///
		/// @return	true if the file exists
		static bool Exists(std::wstring const& filename);

		/// @brief	Query if a path is a directory.
		///
		/// @param	filename	Path of the file.
		///
		/// @return	true if the path is directory, false if not.
		static bool IsDirectory(std::wstring const& filename);

		/// @brief  Query if a path is a file. (That is, exists, and is not a directory)
		///
		/// @param  filename    Path of the file to check.
		///
		/// @remarks Equivalent to Exists() && !IsDirectory()
		///
		/// @return true if the path exists and is a file, false otherwise.
		static bool IsExclusiveFile(std::wstring const& fileName);

		/// @brief	Query if filename is an executable.
		///
		/// @param	filename	Filename of the file.
		///
		/// @return	true if it is an executable, false if not.
		static bool IsExecutable(std::wstring const& filename);

		/// @brief	Gets the company of the file
		///
		/// @param	target	Target file
		///
		/// @return	The company.
		/// 
		/// @throw	Win32Exception on error
		static std::wstring GetCompany(std::wstring const& target);
	};

	/// @brief	Finds files in directories.  Wrapper around FindFirstFile and FindNextFile
	class FindFiles : boost::noncopyable
	{
		std::stack<HANDLE> handles;
		const bool skipDotDirectories;
		const bool recursive;
		const bool includeRelativeSubPath;
		std::wstring rootPath;
		std::stack<const std::wstring> subPaths;
		bool valid;

	public:
		/// @summary	The data of the next file found.
		WIN32_FIND_DATAW data;

		/// @brief	Constructor
		///
		/// @param	pattern				  	A pattern specifying the files of interest.  Supports 
		/// 								everything that FindFirstFile supports (check MSDN docs 
		/// 								for this)
		/// @param	recursive			  	(optional) whether to recurse deeper into directories or
		/// 								not
		/// @param	includeRelativeSubPath	(optional) if recursive is set to true, when this is set
		/// 								to true, it will print the part of the path between the 
		/// 								root pattern path and the file.  For example, if the 
		/// 								pattern is "C:\*" and the current file is "C:\foo\bar",
		/// 								the cFileName part of the data struct will be 
		/// 								"foo\bar" when this is true and "bar" when this is 
		/// 								false.
		/// @param	skipDotDirectories	  	(optional) if this is true, the implied directories . 
		/// 								and .. will be skipped
		/// 
		/// @detail This will swallow invalid path and invalid file exceptions and instead just set
		/// 		IsValid to false.
		FindFiles(std::wstring const& pattern, bool recursive = false, bool includeRelativeSubPath = true, bool skipDotDirectories = true);

		/// @brief	Destructor.
		~FindFiles();

		/// @brief	Populates data with the next file (if there is one)
		void Next();

		/// @brief	Query if the data struct contains valid data
		///
		/// @return	true if valid, false if not.
		bool IsValid();
	};

}}
