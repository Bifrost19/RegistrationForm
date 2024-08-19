#include <iostream>
#include <sqlite3.h>
#include <string>
#include <winsock2.h>
#include <vector>
#include <thread>
#include "bcrypt/BCrypt.hpp"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

//Database global variable
sqlite3* db;

//Contain information after executing queries on data base table
vector<string> entries;

//Create database
bool createDatabase() {
    const char* sqlCreateTable = R"(
        CREATE TABLE IF NOT EXISTS Users (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Username TEXT NOT NULL UNIQUE,
            Password TEXT NOT NULL,
            Email TEXT NOT NULL UNIQUE
        );
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sqlCreateTable, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

//Database queries callback function
static int CallBack(void* unused, int count, char** data, char** columns)
{
    for (int i = 0; i < count; i++)
    {
        string buffer(data[i]);
        entries.push_back(buffer);
    }
    return 0;
}

//Add new user in the database
bool addUser(const string& username, const string& password, const string& email) {

    char* errorMessage = nullptr;
    //All passwords are being hashed before saving them in the database
    string sql = "INSERT INTO USERS (USERNAME, PASSWORD, EMAIL) VALUES ('" + username + "', '" + BCrypt::generateHash(password) + "', '" + email + "');";
    int exit = sqlite3_exec(db, sql.c_str(), NULL, 0, &errorMessage);
    if (exit != SQLITE_OK) {
        cerr << "Error inserting data: " << errorMessage << endl;
        sqlite3_free(errorMessage);
        return false;
    }
    return true;
}

//Check if there is a user with such email and correct password
bool loginUser(const string& email, const string& password)
{
    string query = "SELECT Password FROM USERS WHERE Email = '" + email + "\'";
    sqlite3_exec(db, query.c_str(), CallBack, 0, 0);
    if (entries.size() != 0) return BCrypt::validatePassword(password, entries[0]);      
    else return false;
}

//Change username of a user
bool changeUsername(const string& newUsername, const string& password, const string& email)
{
    //Get password of a user with such email
    string initQuery = "SELECT Password FROM USERS WHERE Email = '" + email + "'";
    sqlite3_exec(db, initQuery.c_str(), CallBack, 0, 0);

    //Check if the entered password matches the official one
    if (!BCrypt::validatePassword(password, entries[0])) return false;
    
    //Check if there is another user with the desired username
    entries.clear();
    string secQuery = "SELECT Username FROM USERS WHERE UserName = '" + newUsername + "'";
    sqlite3_exec(db, secQuery.c_str(), CallBack, 0, 0);
    if (entries.size() > 0) return false;

    //Change user's username
    string query = "UPDATE Users SET Username = '" + newUsername + "' WHERE Email = '" + email + "';";
    sqlite3_exec(db, query.c_str(), 0, 0, 0);
    return true;
}

//Change password of a user
bool changePassword(const string& newPassword, const string& email)
{
    //Hash the new password
    string query = "UPDATE Users SET Password = '" + BCrypt::generateHash(newPassword) + "' WHERE Email = '" + email + "';";
    sqlite3_exec(db, query.c_str(), 0, 0, 0);
    return true;
}

//Handle the queries of the client
void handleClient(SOCKET clientSocket) 
{
    //Receive information from the client
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[bytesReceived] = '\0';

    if (bytesReceived > 0) {

        //Separate and store the information from the client
        entries.clear();
        string data(buffer), username, password, email, purpose;
        int counter = 0;
        for (int i = 0; i < data.size(); i++)
        {
            switch (counter)
            {
            case 0:
                username += data[i];
                break;
            case 1:
                password += data[i];
                break;
            case 2:
                email += data[i];
                break;
            case 3:
                purpose += data[i];
                break;
            }
            if (data[i] == ' ') counter++;
        }
        username = username.substr(0, username.size() - 1);
        password = password.substr(0, password.size() - 1);
        email = email.substr(0, email.size() - 1);

        //Distribute tasks depending on the type of user request
        if (purpose == "Registration")
        {
            if (addUser(username, password, email))
                send(clientSocket, "Success!", 9, 0);
            else 
                send(clientSocket, "Failed to execute operation!", 29, 0);

        }
        else if (purpose == "Login")
        {
               if(loginUser(email, password)) 
                   send(clientSocket, "Success!", 9, 0);
               else 
                   send(clientSocket, "Failed to execute operation!", 29, 0);
        }
        else if (purpose == "ChangeUserName")
        {
            if (changeUsername(username, password, email))
                send(clientSocket, "Success!", 9, 0);
            else
                send(clientSocket, "Failed to execute operation!", 29, 0);
        }
        else if (purpose == "ChangePassword")
        {
            if (changePassword(password, email))
                send(clientSocket, "Success!", 9, 0);
            else
                send(clientSocket, "Failed to execute operation!", 29, 0);
        }
    }

    closesocket(clientSocket);
}

//Set the socket, address and listen for client connections
int main() {

    sqlite3_open("users.db", &db);
    createDatabase();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed!" << endl;
        return 1;
    }

    SOCKET serverSocket;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Can't create socket! Error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Can't bind socket! Error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listening failed! Error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    while (true) {
        cout << "...Waiting for connection..." << endl;
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket != INVALID_SOCKET) {
            cout << "Connection succeeded!" << endl;

            // Create a thread for each client
            thread clientThread(handleClient, clientSocket);
            clientThread.detach(); // Detach the thread to let it run independently
        }
        else
        {
            cerr << "Accept failed! Error: " << WSAGetLastError() << endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }
    }

    sqlite3_close(db);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}