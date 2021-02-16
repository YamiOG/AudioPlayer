#include <stdio.h> 
#include <iostream>
#include <string>
#include <vector>
#include <sstream> 
#include <filesystem>
#include <SDL.h>
#include <stb_image.h>
#include <stb_truetype.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

using namespace std;
namespace fs = std::filesystem;

SDL_Window *window;
SDL_Renderer* renderer;
SDL_Event ev;

SDL_Point mousePos = {0, 0};

int songSelect = 0;

SDL_Rect buttons[6] = {{0, 0, 32, 32}, {32, 0, 32, 32}, {64, 0, 32, 32}, {96, 0, 32, 32}, {128, 0, 32, 32}, {160, 0, 32, 32}};
string imageLocations[] = {"quit.png", "back.png", "pause.png", "play.png", "next.png", "restart.png", "boost.png"};
SDL_Texture *textures[7];

bool running = true;
bool pause = false;

ma_device maDevice;
ma_result maResult;

int decoderCount = 4;
string fileLocations[] = {"Running in the 90's.mp3", "Undertale - Papyrus Theme Song - Bonetrousle.mp3", "Giorno's Theme Hardbass (JoRo Remix).mp3", "Bonetrousle (EAR RAPE WARNING).mp3"};
ma_decoder* maDecoders = (ma_decoder*)malloc(sizeof(ma_decoder) * decoderCount);
bool* decoderEnd = (bool*)malloc(sizeof(bool) * decoderCount);

vector<string> list;

ma_uint32 read_and_mix_pcm_frames_f32(ma_decoder* pDecoder, float* pOutputF32, ma_uint32 frameCount)
{
    float temp[4096];
    ma_uint32 tempCapInFrames = ma_countof(temp) / 2;
    ma_uint32 totalFramesRead = 0;

    while (totalFramesRead < frameCount) {
        ma_uint32 iSample;
        ma_uint32 framesReadThisIteration;
        ma_uint32 totalFramesRemaining = frameCount - totalFramesRead;
        ma_uint32 framesToReadThisIteration = tempCapInFrames;
        if (framesToReadThisIteration > totalFramesRemaining) {
            framesToReadThisIteration = totalFramesRemaining;
        }

        framesReadThisIteration = (ma_uint32)ma_decoder_read_pcm_frames(pDecoder, temp, framesToReadThisIteration);
        if (framesReadThisIteration == 0) {
            break;
        }

        /* Mix the frames together. */
        for (iSample = 0; iSample < framesReadThisIteration*2; ++iSample) {
            pOutputF32[totalFramesRead*2 + iSample] += temp[iSample];
        }

        totalFramesRead += framesReadThisIteration;

        if (framesReadThisIteration < framesToReadThisIteration) {
            break;  /* Reached EOF. */
        }
    }
    
    return totalFramesRead;
}

void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    float* pOutputF32 = (float*)pOutput;
        if(!pause){
            if (!decoderEnd[songSelect]) {
                ma_uint64 framesRead  = ma_decoder_read_pcm_frames(&maDecoders[songSelect], pOutput, frameCount);
                if (framesRead < frameCount) {
                    decoderEnd[songSelect] = true;
                }
            }
        }
    (void)pInput;
}

int GetDirectories(string path, vector<string> &list){
    if(fs::exists(fs::path(path))){
        for (auto& p : fs::directory_iterator(path)){
            list.push_back(p.path().filename().u8string());
        }
    }
    return 0;
}

SDL_Texture *GetTextureFromFile(string location){
    int req_format = STBI_rgb_alpha;
    int width, height, orig_format;
    unsigned char* data = stbi_load(location.c_str(), &width, &height, &orig_format, req_format); 
    if (data == NULL) {
        printf("ERROR:stb_image Failed to load file");
        return nullptr;
    }

    int depth = 32;
    int pitch = 4*width;

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom((void*)data, width, height, depth, pitch, SDL_PIXELFORMAT_RGBA32);

    if(surface){
        width = surface->w;
        height = surface->h;
        SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return t;
    }
    else{
        printf("ERROR:Failed to load file");
        return nullptr;
    }
    return nullptr;
}

