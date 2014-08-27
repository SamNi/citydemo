#include <gtest/gtest.h>
#include "../state_machine.h"

TEST(sm, validity_empty_state_machine) {
    state_machine sm;

    ASSERT_FALSE(sm.validate());
};

TEST(sm, validity_empty_state_machine_with_initial_state) {
    state_machine sm;

    sm.add_state("asdf");
    sm.set_initial("asdf");

    ASSERT_TRUE(sm.validate());
}

TEST(sm, validity_bad_initial_state) {
    state_machine sm;

    sm.set_initial("asdf");
    sm.add_state("dkgjkljgklqejkglqeg");
    ASSERT_FALSE(sm.validate());
}

TEST(sm, validity_of_wakey_wakey_scenario) {
    state_machine sm;

    sm.add_state("asleep");
    sm.add_state("awake");
    sm.add_event("good_morning");
    sm.add_event("good_night");

    sm.add_transition("asleep", "awake", "good_morning");
    sm.add_transition("awake", "asleep", "good_night");

    sm.set_initial("asleep");
    ASSERT_TRUE(sm.validate());
}

TEST(sm, toggle_switch) {
    state_machine sm;

    sm.add_state("on");
    sm.add_state("off");
    sm.add_transition("on", "off", "flick");
    sm.add_transition("off", "on", "flick");
    sm.set_initial("off");

    ASSERT_FALSE(sm.compile());
    sm.add_event("flick");
    ASSERT_TRUE(sm.compile());
}