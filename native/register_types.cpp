#include "parent_game_node.hpp"
#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
using namespace godot;
static void init(ModuleInitializationLevel level){if(level==MODULE_INITIALIZATION_LEVEL_SCENE) GDREGISTER_CLASS(ParentGameNode);}
static void uninit(ModuleInitializationLevel){}
extern "C" GDExtensionBool GDE_EXPORT parent_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,GDExtensionClassLibraryPtr library,GDExtensionInitialization *initialization){
    GDExtensionBinding::InitObject init_obj(get_proc_address,library,initialization);
    init_obj.register_initializer(init);init_obj.register_terminator(uninit);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
