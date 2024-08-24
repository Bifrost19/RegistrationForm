#include "Server.h"

//Contain information after executing queries on data base table
thread_local vector<string> entries;

//Create database 
bool createDatabase(sqlite3*& db) 
{
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
bool addUser(const string& username, const string& password, const string& email, sqlite3*& db)
{
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
bool loginUser(const string& email, const string& password, sqlite3*& db)
{
    entries.clear();
    string query = "SELECT Password FROM USERS WHERE Email = '" + email + "\'";
    sqlite3_exec(db, query.c_str(), CallBack, 0, 0);
    if (entries.size() != 0) return BCrypt::validatePassword(password, entries[0]);      
    else return false;
}

//Change username of a user
bool changeUsername(const string& newUsername, const string& password, const string& email, sqlite3*& db)
{
    entries.clear();
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
bool changePassword(const string& newPassword, const string& email, sqlite3*& db)
{
    //Hash the new password
    string query = "UPDATE Users SET Password = '" + BCrypt::generateHash(newPassword) + "' WHERE Email = '" + email + "';";
    sqlite3_exec(db, query.c_str(), 0, 0, 0);
    return true;
}

//Handle the queries of the client
void handleClient(tuple<SOCKET, sqlite3*&> buff) 
{
    SOCKET clientSocket = get<0>(buff);

    //Receive information from the client
    char buffer[1024];
    int bytesReceived;

    try
    {
        bytesReceived = recv(clientSocket, buffer, 1024, 0);
        buffer[bytesReceived] = '\0';
        if (bytesReceived == 0) throw "Receiving failed!";

        //Separate and store the information from the client
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
            if (addUser(username, password, email, get<1>(buff)))
                send(clientSocket, "Success!", 9, 0);
            else
                send(clientSocket, "Failed to execute operation!", 29, 0);

        }
        else if (purpose == "Login")
        {
            if (loginUser(email, password, get<1>(buff)))
                send(clientSocket, "Success!", 9, 0);
            else
                send(clientSocket, "Failed to execute operation!", 29, 0);
        }
        else if (purpose == "ChangeUserName")
        {
            if (changeUsername(username, password, email, get<1>(buff)))
                send(clientSocket, "Success!", 9, 0);
            else
                send(clientSocket, "Failed to execute operation!", 29, 0);
        }
        else if (purpose == "ChangePassword")
        {
            if (changePassword(password, email, get<1>(buff)))
                send(clientSocket, "Success!", 9, 0);
            else
                send(clientSocket, "Failed to execute operation!", 29, 0);
        }
    }
    catch (exception e)
    {
        cout << "Error: " << e.what() << endl;
        closesocket(clientSocket);
    }

    closesocket(clientSocket);
}

//Set the socket, address and listen for client connections
void handleServer(sqlite3*& db)
{
    WSADATA wsaData;
    SOCKET serverSocket;

    try
    {
        //Initialize Win socket
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) throw "Windows socket initialization failed!";

        //Initialize server socket
        if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) throw "Server socket initialization failed!";

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(8080);

        //Bind the socket to an address
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) throw "Binding failed!";

        //Set the server socket to listen for client connections
        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) throw "Listening failed!";
    }
    catch (exception e)
    {
        cerr << "Error: " << e.what() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    while (true) 
    {
        cout << "...Waiting for connection..." << endl;

        try
        {
            SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) throw "Invalid socket";
            cout << "Connection succeeded!" << endl;

            //Collect two arguments in one
            tuple<SOCKET, sqlite3*&> buff(clientSocket, db);
            // Create a thread for each client
            thread clientThread(handleClient, buff);
            clientThread.detach(); // Detach the thread to let it run independently
        }
        catch (exception e)
        {
            cerr << "Error: " << e.what() << endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

    }

    //Close server socket
    closesocket(serverSocket);
    WSACleanup();
}

//Create or open database
int main() //Comment before testing with doctest
{
    sqlite3* db;
    sqlite3_open("users.db", &db);
    createDatabase(db);

    handleServer(db);
    sqlite3_close(db);

    return 0;
}

