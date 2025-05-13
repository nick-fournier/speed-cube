#pragma once

#include <cstdint>
#include <cstddef>

uint32_t to_epoch(const char* date_str, float time_val);
void date_from_epoch(uint32_t epoch, char* out, size_t len);
void time_from_epoch(uint32_t epoch, char* out, size_t len);
