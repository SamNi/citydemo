#ifndef APPLICATION_H_
#define APPLICATION_H_

class Application {
public:
    Application(int argc, char *argv[], const char *title);
    int Run(void);

private:
    bool startup(void);
    bool Done(void);
    void Update(uint32_t delta_ms);
    void render(void);
    void shutdown(void);
};

#endif  // APPLICATION_H_