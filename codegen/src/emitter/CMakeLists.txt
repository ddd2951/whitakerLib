add_library(gen_emitter STATIC
    emitter.cpp
    image_file.cpp
    relationship_emitter/image_builder.cpp
    relationship_emitter/image_writer.cpp
    relationship_emitter/relationship_index.cpp
    relationship_emitter/relationship_post_validation.cpp
    relationship_emitter/string_section.cpp)
target_link_libraries(gen_emitter PUBLIC gen_types)
target_link_libraries(gen_emitter PRIVATE
    gen_policy
    gen_config
    gen_error
    gen_publish
    gen_expand
    gen_runtime_relationship
    gen_word)
