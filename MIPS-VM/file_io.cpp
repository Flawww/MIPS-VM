#include "pch.h"
#include "file_io.h"


file_manager::~file_manager() {
	for (auto& it : m_open_fds) {
		fclose(it.second); // close all files we've opened
	}
}

FILE* file_manager::get_fd(int32_t handle) {
	switch (handle) {
	case 0:
		return stdin;
	case 1:
		return stdout;
	case 2:
		return stderr;
	}

	auto fd = m_open_fds.find(handle);
	if (fd == m_open_fds.end()) {
		return  (FILE*)-1; // return -1 if this fd does not correspond to file handle
	}

	return fd->second;
}

int32_t file_manager::open_file(std::string file, int32_t flags, int32_t mode) {
	std::string file_mode;
	switch (flags) {
	case 0:
	{
		file_mode = "r";
	}
	break;
	case 1:
	{
		file_mode = "w";
	}
	break;
	case 9:
	{
		file_mode = "a";
	}
	break;
	default:
		return -1;
	}

	FILE* f;
#ifdef _MSC_VER
	fopen_s(&f, file.c_str(), file_mode.c_str());
#else
	f = fopen(file.c_str(), file_mode.c_str());
#endif
	if (!f) {
		return -1;
	}

	int32_t fd = m_fd_num++;
	m_open_fds[fd] = f;

	return fd;
}

int32_t file_manager::read_file(int32_t handle, uint8_t* buf, uint32_t max_chars) {
	FILE* f = get_fd(handle);
	if (f == (FILE*)-1) {
		return -1; // invalid fd
	}

	int32_t read_bytes;
#ifdef _MSC_VER
	read_bytes = (int32_t)fread_s(buf, max_chars, 1, max_chars, f);
#else
	read_bytes = (int32_t)fread(buf, 1, max_chars, f);
#endif

	if (read_bytes != max_chars) {
		if (feof(f)) {
			return 0; // indicate EOF
		}
		else {
			return -1; // indicate ERROR
		}
	}

	return read_bytes;
}

int32_t file_manager::write_file(int32_t handle, uint8_t* buf, uint32_t max_chars) {
	FILE* f = get_fd(handle);
	if (f == (FILE*)-1) {
		return -1; // invalid fd
	}

	int32_t written = (int32_t)fwrite(buf, 1, max_chars, f);
	if (written != max_chars) {
		return -1; // indicate ERROR

	}

	return written;
}

void file_manager::close_file(int32_t handle) {
	auto it = m_open_fds.find(handle);
	if (it != m_open_fds.end()) {
		fclose(it->second); // close handle
		m_open_fds.erase(it); // erase from hashmap
	}
}