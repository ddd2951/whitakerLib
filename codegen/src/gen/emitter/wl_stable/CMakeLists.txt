add_library(gen_emitter_wl_stable STATIC
    emitter.cpp
    image_file.cpp
    relationship_emitter/image_builder.cpp
    relationship_emitter/image_writer.cpp
    relationship_emitter/relationship_emitter.cpp
    relationship_emitter/relationship_index.cpp
    relationship_emitter/relationship_post_validation.cpp
    relationship_emitter/string_section.cpp)
target_link_libraries(gen_emitter_wl_stable PUBLIC gen_types)
target_link_libraries(gen_emitter_wl_stable PRIVATE
    gen_policy
    gen_error
    gen_publish
    gen_runtime_relationship)
