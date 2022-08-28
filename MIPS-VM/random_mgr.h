#pragma once
#include "pch.h"

class random_mgr {
public:
	random_mgr() {
		std::random_device rd;
		m_gen = std::mt19937(rd());
	}

	void set_seed(uint32_t id, uint32_t seed) {
		std::mt19937 gen(seed);
		m_generators[id] = gen;
	}

	uint32_t get_int(uint32_t id) {
		auto gen = get_gen(id);
		std::uniform_int_distribution<uint32_t> dist(0, std::numeric_limits<uint32_t>::max());
		return dist(gen);
	}

	uint32_t get_int_range(uint32_t id, uint32_t max) {
		auto gen = get_gen(id);
		std::uniform_int_distribution<uint32_t> dist(0, max);
		return dist(gen);
	}

	float get_float(uint32_t id) {
		auto gen = get_gen(id);
		std::uniform_real_distribution<float> dist(0.f, 1.f);
		return dist(gen);
	}

private:

	std::mt19937 get_gen(uint32_t id) {
		auto gen = m_generators.find(id);
		if (gen == m_generators.end()) {
			return m_gen; // this id does not have a set seed, fall back to normal random
		}
		else {
			return gen->second;
		}
	}

	std::mt19937 m_gen;
	std::unordered_map<uint32_t, std::mt19937> m_generators;
};