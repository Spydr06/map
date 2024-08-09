#pragma once

#include "map.hpp"

#include <fstream>
#include <memory>

void load_preprocessed_data(std::ifstream& input, std::shared_ptr<Map> map);
auto preprocess_data(const char* xml_path, const char* data_path, std::shared_ptr<Map> map) -> int;
