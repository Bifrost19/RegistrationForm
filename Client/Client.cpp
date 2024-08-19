#include <windows.h>
#include <string>
#include <random>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

//The mail of currently logged in user 
string emailG = "";

//Handles for all UI elements(texts, input fields, check boxes, buttons)
HWND hUsername, hPassword, hEmail, hStatic1, hLink, hConfirmPassword, hCheckBox, hCancel, hCaptcha, hCaptchaBox;

//Fonts
HFONT hFont = CreateFont(30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial"));

HFONT hFontUnderLined = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial"));

HFONT hCaptchaFont = CreateFont(20, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Comic Sans MS"));

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

//Default page value
Page page = Page::REGISTER;

//Convert from wide chars to string
string LPWSTRToString(LPWSTR lpwstr) {

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, nullptr, 0, nullptr, nullptr);
    vector<char> buffer(sizeNeeded);

    WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, buffer.data(), sizeNeeded, nullptr, nullptr);

    return string(buffer.data());
}

//Destroy all child UI elements of a page
BOOL CALLBACK DestroyChildProc(HWND hWndChild, LPARAM lParam) {
    DestroyWindow(hWndChild);
    return TRUE;
}

//Send and receive data to/from the server
bool sendData(const string& username, const string& password, const string& email, const string& purpose, Error& error) {
    
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {     
        error = Error::WSASTARTUPFAILED;
        return false;
    }

    SOCKET sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
    {
        error = Error::CANTCREATESOCKET;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        error = Error::CONNECTIONFAILED;
        closesocket(sock);
        WSACleanup();
        return false;
    }

    string data = username + " " + password + " " + email + ' ' + purpose;
    send(sock, data.c_str(), data.size(), 0);

    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    buffer[bytesReceived] = '\0';
    string buffStr(buffer);
    if (buffStr == "Failed to execute operation!") return false;

    closesocket(sock);
    WSACleanup();
    return true;
}

//Check if a password contains all necessary characters
bool isPasswordLegit(const string password, int passwordLen)
{
    bool hasLowerCaseLetter = false, hasUpperCaseLetter = false, hasNumber = false, hasUniqueChar = false;

    for (int i = 0; i < passwordLen - 1; i++)
    {
        if (password[i] >= 'a' && password[i] <= 'z') hasLowerCaseLetter = true;
        else if (password[i] >= 'A' && password[i] <= 'Z') hasUpperCaseLetter = true;
        else if (password[i] >= '0' && password[i] <= '9') hasNumber = true;
        else if (password[i] >= '!' && password[i] <= '&' || password[i] >= '(' && password[i] <= '/' ||
                 password[i] >= ':' && password[i] <= '@' || password[i] >= '[' && password[i] <= '`' ||
                 password[i] >= '{' && password[i] <= '~') hasUniqueChar = true;
        else return false;       
    }
    return hasLowerCaseLetter && hasUpperCaseLetter && hasNumber && hasUniqueChar;
}

//Generate random number and write it to the captcha
void setCaptchaText()
{
    random_device rd;
    mt19937 gen(rd());
    std::uniform_int_distribution<> dist(10000, 99999);
    int randomNumber = dist(gen);

    string randomNumberStr = to_string(randomNumber);
    wstring wtext(randomNumberStr.begin(), randomNumberStr.end());
    SendMessage(hCaptcha, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(wtext.c_str()));
}

//Print all conection error messages
void printConnectionErrorMessages(Error error, HWND hwnd)
{
    switch (error)
    {
    case Error::WSASTARTUPFAILED:
        MessageBox(hwnd, TEXT("WSAStartup failed!"), TEXT("Info"), MB_OK);
        break;
    case Error::CANTCREATESOCKET:
        MessageBox(hwnd, TEXT("Can't create socket!"), TEXT("Info"), MB_OK);
        break;
    case Error::CONNECTIONFAILED:
        MessageBox(hwnd, TEXT("Connection failed!"), TEXT("Info"), MB_OK);
        break;
    }
}

