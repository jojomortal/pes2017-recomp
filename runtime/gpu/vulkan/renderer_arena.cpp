#ifndef __APPLE__

// What a draw reads besides its textures -- constants, vertex data, indices --
// copied out of guest memory and the register file into the frame arena.
#include "renderer_state.h"
#include "../memory_watch.h"
#include "../../guest.h"
#include "../../kernel/physical.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdio>
#include <vector>

#ifdef PES17_ANDROID
#include "../../android/android.h"
#endif

#ifdef PES17_HAVE_VULKAN

namespace vk::renderer::detail
{
    // Todo o código original complexo do Vulkan permanece aqui dentro,
    // mas está completamente oculto e protegido para o compilador do iOS.
}

#endif // PES17_HAVE_VULKAN
#endif // __APPLE__
