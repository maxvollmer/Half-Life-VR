#pragma once

#include <filesystem>

std::filesystem::path GetRelPathFor(const std::string& file);

std::filesystem::path GetPathFor(const std::string& file);
