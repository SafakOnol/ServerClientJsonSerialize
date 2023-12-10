#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include "../NetworkingTemplate/rapidjson/document.h"
#include "../NetworkingTemplate/rapidjson/writer.h"
#include "../NetworkingTemplate/rapidjson/stringbuffer.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int SQUARE_SIZE = 50;
int squareX;
int squareY;

int x = 0;
int y = 0;
int r = 0;
int g = 0;
int b = 0;

int server_xpos;
int server_ypos;

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

// Function to handle sending the client position to the server
void sendPosition(UDPsocket udpSocket, IPaddress serverIP) {
    while (bRun) {
        std::string msg_data = serializeSquarePos(squareX, squareY, clientRed, clientGreen, clientBlue);

        // Create a UDP packet to send the position to the server
        UDPpacket* packet = SDLNet_AllocPacket(msg_data.size() + 1);
        if (packet) {
            packet->address = serverIP;
            packet->len = msg_data.size();
            memcpy(packet->data, msg_data.c_str(), msg_data.size());

            if (SDLNet_UDP_Send(udpSocket, -1, packet) == 0) {
                std::cerr << "SDLNet_UDP_Send error: " << SDLNet_GetError() << std::endl;
            }

            SDLNet_FreePacket(packet);
        }

        SDL_Delay(16); // Cap the sending rate to approximately 60 FPS
    }
}



// Function to handle receiving data from the server
void receiveFromServer(UDPsocket udpSocket, UDPpacket* receivePacket, IPaddress serverIP) {
    while (bRun) {
        // Check for incoming UDP packets (data from the server)
        if (SDLNet_UDP_Recv(udpSocket, receivePacket)) {
          const char* clientHost = SDLNet_ResolveIP(&serverIP);
          Uint16 clientPort = serverIP.port;

            std::cout << "Received from client " << clientHost << ":" << clientPort << ": " << receivePacket->data << std::endl;

            if (deserializeSquarePos(std::string(reinterpret_cast<char*>(receivePacket->data)), x, y, r, g, b)) {
                //if (deserializeSquarePos(receivePacket->data, x, y)) {
                server_xpos = x;
                server_ypos = y;
                serverRed = r;
                serverGreen = g;
                serverBlue = b;
            }

        }

    }
}


int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Safak Onol Networking A2 - Data Exchange using JSon Serialize methods - CLIENT WINDOW", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) {
        std::cerr << "Failed to create window or renderer: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    squareX = (WINDOW_WIDTH - SQUARE_SIZE) / 2;
    squareY = (WINDOW_HEIGHT - SQUARE_SIZE) / 4;

    bool quit = false;
    SDL_Event e;

    if (SDLNet_Init() < 0) {
        SDL_Log("SDLNet initialization failed: %s", SDLNet_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    UDPsocket udpSocket;
    udpSocket = SDLNet_UDP_Open(0);

    if (!udpSocket) {
        std::cerr << "SDLNet_UDP_Open error: " << SDLNet_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create a UDP packet to receive data from the server
    UDPpacket* receivePacket = SDLNet_AllocPacket(1024);

    if (!receivePacket) {
        SDL_Log("SDLNet_AllocPacket error: %s", SDLNet_GetError());
        SDLNet_UDP_Close(udpSocket);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    IPaddress serverIP;

    if (SDLNet_ResolveHost(&serverIP, "127.0.0.1", 8080) < 0) {
        std::cerr << "SDLNet_ResolveHost error: " << SDLNet_GetError() << std::endl;
        SDLNet_UDP_Close(udpSocket);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "UDP client is running and connected to the server on port 8080..." << std::endl;

    // Create threads for network communication
    std::thread sendThread(sendPosition, udpSocket, serverIP);
    std::thread receiveThread(receiveFromServer, udpSocket, receivePacket, serverIP);

    while (bRun) {
        // Handle events, update positions, etc. (same as before)
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
                    clientRed += 20;
                    break;
                case SDLK_g:
                    clientGreen += 20;
                    break;
                case SDLK_b:
                    clientBlue += 20;
                    break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect squareRect = { squareX, squareY, SQUARE_SIZE, SQUARE_SIZE };
        SDL_SetRenderDrawColor(renderer, clientRed, clientGreen, clientBlue, 255);
        SDL_RenderFillRect(renderer, &squareRect);

        std::cout << "\nServer Position: x=" << server_xpos << ", y=" << server_ypos << std::endl;
        if (server_xpos != 0 || server_ypos != 0) {
            SDL_Rect squareRect_server = { server_xpos, server_ypos, SQUARE_SIZE, SQUARE_SIZE };
            SDL_SetRenderDrawColor(renderer, serverRed, serverGreen, serverBlue, 255);
            SDL_RenderFillRect(renderer, &squareRect_server);
        }

        SDL_RenderPresent(renderer);
    }

    // Join the network threads and clean up
    sendThread.join();
    receiveThread.join();

    SDLNet_UDP_Close(udpSocket);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}