set(SDL_STATIC FALSE)
add_subdirectory(SDL2)
add_library(SDL2::SDL2 ALIAS SDL2)

add_subdirectory(freetype)
add_library(Freetype::Freetype ALIAS freetype)

add_subdirectory(SDL2_ttf)

add_subdirectory(ghc_filesystem)
