#include "base.h"
#include "behavior_tree.h"

struct MyAction : public BTAction {
    explicit MyAction(void)  {}
    virtual BTStatus execute(void) {
        static int n = 1;
        LOG(LOG_TRACE, "MyAction %08X with n = %d", this, n);
        return (n++ == 5) ? BT_FAILURE : BT_SUCCESS;
    }
};

int main(void) {
    {
        BTSequence sequencer;

        sequencer.m_children.push_back(BTPtr(new MyAction()));

        while (BT_SUCCESS == sequencer.execute());
    }
    return 0;
}