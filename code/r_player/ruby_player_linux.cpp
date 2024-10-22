#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

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

    av_register_all();
    avformat_network_init();

    AVFormatContext* pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, g_szPlayFileName, NULL, NULL) != 0) {
        printf("Couldn't open file: %s\n", g_szPlayFileName);
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information\n");
        return -1;
    }

    int videoStream = -1;
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        printf("Couldn't find a video stream\n");
        return -1;
    }

    AVCodecParameters* pCodecParameters = pFormatCtx->streams[videoStream]->codecpar;
    AVCodec* pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL) {
        printf("Unsupported codec!\n");
        return -1;
    }

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pCodecParameters) < 0) {
        printf("Couldn't copy codec context\n");
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    AVFrame* pFrame = av_frame_alloc();
    AVPacket packet;

    struct SwsContext* sws_ctx = sws_getContext(
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    uint8_t* yuv_buffer = (uint8_t*)malloc(3 * pCodecCtx->width * pCodecCtx->height);
    AVFrame* pFrameYUV = av_frame_alloc();
    pFrameYUV->data[0] = yuv_buffer;
    pFrameYUV->data[1] = pFrameYUV->data[0] + pCodecCtx->width * pCodecCtx->height;
    pFrameYUV->data[2] = pFrameYUV->data[1] + (pCodecCtx->width * pCodecCtx->height) / 4;
    pFrameYUV->linesize[0] = pCodecCtx->width;
    pFrameYUV->linesize[1] = pCodecCtx->width / 2;
    pFrameYUV->linesize[2] = pCodecCtx->width / 2;

    while (!g_bQuit) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                g_bQuit = true;
            }
        }

        if (av_read_frame(pFormatCtx, &packet) < 0)
            break;

        if (packet.stream_index == videoStream) {
            if (avcodec_send_packet(pCodecCtx, &packet) == 0) {
                while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                    sws_scale(
                        sws_ctx,
                        (uint8_t const* const*)pFrame->data,
                        pFrame->linesize,
                        0,
                        pCodecCtx->height,
                        pFrameYUV->data,
                        pFrameYUV->linesize
                    );

                    video_player_render_frame(renderer, texture, yuv_buffer, pFrameYUV->linesize[0]);
                    SDL_Delay(33);
                }
            }
        }

        av_packet_unref(&packet);
    }

    sws_freeContext(sws_ctx);
    av_frame_free(&pFrameYUV);
    free(yuv_buffer);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}