//Generates all UI elements for registraion page
void registerPage(HWND hwnd)
{
    hStatic1 = CreateWindow(TEXT("static"), TEXT("REGISTRATION"), WS_VISIBLE | WS_CHILD, 350, 0, 180, 40, hwnd, NULL, NULL, NULL);

    CreateWindow(TEXT("static"), TEXT("Username:"), WS_VISIBLE | WS_CHILD, 100, 40, 80, 30, hwnd, NULL, NULL, NULL);
    hUsername = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER, 100, 70, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("static"), TEXT("Password:"), WS_VISIBLE | WS_CHILD, 100, 140, 80, 30, hwnd, NULL, NULL, NULL);
    hPassword = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 100, 170, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("static"), TEXT("Confirm Password:"), WS_VISIBLE | WS_CHILD, 100, 240, 130, 30, hwnd, NULL, NULL, NULL);
    hConfirmPassword = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 100, 270, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("static"), TEXT("Email:"), WS_VISIBLE | WS_CHILD, 100, 340, 80, 30, hwnd, NULL, NULL, NULL);
    hEmail = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER, 100, 370, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("BUTTON"), TEXT("Register"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 430, 300, 80, hwnd, (HMENU)1, nullptr, nullptr);

    hLink = CreateWindow(TEXT("static"), TEXT("Already have an account?"), WS_CHILD | WS_VISIBLE | WS_BORDER | SS_NOTIFY | SS_CENTER, 350, 520, 150, 30, hwnd, (HMENU)2, nullptr, nullptr);

    hCheckBox = CreateWindowEx(0, TEXT("BUTTON"), TEXT("Show Password"), WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 650, 450, 150, 30, hwnd, (HMENU)3, nullptr, nullptr);

    hCaptcha = CreateWindow(TEXT("static"), TEXT(""), WS_VISIBLE | WS_CHILD | SS_CENTER, 80, 450, 80, 30, hwnd, NULL, NULL, NULL);

    hCaptchaBox = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER, 80, 480, 80, 30, hwnd, nullptr, nullptr, nullptr);

    SendMessage(hStatic1, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hUsername, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hPassword, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hConfirmPassword, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hEmail, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hLink, WM_SETFONT, (WPARAM)hFontUnderLined, TRUE);
    SendMessage(hCaptcha, WM_SETFONT, (WPARAM)hCaptchaFont, TRUE);

    setCaptchaText();
}

//Generates all UI elements for login page
void loginPage(HWND hwnd)
{
    hStatic1 = CreateWindow(TEXT("static"), TEXT("LOGIN"), WS_VISIBLE | WS_CHILD | SS_CENTER, 350, 0, 180, 40, hwnd, NULL, NULL, NULL);

    CreateWindow(TEXT("static"), TEXT("Email:"), WS_VISIBLE | WS_CHILD, 100, 40, 80, 30, hwnd, NULL, NULL, NULL);
    hEmail = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER, 100, 70, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("static"), TEXT("Password:"), WS_VISIBLE | WS_CHILD, 100, 140, 80, 30, hwnd, NULL, NULL, NULL);
    hPassword = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 100, 170, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("BUTTON"), TEXT("Login"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 250, 300, 80, hwnd, (HMENU)1, nullptr, nullptr);

    CreateWindow(TEXT("BUTTON"), TEXT("Back To Register"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 350, 300, 80, hwnd, (HMENU)2, nullptr, nullptr);

    hCheckBox = CreateWindowEx(0, TEXT("BUTTON"), TEXT("Show Password"), WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 650, 250, 150, 30, hwnd, (HMENU)3, nullptr, nullptr);

    hCaptcha = CreateWindow(TEXT("static"), TEXT(""), WS_VISIBLE | WS_CHILD | SS_CENTER, 80, 250, 80, 30, hwnd, NULL, NULL, NULL);

    hCaptchaBox = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER, 80, 280, 80, 30, hwnd, nullptr, nullptr, nullptr);

    SendMessage(hStatic1, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hPassword, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hEmail, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hCaptcha, WM_SETFONT, (WPARAM)hCaptchaFont, TRUE);

    setCaptchaText();
}

