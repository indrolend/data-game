#include <cstdio>

#include "DesktopPlaytestPolicy.hpp"
#include "DesktopPersistencePolicy.hpp"

int main(){
    constexpr DesktopPlaytestPolicy normal{};
    constexpr DesktopPlaytestPolicy automated{true};
    constexpr DesktopPersistencePolicyInputs ordinary{};
    constexpr DesktopPersistencePolicyInputs capture{false,false,true};
    constexpr DesktopPersistencePolicyInputs roomSmoke{false,false,false,false,false,false,true};
    constexpr DesktopPersistencePolicyInputs multiplayerFixture{false,false,false,false,false,false,false,true};
    constexpr DesktopPersistencePolicyInputs renderStress{false,false,false,false,false,false,false,false,true};
    constexpr DesktopPersistencePolicyInputs crowdStress{false,false,false,false,false,false,false,false,false,true};
    const bool ok=
        normal.clearInputOnFocusChange(false)&&normal.clearInputOnFocusChange(true)&&normal.releaseCaptureOnFocusLoss()&&normal.acceptsRelativeMouseLook()&&normal.allowsNetworkMode(true)&&
        automated.clearInputOnFocusChange(false)&&!automated.clearInputOnFocusChange(true)&&!automated.releaseCaptureOnFocusLoss()&&!automated.acceptsRelativeMouseLook()&&
        automated.allowsNetworkMode(false)&&!automated.allowsNetworkMode(true)&&
        !isolatesPersistentSave(ordinary)&&isolatesPersistentSave(capture)&&isolatesPersistentSave(roomSmoke)&&
        isolatesPersistentSave(multiplayerFixture)&&isolatesPersistentSave(renderStress)&&isolatesPersistentSave(crowdStress);
    if(!ok){std::fprintf(stderr,"DESKTOP_PLAYTEST_POLICY_FAILED\n");return 1;}
    std::printf("DESKTOP_PLAYTEST_POLICY_OK normal_focus_pause=ON automation_focus_pause=OFF input_clearing=ON automation_mouse_look=OFF automation_network=REFUSED diagnostics_persistence=ISOLATED\n");
    return 0;
}
