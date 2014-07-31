#ifndef APPLICATION_H_
#define APPLICATION_H_

class Application {
public:
    Application(int argc, char *argv[], const char *title);
    int Run(void);

private:
    bool Startup(void);
    bool Done(void);
    void Update(uint32_t delta_ms);
    void Render(void);
    void Shutdown(void);
};

#endif  // APPLICATION_H_