//Generates all UI elements for logged in(account) page
void loggedInPage(HWND hwnd)
{
    hStatic1 = CreateWindow(TEXT("static"), TEXT("You are logged in your account!"), WS_VISIBLE | WS_CHILD, 350, 0, 180, 40, hwnd, NULL, NULL, NULL);

    CreateWindow(TEXT("BUTTON"), TEXT("Change username"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 100, 200, 70, hwnd, (HMENU)1, nullptr, nullptr);
    CreateWindow(TEXT("BUTTON"), TEXT("Change password"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 200, 200, 70, hwnd, (HMENU)2, nullptr, nullptr);
    CreateWindow(TEXT("BUTTON"), TEXT("Log out"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 300, 200, 70, hwnd, (HMENU)3, nullptr, nullptr);
}

//Generates all UI elements for change username page
void changeUsernamePage(HWND hwnd)
{
    hStatic1 = CreateWindow(TEXT("static"), TEXT("CHANGE USERNAME"), WS_VISIBLE | WS_CHILD | SS_CENTER, 320, 0, 270, 40, hwnd, NULL, NULL, NULL);

    CreateWindow(TEXT("static"), TEXT("New username:"), WS_VISIBLE | WS_CHILD, 100, 40, 120, 30, hwnd, NULL, NULL, NULL);
    hUsername = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER, 100, 70, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("static"), TEXT("Password:"), WS_VISIBLE | WS_CHILD, 100, 140, 80, 30, hwnd, NULL, NULL, NULL);
    hPassword = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 100, 170, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("BUTTON"), TEXT("Change username"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 250, 300, 80, hwnd, (HMENU)1, nullptr, nullptr);

    CreateWindow(TEXT("BUTTON"), TEXT("Cancel"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 350, 300, 80, hwnd, (HMENU)2, nullptr, nullptr);

    hCheckBox = CreateWindowEx(0, TEXT("BUTTON"), TEXT("Show Password"), WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 650, 250, 150, 30, hwnd, (HMENU)3, nullptr, nullptr);

    SendMessage(hStatic1, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hPassword, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hUsername, WM_SETFONT, (WPARAM)hFont, TRUE);
}

//Generates all UI elements for change password page
void changePasswordPage(HWND hwnd)
{
    hStatic1 = CreateWindow(TEXT("static"), TEXT("CHANGE PASSWORD"), WS_VISIBLE | WS_CHILD | SS_CENTER, 320, 0, 270, 40, hwnd, NULL, NULL, NULL);

    CreateWindow(TEXT("static"), TEXT("New Password:"), WS_VISIBLE | WS_CHILD, 100, 40, 140, 30, hwnd, NULL, NULL, NULL);
    hPassword = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 100, 70, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("static"), TEXT("Confirm New Password:"), WS_VISIBLE | WS_CHILD, 100, 140, 160, 30, hwnd, NULL, NULL, NULL);
    hConfirmPassword = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 100, 170, 700, 50, hwnd, nullptr, nullptr, nullptr);

    CreateWindow(TEXT("BUTTON"), TEXT("Change password"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 250, 300, 80, hwnd, (HMENU)1, nullptr, nullptr);

    CreateWindow(TEXT("BUTTON"), TEXT("Cancel"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 350, 300, 80, hwnd, (HMENU)2, nullptr, nullptr);

    hCheckBox = CreateWindowEx(0, TEXT("BUTTON"), TEXT("Show Password"), WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 650, 250, 150, 30, hwnd, (HMENU)3, nullptr, nullptr);

    SendMessage(hStatic1, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hPassword, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hConfirmPassword, WM_SETFONT, (WPARAM)hFont, TRUE);
}

//Main callback window function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    HDC hdc;
    RECT rect;

    switch (uMsg) {
    case WM_CREATE:
        //Initial UI generation
        registerPage(hwnd);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) //First UI action code
        {
            if (page == Page::REGISTER) // Register button
            {
                //Initialize necessary variables
                int usernameLen = GetWindowTextLength(hUsername) + 1;
                WCHAR* username = new WCHAR[usernameLen];

                int passwordLen = GetWindowTextLength(hPassword) + 1;
                WCHAR* password = new WCHAR[passwordLen];

                int confirmPasswordLen = GetWindowTextLength(hConfirmPassword) + 1;
                WCHAR* confirmPassword = new WCHAR[confirmPasswordLen];

                int emailLen = GetWindowTextLength(hEmail) + 1;
                WCHAR* email = new WCHAR[emailLen];

                int captchaTextLen = GetWindowTextLength(hCaptcha) + 1;
                WCHAR* captchaText = new WCHAR[captchaTextLen];

                int captchaTextBoxLen = GetWindowTextLength(hCaptchaBox) + 1;
                WCHAR* captchaBoxText = new WCHAR[captchaTextBoxLen];

                //Collect information from input fields and check boxes
                GetWindowText(hUsername, username, usernameLen);
                GetWindowText(hPassword, password, passwordLen);
                GetWindowText(hConfirmPassword, confirmPassword, confirmPasswordLen);
                GetWindowText(hEmail, email, emailLen);
                GetWindowText(hCaptcha, captchaText, captchaTextLen);
                GetWindowText(hCaptchaBox, captchaBoxText, captchaTextBoxLen);

                //Check for matching password input fields
                if (LPWSTRToString(password) != LPWSTRToString(confirmPassword))
                {
                    MessageBox(hwnd, TEXT("Passwords don't match!"), TEXT("Info"), MB_OK);
                    break;
                }
                if (LPWSTRToString(password).size() < 8) //Check for min password length
                {
                    MessageBox(hwnd, TEXT("The password must be at least 8 characters long!"), TEXT("Info"), MB_OK);
                    break;
                }
                if (!isPasswordLegit(LPWSTRToString(password), passwordLen)) //Check for password characters's legitimacy
                {
                    MessageBox(hwnd,
                        TEXT("The password must contain a-z, A-Z, 0-9, !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~!"),
                        TEXT("Info"), MB_OK);
                    break;
                }
                //Check for empty input fields
                if(usernameLen == 1 || passwordLen == 1 || confirmPasswordLen == 1 || emailLen == 1)
                {
                    MessageBox(hwnd, TEXT("Not all fields are filled!"), TEXT("Info"), MB_OK);
                    break;
                }
                //Check for valid captcha
                if (LPWSTRToString(captchaBoxText) != LPWSTRToString(captchaText))
                {
                    MessageBox(hwnd, TEXT("The captcha is incorrect!"), TEXT("Info"), MB_OK);
                    setCaptchaText();
                    break;
                }

                emailG = LPWSTRToString(email);
                Error error = Error::DEFAULT;

                //Send information to the server
                if (!sendData(LPWSTRToString(username), LPWSTRToString(password), LPWSTRToString(email), "Registration", error))
                {
                    if (error == Error::DEFAULT) MessageBox(hwnd, TEXT("User with such username or email already exists!"), TEXT("Info"), MB_OK);
                    else printConnectionErrorMessages(error, hwnd);
                    break;
                }

                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::LOGGEDIN;
                loggedInPage(hwnd);
                MessageBox(hwnd, TEXT("You have successfully created a new account!"), TEXT("Info"), MB_OK);
            }
            else if (page == Page::LOGIN) //Login button
            {
                //Initialize necessary variables
                int emailLen = GetWindowTextLength(hEmail) + 1;
                WCHAR* email = new WCHAR[emailLen];

                int passwordLen = GetWindowTextLength(hPassword) + 1;
                WCHAR* password = new WCHAR[passwordLen];

                int captchaTextLen = GetWindowTextLength(hCaptcha) + 1;
                WCHAR* captchaText = new WCHAR[captchaTextLen];

                int captchaTextBoxLen = GetWindowTextLength(hCaptchaBox) + 1;
                WCHAR* captchaBoxText = new WCHAR[captchaTextBoxLen];

                //Collect information from input fields and check boxes
                GetWindowText(hEmail, email, emailLen);
                GetWindowText(hPassword, password, passwordLen);
                GetWindowText(hCaptcha, captchaText, captchaTextLen);
                GetWindowText(hCaptchaBox, captchaBoxText, captchaTextBoxLen);

                //Check for empty input fields
                if (passwordLen == 1 || emailLen == 1)
                {
                    MessageBox(hwnd, TEXT("Not all fields are filled!"), TEXT("Info"), MB_OK);
                    break;
                }
                //Check for valid captcha
                if (LPWSTRToString(captchaBoxText) != LPWSTRToString(captchaText))
                {
                    MessageBox(hwnd, TEXT("The captcha is incorrect!"), TEXT("Info"), MB_OK);
                    setCaptchaText();
                    break;
                }

                emailG = LPWSTRToString(email);
                Error error = Error::DEFAULT;

                //Send information to the server
                if (!sendData("", LPWSTRToString(password), LPWSTRToString(email), "Login", error))
                {
                    if (error == Error::DEFAULT) MessageBox(hwnd, TEXT("Invalid email or incorrect password!"), TEXT("Info"), MB_OK);
                    else printConnectionErrorMessages(error, hwnd);
                    
                    break;
                }

                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::LOGGEDIN;
                loggedInPage(hwnd);
            }
            else if (page == Page::LOGGEDIN) // Change username button
            {
                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::CHANGEUSERNAME;
                changeUsernamePage(hwnd);
            }
            else if (page == Page::CHANGEUSERNAME) // change button(username)
            {
                //Initialize necessary variables
                int usernameLen = GetWindowTextLength(hUsername) + 1;
                WCHAR* username = new WCHAR[usernameLen];

                int passwordLen = GetWindowTextLength(hPassword) + 1;
                WCHAR* password = new WCHAR[passwordLen];

                //Collect information from input fields and check boxes
                GetWindowText(hUsername, username, usernameLen);
                GetWindowText(hPassword, password, passwordLen);

                //Check for empty input fields
                if (usernameLen == 1 || passwordLen == 1)
                {
                    MessageBox(hwnd, TEXT("Not all fields are filled!"), TEXT("Info"), MB_OK);
                    break;
                }

                Error error = Error::DEFAULT;

                //Send information to the server
                if (!sendData(LPWSTRToString(username), LPWSTRToString(password), emailG, "ChangeUserName", error))
                {      
                    if (error == Error::DEFAULT) MessageBox(hwnd, TEXT("User with such username already exists or incorrect password!"), TEXT("Info"), MB_OK);
                    else printConnectionErrorMessages(error, hwnd);
                    
                    break;
                }

                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::LOGGEDIN;
                loggedInPage(hwnd);
                MessageBox(hwnd, TEXT("You have successfully changed your username!"), TEXT("Info"), MB_OK);
            }
            else if (page == Page::CHANGEPASSWORD) // change button(password)
            {
                //Initialize necessary variables
                int passwordLen = GetWindowTextLength(hPassword) + 1;
                WCHAR* password = new WCHAR[passwordLen];

                int confirmPasswordLen = GetWindowTextLength(hConfirmPassword) + 1;
                WCHAR* confirmPassword = new WCHAR[confirmPasswordLen];

                //Collect information from input fields and check boxes
                GetWindowText(hConfirmPassword, confirmPassword, confirmPasswordLen);
                GetWindowText(hPassword, password, passwordLen);

                //Check for matching password input fields
                if (LPWSTRToString(password) != LPWSTRToString(confirmPassword))
                {
                    MessageBox(hwnd, TEXT("Passwords don't match!"), TEXT("Info"), MB_OK);
                    break;
                }
                if (LPWSTRToString(password).size() < 8) //Check for min password length
                {
                    MessageBox(hwnd, TEXT("The password must be at least 8 characters long!"), TEXT("Info"), MB_OK);
                    break;
                }
                //Check for password characters's legitimacy
                if (!isPasswordLegit(LPWSTRToString(password), passwordLen))
                {
                    MessageBox(hwnd,
                        TEXT("The password must contain a-z, A-Z, 0-9, !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~!"),
                        TEXT("Info"), MB_OK);
                    break;
                }
                //Check for empty input fields
                if (confirmPasswordLen == 1 || passwordLen == 1)
                {
                    MessageBox(hwnd, TEXT("Not all fields are filled!"), TEXT("Info"), MB_OK);
                    break;
                }

                Error error = Error::DEFAULT;

                //Send information to the server
                sendData("", LPWSTRToString(password), emailG, "ChangePassword", error);

                //Print connection error messages if there have any
                printConnectionErrorMessages(error, hwnd);

                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::LOGGEDIN;
                loggedInPage(hwnd);
                MessageBox(hwnd, TEXT("You have successfully changed your password!"), TEXT("Info"), MB_OK);
            }
        }
        else if (LOWORD(wParam) == 2) //Second UI action code
        {
            if (page == Page::REGISTER) //Login button
            {
                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::LOGIN;
                loginPage(hwnd);
            }
            else if (page == Page::LOGIN) //Back to register button
            {
                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::REGISTER;
                registerPage(hwnd);
            }
            else if (page == Page::LOGGEDIN) //Change password button
            {
                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::CHANGEPASSWORD;
                changePasswordPage(hwnd);
            }
            else if (page == Page::CHANGEUSERNAME || page == Page::CHANGEPASSWORD) //Cancel button
            {
                //Change page
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::LOGGEDIN;
                loggedInPage(hwnd);
            }
        }
        else if (LOWORD(wParam) == 3) //Third UI action code
        {
            //Show password check box
            if (page == Page::REGISTER || page == Page::LOGIN || page == Page::CHANGEUSERNAME || page == Page::CHANGEPASSWORD) //CheckBox
            {
                //Turn on/off show password check box
                if (SendMessage(hCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    SendMessage(hCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
                }
                else {
                    SendMessage(hCheckBox, BM_SETCHECK, BST_CHECKED, 0);
                }

                //Show password text or hide password text
                if (IsDlgButtonChecked(hwnd, 3) == BST_CHECKED)
                {
                    SendMessage(hPassword, EM_SETPASSWORDCHAR, 0, 0);
                    SendMessage(hConfirmPassword, EM_SETPASSWORDCHAR, 0, 0);
                    InvalidateRect(hPassword, NULL, TRUE);
                    InvalidateRect(hConfirmPassword, NULL, TRUE);
                }
                else
                {
                    SendMessage(hPassword, EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
                    SendMessage(hConfirmPassword, EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
                    InvalidateRect(hPassword, NULL, TRUE);
                    InvalidateRect(hConfirmPassword, NULL, TRUE);
                }

            }
            else if (page == Page::LOGGEDIN) //Log out button
            {
                EnumChildWindows(hwnd, DestroyChildProc, 0);
                InvalidateRect(hwnd, NULL, TRUE);
                page = Page::REGISTER;
                registerPage(hwnd);
                emailG = "";
            }
        }
        break;

    case WM_ERASEBKGND: //Erase background when needed
        hdc = (HDC)wParam;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
        return 1;

    case WM_DESTROY: //Destroy window when the application terminates
        PostQuitMessage(0);
        break;

    default: //Return default window
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

//Main window function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
   
    //Initialize window class and give it values to its attributes
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("Registration Form");

    //Register window class
    RegisterClass(&wc);

    //Create main application window
    HWND hwnd = CreateWindowEx(0, TEXT("Registration Form"), TEXT("Registration Form"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 900, 600, nullptr, nullptr, hInstance, nullptr);
    if (hwnd == nullptr) {
        return 0;
    }

    //Show and update window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    //Manage messages
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}