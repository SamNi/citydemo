#include "state_machine.h"
#include <cstring>
#include <cassert>
#include <list>
#include <algorithm>
#include <map>

struct Transition {
    Transition(void) : src(nullptr), dest(nullptr), evt(nullptr) { }
    Transition(const char *s, const char *d, const char *e) : src(s), dest(d), evt(e) { }
    const char *src, *dest, *evt;
};

struct Compiled_State_Machine {
    explicit Compiled_State_Machine(const std::list<const char *>& states, 
        const std::list<const char *>& events, 
        const std::list<Transition>& transitions, const char *initial) : m_is_valid(false) {
        m_num_states = (uint16_t)states.size();
        m_num_events = (uint16_t)events.size();
        auto n = m_num_states*m_num_events;
        m_table = new Entry[n];
        memset(m_table, NO_TRANSITION, n*sizeof(Entry));

        Entry state_counter = 0;
        Entry event_counter = 0;
        for (auto s : states) {
            //m_state_index_map.emplace(s, state_counter);
            m_state_index_map[s] = state_counter;
            ++state_counter;
        }
        for (auto e : events) {
            //m_event_index_map.emplace(e, event_counter);
            m_event_index_map[e] = event_counter;
            ++event_counter;
        }
        for (auto t : transitions) {
            auto src_idx = m_state_index_map[t.src];
            auto dest_idx = m_state_index_map[t.dest];
            auto evt_idx = m_event_index_map[t.evt];
            if (0 == strcmp(initial, t.src))
                m_initial_state = src_idx;
            else if (0 == strcmp(initial, t.dest))
                m_initial_state = dest_idx;
            m_table[ addr(src_idx, evt_idx) ] = dest_idx;
        }
        m_current_state = m_initial_state;

        assert((0 <= m_initial_state) && (m_initial_state < m_num_states));
        m_is_valid = true;
    }
    ~Compiled_State_Machine(void) {
        delete[] m_table;
    }
    inline int16_t addr(int16_t state, int16_t evt) {
        assert( !(state >= m_num_states) || (evt >= m_num_events) || (state < 0) || (evt < 0));
        return m_num_events*state + evt;
    }
    bool validate(void) const { return m_is_valid; }
    void receive(Entry evt) {
        Entry new_state;
        new_state = m_table[addr(m_current_state, evt)];
        if (new_state != NO_TRANSITION) {
            m_current_state = new_state;
        }
        printf("%d\n", m_current_state);
    }

    static const Entry NO_TRANSITION = UINT16_MAX;
    Entry *m_table;
    uint16_t m_num_states;
    uint16_t m_num_events;
    bool m_is_valid;
    Entry m_initial_state;
    Entry m_current_state;

    EntryMap m_state_index_map;
    EntryMap m_event_index_map;
};

struct state_machine::Impl {
    explicit Impl() : m_initial_state(nullptr), m_dirty(true), m_csm(nullptr) {
    }
    ~Impl(void) {
        m_csm.reset(nullptr);
    }
    void add_state(const char *state_name) {
        m_states.push_back(state_name);
        m_dirty = true;
    }
    void remove_state(const char *state_name) {
        state_name;
        assert(0);
    }
    void add_event(const char *event_name) {
        m_events.push_back(event_name);
        m_dirty = true;
    }
    void remove_event(const char *event_name) {
        event_name;
        assert(0);
    }
    void add_transition(const char *source, const char *destination, const char *evt) {
        m_transitions.push_back(Transition(source, destination, evt));
        m_dirty = true;
    }
    void set_initial(const char *state_name) {
        m_initial_state = state_name;
        m_dirty = true;
    }
    bool validate(void) const {
        if (std::find(m_states.begin(), m_states.end(), m_initial_state) == m_states.end())
            return false;
        for (auto t : m_transitions) {
            auto b = std::find(m_states.begin(), m_states.end(), t.src);
            auto c = std::find(m_states.begin(), m_states.end(), t.dest);
            auto d = std::find(m_events.begin(), m_events.end(), t.evt);

            if (m_states.end() == std::find(m_states.begin(), m_states.end(), m_initial_state))
                return false;
            if (m_states.end() == std::find(m_states.begin(), m_states.end(), t.src))
                return false;
            if (m_states.end() == std::find(m_states.begin(), m_states.end(), t.dest))
                return false;
            if (m_events.end() == std::find(m_events.begin(), m_events.end(), t.evt))
                return false;
        }
        return true;
    };
    bool compile() {
        if (false == validate())
            return false;
        Compiled_State_Machine *new_csm = new Compiled_State_Machine(m_states, m_events, m_transitions, m_initial_state);
        if (false == new_csm->validate()) {
            delete[] new_csm;
            return false;
        }
        m_dirty = false;
        m_csm.reset(new_csm);
        return true;
    }
    void receive(Entry evt) {
        evt;
    }
    void receive(const char *evt) {
        m_csm->receive( m_csm->m_event_index_map[evt] );
    }
    std::list<const char *> m_states;
    std::list<const char *> m_events;
    std::list<Transition> m_transitions;
    const char *m_initial_state;

    typedef std::unique_ptr<Compiled_State_Machine> CSMPtr;
    CSMPtr m_csm;
    bool m_dirty;
};

state_machine::state_machine(void) : m_impl(ImplPtr(new Impl())) { }
state_machine::~state_machine(void) { m_impl.reset(nullptr); }
void state_machine::add_state(const char *state_name) { m_impl->add_state(state_name); }
void state_machine::remove_state(const char *state_name) { m_impl->remove_state(state_name); }
void state_machine::add_event(const char *event_name) { m_impl->add_event(event_name); }
void state_machine::remove_event(const char *event_name) { m_impl->remove_event(event_name); }
void state_machine::add_transition(const char *src, const char *dest, const char *evt) { m_impl->add_transition(src, dest, evt); }
void state_machine::set_initial(const char *state_name) { m_impl->set_initial(state_name); }
bool state_machine::validate(void) const { return m_impl->validate(); }
bool state_machine::compile(void) { return m_impl->compile(); }
void state_machine::receive(Entry evt) { m_impl->receive(evt); }
void state_machine::receive(const char *evt) { m_impl->receive(evt); }