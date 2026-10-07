set(WHITAKER_WARNINGS
    -Wall -Wextra -Wpedantic
    -Wshadow -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-equal
    -Wformat=2 -Wnull-dereference -Wredundant-decls -Wundef
    -Wunused-macros -Wmissing-declarations
    -Wconversion -Wsign-conversion)
set(WHITAKER_WARNINGS_CXX
    -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Wsuggest-override
    -Wextra-semi -Wzero-as-null-pointer-constant)
set(WHITAKER_WARNINGS_C -Wstrict-prototypes -Wmissing-prototypes)
set(WHITAKER_WARNINGS_GNU
    -Wduplicated-cond -Wduplicated-branches -Wlogical-op
    -Wimplicit-fallthrough=5 -Wunused-const-variable=2 -Warith-conversion)
set(WHITAKER_WARNINGS_CLANG -Wimplicit-fallthrough -Wunused-const-variable)
set(WHITAKER_WARNINGS_GNU_CXX -Wuseless-cast)
