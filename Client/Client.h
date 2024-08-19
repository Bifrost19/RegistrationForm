#pragma once

#include "resource.h"
#include <windows.h>
#include <string>
#include <random>
#include <vector>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

//All pages in the app have their own unique code
enum class Page
{
    REGISTER,
    LOGIN,
    LOGGEDIN,
    CHANGEUSERNAME,
    CHANGEPASSWORD
};

//Different errors with the connection server-client
enum class Error
{
    DEFAULT,
    WSASTARTUPFAILED,
    CANTCREATESOCKET,
    CONNECTIONFAILED
};

//Convert from wide chars to string
string LPWSTRToString(LPWSTR lpwstr);

//Destroy all child UI elements of a page
BOOL CALLBACK DestroyChildProc(HWND hWndChild, LPARAM lParam);

//Send and receive data to/from the server
bool sendData(const string& username, const string& password, const string& email, const string& purpose, Error& error);

//Check if a password contains all necessary characters
bool isPasswordLegit(const string password, int passwordLen);

//Generate random number and write it to the captcha
void setCaptchaText();

//Print all conection error messages
void printConnectionErrorMessages(Error error, HWND hwnd);

//Generates all UI elements for registraion page
void registerPage(HWND hwnd);

//Generates all UI elements for login page
void loginPage(HWND hwnd);

//Generates all UI elements for logged in(account) page
void loggedInPage(HWND hwnd);

//Generates all UI elements for change username page
void changeUsernamePage(HWND hwnd);

//Generates all UI elements for change password page
void changePasswordPage(HWND hwnd);

//Main callback window function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);