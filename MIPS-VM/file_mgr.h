#pragma once
#include "pch.h"

class file_manager {
public:
	file_manager(): m_fd_num(3) {}
	~file_manager();

	int32_t open_file(std::string file, int32_t flags, int32_t mode);
	int32_t read_file(int32_t handle, uint8_t* buf, uint32_t max_chars);
	int32_t write_file(int32_t handle, uint8_t* buf, uint32_t max_chars);
	void close_file(int32_t handle);
private:
	FILE* get_fd(int32_t handle);

	std::unordered_map<int32_t, FILE*> m_open_fds;
	int32_t m_fd_num;
};