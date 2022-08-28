#pragma once
#define NOMINMAX
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <thread>
#include <array>
#include <cstring>
#include <exception>

// Platform specific includes used for getch and kbhit
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <signal.h>
#include <fcntl.h>
#endif

#include "linux_conio.h"