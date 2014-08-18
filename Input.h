#ifndef _INPUT_H_
#define _INPUT_H_
#include "essentials.h"

struct Input {
    static void startup(void *window_ptr);
    static void shutdown(void);

    static bool get_key_state(int key);
    static void get_mouse_state(void);

private:
    struct Impl;
    static std::unique_ptr<Impl> mImpl;
};

#endif  // ~_INPUT_H_