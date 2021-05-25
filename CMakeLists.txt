cmake_minimum_required(VERSION 3.18)

project(not.civ.6)

set(SERVER_SOURCES
        src/main.c
        src/net/server.c src/net/message.c src/net/client.c)
add_executable(server ${SERVER_SOURCES})