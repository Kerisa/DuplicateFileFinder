
#pragma once

#include <vector>

class Parameters;
class FileRecord;

std::vector<std::set<const FileRecord*>> CompareFile(Parameters& param);