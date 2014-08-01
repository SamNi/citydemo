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
#include "./GL.h"
#include "IRenderer.h"
#include "IEntity.h"
#include "ParticleSystem.h"
#include <typeinfo>

namespace Frontend {

struct Impl;
std::unique_ptr<Impl> pFrontend = nullptr;

struct Impl :  public IRenderer {
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
    
    virtual void visit(const IEntity* ent) const {
        /*
        Backend::BeginFrame();
        Backend::DrawFullscreenQuad();
        Backend::EndFrame(); // With this call, the backend sets off to do its thing
        */
        //LOG(LOG_CRITICAL, "you failed");
        auto the_real_type = typeid(*ent).name();
    }

    virtual void visit(const PSystem* ps) const {
        //LOG(LOG_TRACE, "visiting particle system %d", (int)&ps);
        LOG(LOG_TRACE, "derived");
    }
    virtual void visit(const Particle* p) const {
        LOG(LOG_TRACE, "derived2");
    }

    const GLFWvidmode *modes;
    int nVidmodes;
};

bool Startup(int w, int h) {
    assert(nullptr == pFrontend);
    pFrontend = std::unique_ptr<Impl>(new Impl());
    return pFrontend->Startup(w, h);
}

void Shutdown(void) {
    assert(pFrontend);
    pFrontend->Shutdown();
    pFrontend.reset(nullptr);
}

const IRenderer* getRenderer(void) {
    return pFrontend.get();
}

}  // namespace Frontend