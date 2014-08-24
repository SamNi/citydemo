// Copyright [year] <Copyright Owner>
#ifndef _RESOURCEMANAGER_H_
#define _RESOURCEMANAGER_H_
#include "essentials.h"

struct Resource {
public:
private:
    uint64_t unique_id;
};

struct ResourceManager {
    explicit ResourceManager(void);
    ~ResourceManager(void);
    bool startup();
    void shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

bool startup();
void shutdown(void);

#endif // ~_RESOURCEMANAGER_H_