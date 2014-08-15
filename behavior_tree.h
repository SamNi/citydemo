#ifndef _BEHAVIOR_TREE
#define _BEHAVIOR_TREE
#include "base.h"

enum BTStatus {
    BT_FAILURE = 0,
    BT_SUCCESS,
    BT_RUNNING,
};

// the leaves
struct BTTask {
    virtual BTStatus execute(void) = 0;
};

struct BTAction : public BTTask {
    virtual BTStatus execute(void) = 0;
};

struct BTCondition : public BTTask {
    virtual BTStatus execute(void) = 0;
};

// the composites
struct BTComposite : public BTTask {
    virtual BTStatus execute(void) = 0;
    std::list<std::shared_ptr<BTTask>> m_children;
};

struct BTSelector : public BTComposite {
    // Returns immediately with a success status code
    // when one of its sub-tasks runs successfully
    // Fails on exhaustion, more tolerant
    virtual BTStatus execute(void);
};

struct BTSequence : public BTComposite {
    // Traverses every sub-task and returns false
    // on the first failing sub-task, true on
    // getting to the end without failure
    // Fails on first 'bad' child, more strict
    virtual BTStatus execute(void);
};

typedef std::shared_ptr<BTTask> BTPtr;

#endif  // ~_BEHAVIOR_TREE