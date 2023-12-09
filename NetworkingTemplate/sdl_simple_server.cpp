#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// Constants for the window size and square size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int SQUARE_SIZE = 50;
int squareX;
int squareY;

int client_xpos;
int client_ypos;

struct square_pos {
    int xpos;
    int ypos;
};

square_pos server_square;
square_pos client_square;

// Program running bool (Graceful Exit method)
bool bRun = true;

// Color Costumization
int serverRed = 255;
int serverGreen = 0;
int serverBlue = 0;

int clientRed = 0;
int clientGreen = 255;
int clientBlue = 0;

// Function to serialize square position to JSON
std::string serializeSquarePos(int x, int y, int r, int g, int b) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("x");
    writer.Int(x);
    writer.Key("y");
    writer.Int(y);

    writer.Key("r");
    writer.Int(r);
    writer.Key("g");
    writer.Int(g);
    writer.Key("b");
    writer.Int(b);

    writer.EndObject();

    return s.GetString();
}

// Function to deserialize JSON to square position
bool deserializeSquarePos(const std::string& json, int& x, int& y, int& r, int& g, int& b) {
    rapidjson::Document doc;
    doc.Parse(json.c_str());

    if (!doc.IsObject()) {
        return false;
    }

    const rapidjson::Value& xVal = doc["x"];
    const rapidjson::Value& yVal = doc["y"];

    // pass rgb values
    const rapidjson::Value& rVal = doc["r"];
    const rapidjson::Value& gVal = doc["g"];
    const rapidjson::Value& bVal = doc["b"];

    if (!xVal.IsInt() || !yVal.IsInt()) {
        return false;
    }

    x = xVal.GetInt();
    y = yVal.GetInt();

    // get rbg values
    r = rVal.GetInt();
    g = gVal.GetInt();
    b = bVal.GetInt();

    return true;
}

// Function to handle network communications
void handleNetwork(UDPsocket udpSocket) {
    int x = 0;
    int y = 0;

    int r = 0;
    int g = 0;
    int b = 0;

    UDPpacket* receivePacket = SDLNet_AllocPacket(1024);

    if (!receivePacket) {
        SDL_Log("SDLNet_AllocPacket error: %s", SDLNet_GetError());
        SDLNet_UDP_Close(udpSocket);
        SDL_Quit();
        return;
    }

    std::cout << "UDP server is running and listening on port 12345..." << std::endl;

    while (bRun) {
     
        auto msg_data = serializeSquarePos(squareX, squareY, serverRed, serverGreen, serverBlue);
        std::cout << "Message data: " << msg_data << std::endl;

        // RECEIVE
        if (SDLNet_UDP_Recv(udpSocket, receivePacket)) {
            IPaddress clientAddress = receivePacket->address;
            Uint16 clientPort = clientAddress.port;
            const char* clientHost = SDLNet_ResolveIP(&clientAddress);

            std::cout << "Received from client " << clientHost << ":" << clientPort << ": " << receivePacket->data << std::endl;

            //int x, y;
            //int r, g, b;
            if (deserializeSquarePos(std::string(reinterpret_cast<char*>(receivePacket->data)), x, y, r, g, b)) {
                //if (deserializeSquarePos(receivePacket->data, x, y)) {
                client_xpos = x;
                client_ypos = y;

                // get the values from client and set (deserialize) in server side
                clientRed = r;
                clientGreen = g;
                clientBlue = b;
            }
        }

        // SEND
        UDPpacket* packet = SDLNet_AllocPacket(msg_data.size() + 1); // +1 for null terminator
        if (packet) {
            packet->address = receivePacket->address;
            packet->len = msg_data.size();
            memcpy(packet->data, msg_data.c_str(), msg_data.size());

            if (SDLNet_UDP_Send(udpSocket, -1, packet) == 0) {
                std::cerr << "SDLNet_UDP_Send error: " << SDLNet_GetError() << std::endl;
            }
        }

        SDLNet_FreePacket(packet);
    }

    SDLNet_FreePacket(receivePacket);
}

// Function to handle drawing
void handleDrawing(SDL_Renderer* renderer) {
    squareX = (WINDOW_WIDTH - SQUARE_SIZE) / 2;
    squareY = ((WINDOW_HEIGHT - SQUARE_SIZE) / 4) * 3;

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_UP:
                    squareY -= 10;
                    break;
                case SDLK_DOWN:
                    squareY += 10;
                    break;
                case SDLK_LEFT:
                    squareX -= 10;
                    break;
                case SDLK_RIGHT:
                    squareX += 10;
                    break;
                case SDLK_ESCAPE:
                    bRun = false;
                    quit = true;
                    break;
                case SDLK_r:
                    serverRed += 20;
                    break;
                case SDLK_g:
                    serverGreen += 20;
                    break;
                case SDLK_b:
                    serverBlue += 20;
                    break;

                }
            }
        }
        server_square.xpos = squareX;
        server_square.ypos = squareY;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        std::cout << "\nClient Position: x=" << client_xpos << ", y=" << client_ypos << std::endl;
        if (client_xpos != 0 || client_ypos != 0) {
            SDL_Rect squareRect_client = { client_xpos, client_ypos, SQUARE_SIZE, SQUARE_SIZE };
            SDL_SetRenderDrawColor(renderer, clientRed, clientGreen, clientBlue, 255);
            SDL_RenderFillRect(renderer, &squareRect_client);
        }

        SDL_Rect squareRect = { squareX, squareY, SQUARE_SIZE, SQUARE_SIZE };
        SDL_SetRenderDrawColor(renderer, serverRed, serverGreen, serverBlue, 255);
        SDL_RenderFillRect(renderer, &squareRect);

        

        SDL_RenderPresent(renderer);
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Safak Onol Networking A2 - Data Exchange using JSon Serialize methods - SERVER WINDOW",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (SDLNet_Init() < 0) {
        SDL_Log("SDLNet initialization failed: %s", SDLNet_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    UDPsocket udpSocket = SDLNet_UDP_Open(8080);

    if (!udpSocket) {
        SDL_Log("SDLNet_UDP_Open error: %s", SDLNet_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    std::thread networkThread(handleNetwork, udpSocket);

    handleDrawing(renderer);

    networkThread.join();

    SDLNet_UDP_Close(udpSocket);
    SDLNet_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}