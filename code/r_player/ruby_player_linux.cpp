#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define VIDEO_BUFFER_SIZE 4096

bool g_bQuit = false;
char g_szPlayFileName[256] = {0};
bool g_bPlayStreamUDP = false;
bool g_bPlayFile = false;
bool g_bDebug = false;

int g_iCustomWidth = 1280;
int g_iCustomHeight = 720;
int g_iCustomRefresh = 60;

void handle_sigint(int sig) {
    printf("Caught signal %d\n", sig);
    g_bQuit = true;
}

int video_player_init(SDL_Window** window, SDL_Renderer** renderer, SDL_Texture** texture, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    *window = SDL_CreateWindow("SDL2 Video Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!(*window)) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!(*renderer)) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }

    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!(*texture)) {
        printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }

    return 0;
}

void video_player_render_frame(SDL_Renderer* renderer, SDL_Texture* texture, uint8_t* frame_data, int pitch) {
    SDL_UpdateTexture(texture, NULL, frame_data, pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void video_player_play_file(const char* filename, SDL_Renderer* renderer, SDL_Texture* texture) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Failed to open file %s\n", filename);
        return;
    }

    uint8_t buffer[VIDEO_BUFFER_SIZE];
    while (!g_bQuit) {
        int read_bytes = fread(buffer, 1, VIDEO_BUFFER_SIZE, fp);
        if (read_bytes <= 0) {
            break;
        }

        video_player_render_frame(renderer, texture, buffer, VIDEO_BUFFER_SIZE / 2);
        SDL_Delay(33); // Approximately 30 FPS
    }

    fclose(fp);
}

void video_player_stream_udp(SDL_Renderer* renderer, SDL_Texture* texture) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("Failed to create UDP socket\n");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(12345); // Example UDP port

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Failed to bind UDP socket\n");
        close(sock);
        return;
    }

    uint8_t buffer[VIDEO_BUFFER_SIZE];
    while (!g_bQuit) {
        int len = recv(sock, buffer, VIDEO_BUFFER_SIZE, 0);
        if (len > 0) {
            video_player_render_frame(renderer, texture, buffer, VIDEO_BUFFER_SIZE / 2);
        }
        SDL_Delay(33); // Approximately 30 FPS
    }

    close(sock);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;

    if (argc < 2) {
        printf("Usage: %s [-f filename] [-u] [-w width] [-h height]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            g_bPlayFile = true;
            strncpy(g_szPlayFileName, argv[++i], sizeof(g_szPlayFileName) - 1);
        } else if (strcmp(argv[i], "-u") == 0) {
            g_bPlayStreamUDP = true;
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            g_iCustomWidth = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            g_iCustomHeight = atoi(argv[++i]);
        }
    }

    if (video_player_init(&window, &renderer, &texture, g_iCustomWidth, g_iCustomHeight) != 0) {
        return 1;
    }

    if (g_bPlayFile) {
        video_player_play_file(g_szPlayFileName, renderer, texture);
    } else if (g_bPlayStreamUDP) {
        video_player_stream_udp(renderer, texture);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}