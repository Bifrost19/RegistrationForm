#pragma once
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <winsock2.h>
#include <vector>
#include <thread>
#include "bcrypt/BCrypt.hpp"
#pragma comment(lib, "ws2_32.lib")

using namespace std;

//Create database
bool createDatabase(sqlite3*& db);

//Database queries callback function
static int CallBack(void* unused, int count, char** data, char** columns);

//Add new user in the database
bool addUser(const string& username, const string& password, const string& email, sqlite3*& db);

//Check if there is a user with such email and correct password
bool loginUser(const string& email, const string& password, sqlite3*& db);

//Change username of a user
bool changeUsername(const string& newUsername, const string& password, const string& email, sqlite3*& db);

//Change password of a user
bool changePassword(const string& newPassword, const string& email, sqlite3*& db);

//Handle the queries of the client
void handleClient(tuple<SOCKET, sqlite3*&> buff);

//Set the socket, address and listen for client connections
void handleServer(sqlite3*& db);
