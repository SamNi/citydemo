// Copyright [year] <Copyright Owner>
#ifndef _RESOURCEMANAGER_H_
#define _RESOURCEMANAGER_H_
#include "essentials.h"

namespace Manager {

class Resource {
public:
private:
    uint64_t unique_id;
};

bool startup();
void shutdown(void);

}; // ~namespace

#endif // ~_RESOURCEMANAGER_H_