int Setup(){
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
        return -1;
    }

    window = SDL_CreateWindow("AudioPlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 192, 32, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
    renderer = SDL_CreateRenderer(window, -1, 0);

    for(int i = 0; i < 7; i++){
        textures[i] = GetTextureFromFile("images/" + imageLocations[i]);
    }

    ma_decoder_config maDecoderConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);
    for(int i = 0; i < decoderCount; i++){
        maResult = ma_decoder_init_file(fileLocations[i].c_str(), &maDecoderConfig, &maDecoders[i]);
        maDecoders[i].pUserData = (void*)fileLocations[i].c_str(); 
        if(maResult != MA_SUCCESS){
            printf("Failed to Load Audio File");
            ma_decoder_uninit(&maDecoders[i]);
            return -1;
        }
        decoderEnd[i] = false;
    }

    ma_device_config maConfig = ma_device_config_init(ma_device_type_playback);
    maConfig.playback.format   = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    maConfig.playback.channels = 2;               // Set to 0 to use the device's native channel count.
    maConfig.sampleRate        = 48000;           // Set to 0 to use the device's native sample rate.
    maConfig.dataCallback      = DataCallback;   // This function will be called when miniaudio needs more data.
    maConfig.pUserData         = NULL;   // Can be accessed from the device object (device.pUserData).

    if (ma_device_init(NULL, &maConfig, &maDevice) != MA_SUCCESS) {
        printf("Failed to Initialize Miniaudio Device");
        return -1; 
    }

    if (ma_device_start(&maDevice) != MA_SUCCESS) {
        printf("Failed to Start Miniaudio Device");
        return -1;
    }

    return 0;
}

void EventHandler(){
    SDL_GetMouseState(&mousePos.x, &mousePos.y);
    while(SDL_PollEvent(&ev)){
        if(ev.type == SDL_QUIT){
            running = false;
        }
        if(ev.type == SDL_MOUSEBUTTONDOWN){
            if(ev.button.button == SDL_BUTTON_LEFT){
                for(int i = 0; i < 6; i++){
                    if(buttons[i].x < mousePos.x && buttons[i].x+buttons[i].w > mousePos.x && buttons[i].y < mousePos.y && buttons[i].y + buttons[i].h > mousePos.y){

                        if(i < 2){
                            SDL_SetTextureColorMod(textures[i], 100, 100, 100);
                        }
                        else if(i < 3){
                            if(pause){
                                SDL_SetTextureColorMod(textures[i+1], 100, 100, 100);
                            }
                            else{
                                SDL_SetTextureColorMod(textures[i], 100, 100, 100);
                            }
                        }
                        else{
                            SDL_SetTextureColorMod(textures[i+1], 100, 100, 100);
                        }

                        switch(i){
                            case 0:
                                running = false;
                                break;
                            case 1:
                                songSelect--;
                                if(songSelect < 0){
                                    songSelect = decoderCount-1;
                                }
                                ma_decoder_seek_to_pcm_frame(&maDecoders[songSelect], 0);
                                break;
                            case 2:
                                pause = !pause;
                                break;
                            case 3:
                                songSelect++;
                                if(songSelect >= decoderCount){
                                    songSelect = 0;
                                }
                                ma_decoder_seek_to_pcm_frame(&maDecoders[songSelect], 0);
                                break;
                            case 4:
                                ma_decoder_seek_to_pcm_frame(&maDecoders[songSelect], 0);
                                break;
                        }
                    }
                }
            }
        }
        else{
            for(int i = 0; i < 7; i++){
                SDL_SetTextureColorMod(textures[i], 255, 255, 255);
            }
        }
    }
}

void RenderHandler(){
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, textures[0], NULL, &buttons[0]);
    SDL_RenderCopy(renderer, textures[1], NULL, &buttons[1]);
    if(pause){
        SDL_RenderCopy(renderer, textures[2], NULL, &buttons[2]);
    }
    else{
        SDL_RenderCopy(renderer, textures[3], NULL, &buttons[2]);
    }
    SDL_RenderCopy(renderer, textures[4], NULL, &buttons[3]);
    SDL_RenderCopy(renderer, textures[5], NULL, &buttons[4]);
    SDL_RenderCopy(renderer, textures[6], NULL, &buttons[5]);

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]){
    Setup();


    while(running){
        EventHandler();
        RenderHandler();
    }

    ma_device_uninit(&maDevice);
    SDL_Quit();
    return 0;
}