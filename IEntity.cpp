#include "IEntity.h"

// TODO(SamNi) : Quirks of C++11 mean the only reason why a .cpp file
// exists for what should be a pure abstract INTERFACE is this thing
bool operator==(const IEntity& lhs, const IEntity& rhs) { return &lhs == &rhs; }
