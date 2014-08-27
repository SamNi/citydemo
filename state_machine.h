#ifndef _STATE_MACHINE
#define _STATE_MACHINE
#include <cstdint>
#include <memory>
#include <map>

typedef uint16_t Entry;
typedef std::map<const char *, Entry> EntryMap;

struct state_machine {
    explicit state_machine(void);
    ~state_machine(void);

    void add_state(const char *state_name);
    void remove_state(const char *state_name);

    void add_event(const char *event_name);
    void remove_event(const char *event_name);

    void add_transition(const char *source_state, const char *destination_state, const char *event_name);

    void set_initial(const char *state_name);

    bool validate(void) const;
    bool compile(void);
    void receive(Entry evt);
    void receive(const char *evt);

private:
    struct Impl;
    typedef std::unique_ptr<Impl> ImplPtr;
    ImplPtr m_impl;
};


#endif  //  ~_STATE_MACHINE