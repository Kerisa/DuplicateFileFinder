
#pragma once

#include <vector>
#include <set>

class Parameters;
class FileRecord;

std::vector<std::set<const FileRecord*>> CompareFile(Parameters& param);