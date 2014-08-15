#include "behavior_tree.h"

BTStatus BTSelector::execute(void) {
    LOG(LOG_TRACE, "BTSelector %08X executing children", this);
    for (auto child : m_children) {
        LOG(LOG_TRACE, "BTSelector %08X executing child %08X", this, child.get());
        if (BT_SUCCESS == child->execute()) {
            LOG(LOG_TRACE, "Child %08X was successful, so returning", child.get());
            return BT_SUCCESS;
        }
    }
    LOG(LOG_TRACE, "No more children, thus failure. (BTSelector %08X)\n", this);
    return BT_FAILURE;
}

BTStatus BTSequence::execute(void) {
    LOG(LOG_TRACE, "BTSequence %08X executing children", this);
    for (auto child : m_children) {
        LOG(LOG_TRACE, "BTSequence %08X executing child %08X", this, child.get());
        if (BT_FAILURE == child->execute()) {
            LOG(LOG_TRACE, "Child %08X failed, so returning", child.get());
            return BT_FAILURE;
        }
    }
    LOG(LOG_TRACE, "No more children, thus success . (BTSequence %08X)\n", this);
    return BT_SUCCESS;
}