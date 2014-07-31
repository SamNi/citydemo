#include "./Frontend.h"
#include "./Backend.h"
// after some thought, I have decided to give the blessing of gl calls to the front end as well
// front end responsibility: upload textures, VBOs, give intermediate geometric forms
// to the backend. cache things maybe.
// idea: Visibility deltas. Both the front end and back end are stateful
// only have relevant changes propagate from frontend to backend.
#include <unordered_set>
#include "essentials.h"
#include "./Frontend.h"
#include "GL.h"

namespace Frontend {

struct Pimpl;
std::unique_ptr<Pimpl> pFrontend = nullptr;

struct Pimpl {
    bool Startup(int w, int h) {
        if (!Backend::Startup(w, h)) {
            LOG(LOG_CRITICAL, "Backend::Startup returned false\n");
            return false;
        }

        modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &nVidmodes);
        for (int i = 0;i < nVidmodes;++i) {
            LOG(LOG_INFORMATION, "%d %d at %dHz (R%dG%dB%d)",
                modes[i].width,
                modes[i].height,
                modes[i].refreshRate,
                modes[i].redBits,
                modes[i].greenBits,
                modes[i].blueBits);
        }

        return true;
    }
    void Shutdown(void) {
        // free up any of our own resources
        // ...


        Backend::Shutdown();
    }
    void Render(void) {
        Backend::BeginFrame();
        Backend::DrawFullscreenQuad();
        Backend::EndFrame(); // With this call, the renderer sets off to do its thing
    }

    const GLFWvidmode *modes;
    int nVidmodes;
};

bool Startup(int w, int h) {
    assert(nullptr == pFrontend);
    pFrontend = std::unique_ptr<Pimpl>(new Pimpl());
    return pFrontend->Startup(w, h);
}

void Shutdown(void) {
    assert(pFrontend);
    pFrontend->Shutdown();
    pFrontend.reset(nullptr);
}
void Render(void) {
    assert(pFrontend);
    pFrontend->Render();
}


}  // namespace Frontend