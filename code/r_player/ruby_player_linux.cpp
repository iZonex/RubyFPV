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
#include <string.h>

// Include FFmpeg libraries
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

// Define constants
#define VIDEO_BUFFER_SIZE 4096

// Global variables
bool g_bQuit = false;
char g_szPlayFileName[256] = {0};
bool g_bPlayStreamUDP = false;
bool g_bPlayFile = false;
bool g_bDebug = false;

int g_iCustomWidth = 1280;
int g_iCustomHeight = 720;
int g_iCustomRefresh = 60;
int g_iUDPPort = 12345; // Default UDP port

// SDL Renderer and Texture
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

// Signal handler to gracefully exit
void handle_sigint(int sig) {
    printf("Caught signal %d\n", sig);
    g_bQuit = true;
}

// Function to render a video frame using SDL2
void video_player_render_frame(SDL_Renderer* renderer, SDL_Texture* texture, uint8_t* frame_data, int pitch) {
    // Update SDL texture with the new frame data
    SDL_UpdateYUVTexture(texture, NULL,
                         frame_data, pitch,                             // Y plane
                         frame_data + pitch * g_iCustomHeight, pitch / 2, // U plane
                         frame_data + pitch * g_iCustomHeight + (pitch / 2) * (g_iCustomHeight / 2), pitch / 2); // V plane

    // Clear the renderer, copy the texture, and present the frame
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

// Function to initialize SDL2 components
int video_player_init(SDL_Window** window, SDL_Renderer** renderer, SDL_Texture** texture, int width, int height) {
    // Initialize SDL2 video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    // Create an SDL window
    *window = SDL_CreateWindow("SDL2 Video Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!(*window)) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create an SDL renderer
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!(*renderer)) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return -1;
    }

    // Create a YV12 texture for rendering video frames
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

// Function to play video from a file using FFmpeg and SDL2
void video_player_play_file(const char* filename, SDL_Renderer* renderer, SDL_Texture* texture) {
    AVFormatContext* pFormatCtx = nullptr;

    // Open the video file
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        printf("Couldn't open file: %s\n", filename);
        return;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information\n");
        avformat_close_input(&pFormatCtx);
        return;
    }

    // Find the first video stream
    int videoStream = -1;
    for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        printf("Couldn't find a video stream\n");
        avformat_close_input(&pFormatCtx);
        return;
    }

    // Get codec parameters for the video stream
    AVCodecParameters* pCodecParameters = pFormatCtx->streams[videoStream]->codecpar;
    const AVCodec* pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (!pCodec) {
        printf("Unsupported codec!\n");
        avformat_close_input(&pFormatCtx);
        return;
    }

    // Allocate a codec context for the decoder
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        printf("Could not allocate codec context\n");
        avformat_close_input(&pFormatCtx);
        return;
    }

    // Copy codec parameters to context
    if (avcodec_parameters_to_context(pCodecCtx, pCodecParameters) < 0) {
        printf("Couldn't copy codec context\n");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // Open the codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // Allocate video frames
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameYUV = av_frame_alloc();
    if (!pFrameYUV) {
        printf("Could not allocate frame\n");
        av_frame_free(&pFrame);
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return;
    }

    // Determine required buffer size and allocate buffer
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 32);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    // Initialize SWS context for software scaling
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

    // Assign buffer to pFrameYUV
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 32);

    AVPacket packet;

    // Main decoding loop
    while (!g_bQuit) {
        // Read frames from the file
        if (av_read_frame(pFormatCtx, &packet) < 0) {
            // Reached end of file
            break;
        }

        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            // Send the packet to the decoder
            if (avcodec_send_packet(pCodecCtx, &packet) == 0) {
                // Receive all available frames from the decoder
                while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                    // Convert the image from its native format to YUV420P
                    sws_scale(
                        sws_ctx,
                        (uint8_t const* const*)pFrame->data,
                        pFrame->linesize,
                        0,
                        pCodecCtx->height,
                        pFrameYUV->data,
                        pFrameYUV->linesize
                    );

                    // Render the frame
                    video_player_render_frame(renderer, texture, buffer, pFrameYUV->linesize[0]);

                    // Handle SDL events (e.g., window close)
                    SDL_Event event;
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT) {
                            g_bQuit = true;
                        }
                    }

                    // Control frame rate
                    SDL_Delay(33); // Approximately 30 FPS
                }
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_packet_unref(&packet);
    }

    // Free the YUV frame and buffer
    sws_freeContext(sws_ctx);
    av_free(buffer);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
}

// Function to play video from a UDP stream
void video_player_stream_udp(SDL_Renderer* renderer, SDL_Texture* texture, int udp_port) {
    // Create a UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    // Bind the socket to the specified UDP port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // Initialize structure
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(udp_port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return;
    }

    uint8_t buffer[VIDEO_BUFFER_SIZE];
    while (!g_bQuit) {
        socklen_t addr_len = sizeof(addr);
        int len = recvfrom(sock, buffer, VIDEO_BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
        if (len > 0) {
            // Here we should decode the video packet and render it using SDL2
            // This part needs to handle the video stream decoding similar to video_player_play_file
            // For simplicity, we'll assume the data is already in YUV420P format
            video_player_render_frame(renderer, texture, buffer, VIDEO_BUFFER_SIZE / 2);
        }

        // Handle SDL events (e.g., window close)
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                g_bQuit = true;
            }
        }

        // Control frame rate
        SDL_Delay(33); // Approximately 30 FPS
    }

    // Close the socket
    close(sock);
}

int main(int argc, char* argv[]) {
    // Register signal handlers for graceful exit
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;

    // Parse command-line arguments
    if (argc < 2) {
        printf("Usage: %s [-f filename] [-u] [-p udp_port] [-w width] [-h height]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            g_bPlayFile = true;
            strncpy(g_szPlayFileName, argv[++i], sizeof(g_szPlayFileName) - 1);
            g_szPlayFileName[sizeof(g_szPlayFileName) - 1] = '\0'; // Ensure null-termination
        } else if (strcmp(argv[i], "-u") == 0) {
            g_bPlayStreamUDP = true;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            g_iUDPPort = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            g_iCustomWidth = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            g_iCustomHeight = atoi(argv[++i]);
        }
    }

    // Initialize SDL2
    if (video_player_init(&window, &renderer, &texture, g_iCustomWidth, g_iCustomHeight) != 0) {
        return 1;
    }

    // Play video from file or UDP stream based on command-line arguments
    if (g_bPlayFile) {
        video_player_play_file(g_szPlayFileName, renderer, texture);
    } else if (g_bPlayStreamUDP) {
        video_player_stream_udp(renderer, texture, g_iUDPPort);
    } else {
        printf("No valid mode selected. Use -f to play a file or -u to play a UDP stream.\n");
        g_bQuit = true;
    }

    // Clean up SDL2 resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}