// FORCE ANSI MODE
#undef UNICODE
#undef _UNICODE

#define _WIN32_WINNT 0x0600 
#define _WIN32_IE    0x0500 

#include <windows.h>
#include <wincrypt.h> // Windows Cryptography API
#include <commctrl.h> 
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <map>
#include <random>     // For salt generation
#include "sqlite3.h"

// Auto-link the cryptography library for MSVC compilers
#pragma comment(lib, "advapi32.lib")

// --- CONTROLS IDENTIFIERS ---
#define ID_BTN_REGISTER     2001
#define ID_BTN_LOGIN        2002
#define ID_BTN_ADD_TRANS    2003
#define ID_BTN_DELETE_ACC   2004
#define ID_CBD_TYPE         2005
#define ID_BTN_LOGOUT       2006
#define ID_BTN_EDIT_ENTRY   2007
#define ID_BTN_DELETE_ENTRY 2008
#define ID_LISTVIEW         2009
#define ID_BTN_ANALYTICS    2010

// --- GLOBAL UI HANDLES ---
HWND hLoginTitle, hUserLabel, hUsername, hPassLabel, hPassword, hBtnRegister, hBtnLogin; 
HWND hFormTitle, hAmtLabel, hAmount, hTypeLabel, hTypeCombo, hCatLabel, hCatCombo, hDateLabel, hDate, hDescLabel, hDesc, hBtnCommit; 
HWND hDashTitle, hListView, hSummaryBox, hBtnEditEntry, hBtnDeleteEntry, hBtnLogout, hBtnDeleteAcc, hStatus, hBtnAnalytics; 

std::vector<HWND> loginViewControls;
std::vector<HWND> dashboardViewControls;

sqlite3* db = nullptr;
int loggedInUserId = -1;
std::string loggedInUsername = "";

// --- CATEGORIES ---
const std::vector<std::string> incomeCategories = { "Salary", "Trading Profits", "Business Revenue", "Freelance/Consulting", "Other Income" };
const std::vector<std::string> fixedExpenses    = { "Housing", "Utilities", "Communications", "Transportation", "Insurance", "Debt Repayment", "Child Care" };
const std::vector<std::string> variableExpenses = { "Groceries", "Dining Out", "Personal Care", "Clothing", "Entertainment", "Health & Wellness", "Gifts", "Travel" };

// --- SECURITY & HASHING ---

// Fallback definition for compilers missing the SHA-256 macro
#ifndef CALG_SHA_256
#define CALG_SHA_256 0x0000800c
#endif

// Secure OS-level SHA-256 implementation
std::string sha256(const std::string& data) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string hashStr = "";

    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            // Cast to (BYTE*) instead of (const BYTE*) to satisfy strict IntelliSense rules
            if (CryptHashData(hHash, (BYTE*)data.c_str(), (DWORD)data.length(), 0)) {
                DWORD hashLen = 0;
                CryptGetHashParam(hHash, HP_HASHVAL, NULL, &hashLen, 0);
                
                std::vector<BYTE> hash(hashLen);
                if (CryptGetHashParam(hHash, HP_HASHVAL, hash.data(), &hashLen, 0)) {
                    std::stringstream ss;
                    for (DWORD i = 0; i < hashLen; i++) {
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
                    }
                    hashStr = ss.str();
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return hashStr;
}

// Generate a random 16-character salt
std::string generateSalt() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string salt;
    for (int i = 0; i < 16; ++i) {
        salt += charset[dist(generator)];
    }
    return salt;
}

// Hash the password combined with the unique salt
std::string hashPassword(const std::string& password, const std::string& salt) { 
    return sha256(password + salt); 
}


// --- DATABASE MANAGER ---
bool initDatabase() {
    if (sqlite3_open("finance_tracker.db", &db) != SQLITE_OK) return false;
    const char* schema = 
        "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE NOT NULL, password_hash TEXT NOT NULL);"
        "CREATE TABLE IF NOT EXISTS transactions (id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER, amount REAL, class_type TEXT, category TEXT, date TEXT, description TEXT, FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE);";
    sqlite3_exec(db, schema, nullptr, nullptr, nullptr);
    return true;
}

void displayApplicationState(bool userAuthenticated) {
    for (HWND hwnd : loginViewControls) ShowWindow(hwnd, userAuthenticated ? SW_HIDE : SW_SHOW);
    for (HWND hwnd : dashboardViewControls) ShowWindow(hwnd, userAuthenticated ? SW_SHOW : SW_HIDE);
}

void repopulateCategoryDropdown() {
    SendMessageA(hCatCombo, CB_RESETCONTENT, 0, 0);
    char typeBuffer[40];
    int selIdx = SendMessageA(hTypeCombo, CB_GETCURSEL, 0, 0);
    SendMessageA(hTypeCombo, CB_GETLBTEXT, selIdx, (LPARAM)typeBuffer);
    std::string selectedType(typeBuffer);

    const std::vector<std::string>* targetList = &incomeCategories;
    if (selectedType == "Fixed Expense") targetList = &fixedExpenses;
    else if (selectedType == "Variable Expense") targetList = &variableExpenses;

    for (const auto& category : *targetList) {
        SendMessageA(hCatCombo, CB_ADDSTRING, 0, (LPARAM)category.c_str());
    }
    SendMessageA(hCatCombo, CB_SETCURSEL, 0, 0);
}

void updateDashboardReport() {
    if (loggedInUserId == -1) return;
    
    ListView_DeleteAllItems(hListView);

    const char* sql = "SELECT id, date, class_type, amount, category, description FROM transactions WHERE user_id = ? ORDER BY date ASC;";
    sqlite3_stmt* stmt;
    
    double totalInflow = 0, totalFixed = 0, totalVariable = 0;
    int index = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, loggedInUserId);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int dbId = sqlite3_column_int(stmt, 0); 
            std::string dt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::string tp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            double amt = sqlite3_column_double(stmt, 3);
            std::string ct = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            std::string ds = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

            if (tp == "Income") totalInflow += amt;
            else if (tp == "Fixed Expense") totalFixed += amt;
            else totalVariable += amt;

            std::string displayId = std::to_string(index + 1);

            LVITEM lvItem = {0};
            lvItem.mask = LVIF_TEXT | LVIF_PARAM; 
            lvItem.iItem = index;
            lvItem.iSubItem = 0;
            lvItem.pszText = (LPSTR)displayId.c_str();
            lvItem.lParam = dbId; 
            ListView_InsertItem(hListView, &lvItem);

            std::string amtStr = std::to_string(amt);
            amtStr = amtStr.substr(0, amtStr.find(".") + 3);

            ListView_SetItemText(hListView, index, 1, (LPSTR)dt.c_str());
            ListView_SetItemText(hListView, index, 2, (LPSTR)tp.c_str());
            ListView_SetItemText(hListView, index, 3, (LPSTR)amtStr.c_str());
            ListView_SetItemText(hListView, index, 4, (LPSTR)ct.c_str());
            ListView_SetItemText(hListView, index, 5, (LPSTR)ds.c_str());

            index++;
        }
        double activeSavings = totalInflow - (totalFixed + totalVariable);
        
        std::stringstream ss;
        ss << " Income: +$" << totalInflow << "  |  Fixed Expenses: -$" << totalFixed 
           << "  |  Variable Expenses: -$" << totalVariable << "  |  Net Savings: $" << activeSavings;
        SetWindowTextA(hSummaryBox, ss.str().c_str());
    }
    sqlite3_finalize(stmt);
}

// --- SYSTEM WINDOW PROCESS ROUTER ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    char userBuffer[100], passBuffer[100], amtBuffer[50], typeBuffer[40], catBuffer[100], dateBuffer[30], descBuffer[250];

    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = CreateFontA(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

        hLoginTitle = CreateWindowA("STATIC", "Sign In to Your Tracker", WS_CHILD | SS_CENTER, 380, 110, 320, 25, hwnd, NULL, NULL, NULL);
        hUserLabel  = CreateWindowA("STATIC", "Username:", WS_CHILD, 380, 160, 320, 20, hwnd, NULL, NULL, NULL);
        hUsername   = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 380, 185, 320, 26, hwnd, NULL, NULL, NULL);
        hPassLabel  = CreateWindowA("STATIC", "Password:", WS_CHILD, 380, 225, 320, 20, hwnd, NULL, NULL, NULL);
        hPassword   = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL, 380, 250, 320, 26, hwnd, NULL, NULL, NULL);
        hBtnLogin   = CreateWindowA("BUTTON", "Log In", WS_CHILD, 380, 300, 320, 38, hwnd, (HMENU)ID_BTN_LOGIN, NULL, NULL);
        hBtnRegister= CreateWindowA("BUTTON", "Create Account", WS_CHILD, 380, 350, 320, 32, hwnd, (HMENU)ID_BTN_REGISTER, NULL, NULL);

        loginViewControls = { hLoginTitle, hUserLabel, hUsername, hPassLabel, hPassword, hBtnLogin, hBtnRegister };

        hFormTitle  = CreateWindowA("STATIC", "ADD NEW TRANSACTION", WS_CHILD | SS_LEFT, 25, 20, 320, 25, hwnd, NULL, NULL, NULL);
        hAmtLabel   = CreateWindowA("STATIC", "Amount ($):", WS_CHILD, 25, 60, 320, 18, hwnd, NULL, NULL, NULL);
        hAmount     = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 25, 80, 320, 26, hwnd, NULL, NULL, NULL);
        
        hTypeLabel  = CreateWindowA("STATIC", "Transaction Type:", WS_CHILD, 25, 120, 320, 18, hwnd, NULL, NULL, NULL);
        hTypeCombo  = CreateWindowA("COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 25, 140, 320, 200, hwnd, (HMENU)ID_CBD_TYPE, NULL, NULL);
        
        hCatLabel   = CreateWindowA("STATIC", "Category:", WS_CHILD, 25, 180, 320, 18, hwnd, NULL, NULL, NULL);
        hCatCombo   = CreateWindowA("COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 25, 200, 320, 200, hwnd, NULL, NULL, NULL);

        hDateLabel  = CreateWindowA("STATIC", "Date (YYYY-MM-DD):", WS_CHILD, 25, 240, 320, 18, hwnd, NULL, NULL, NULL);
        hDate       = CreateWindowA("EDIT", "2026-05-18", WS_CHILD | WS_BORDER, 25, 260, 320, 26, hwnd, NULL, NULL, NULL);
        hDescLabel  = CreateWindowA("STATIC", "Note / Description:", WS_CHILD, 25, 300, 320, 18, hwnd, NULL, NULL, NULL);
        hDesc       = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 25, 320, 320, 26, hwnd, NULL, NULL, NULL);
        
        hBtnCommit  = CreateWindowA("BUTTON", "Add Transaction", WS_CHILD, 25, 360, 320, 40, hwnd, (HMENU)ID_BTN_ADD_TRANS, NULL, NULL);
        hBtnEditEntry= CreateWindowA("BUTTON", "Edit Selected", WS_CHILD, 25, 410, 155, 32, hwnd, (HMENU)ID_BTN_EDIT_ENTRY, NULL, NULL);
        hBtnDeleteEntry= CreateWindowA("BUTTON", "Delete Selected", WS_CHILD, 190, 410, 155, 32, hwnd, (HMENU)ID_BTN_DELETE_ENTRY, NULL, NULL);
        
        hBtnLogout   = CreateWindowA("BUTTON", "Log Out", WS_CHILD, 25, 452, 155, 32, hwnd, (HMENU)ID_BTN_LOGOUT, NULL, NULL);
        hBtnDeleteAcc= CreateWindowA("BUTTON", "Delete Account", WS_CHILD, 190, 452, 155, 32, hwnd, (HMENU)ID_BTN_DELETE_ACC, NULL, NULL);

        hDashTitle  = CreateWindowA("STATIC", "Recent Transactions", WS_CHILD | SS_LEFT, 375, 20, 680, 25, hwnd, NULL, NULL, NULL);
        hListView   = CreateWindowExA(0, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL, 375, 48, 680, 342, hwnd, (HMENU)ID_LISTVIEW, NULL, NULL);
        
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        
        lvc.pszText = (LPSTR)"ID"; lvc.cx = 45; ListView_InsertColumn(hListView, 0, &lvc);
        lvc.pszText = (LPSTR)"Date"; lvc.cx = 90; ListView_InsertColumn(hListView, 1, &lvc);
        lvc.pszText = (LPSTR)"Type"; lvc.cx = 120; ListView_InsertColumn(hListView, 2, &lvc);
        lvc.pszText = (LPSTR)"Amount"; lvc.cx = 95; ListView_InsertColumn(hListView, 3, &lvc);
        lvc.pszText = (LPSTR)"Category"; lvc.cx = 140; ListView_InsertColumn(hListView, 4, &lvc);
        lvc.pszText = (LPSTR)"Note"; lvc.cx = 185; ListView_InsertColumn(hListView, 5, &lvc);

        hSummaryBox = CreateWindowA("STATIC", "", WS_CHILD | WS_BORDER | SS_LEFT, 375, 400, 680, 44, hwnd, NULL, NULL, NULL);
        
        hBtnAnalytics = CreateWindowA("BUTTON", "Open Smart Analytics Dashboard", WS_CHILD, 375, 452, 680, 32, hwnd, (HMENU)ID_BTN_ANALYTICS, NULL, NULL);

        hStatus     = CreateWindowA("STATIC", "Status: Ready.", WS_CHILD | SS_LEFTNOWORDWRAP, 25, 495, 1030, 20, hwnd, NULL, NULL, NULL);

        dashboardViewControls = { hFormTitle, hAmtLabel, hAmount, hTypeLabel, hTypeCombo, hCatLabel, hCatCombo, hDateLabel, hDate, hDescLabel, hDesc, hBtnCommit, hBtnEditEntry, hBtnDeleteEntry, hBtnLogout, hBtnDeleteAcc, hDashTitle, hListView, hSummaryBox, hBtnAnalytics };

        for (HWND ctrl : loginViewControls)     SendMessageA(ctrl, WM_SETFONT, (WPARAM)hFont, TRUE);
        for (HWND ctrl : dashboardViewControls) SendMessageA(ctrl, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageA(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

        SendMessageA(hTypeCombo, CB_ADDSTRING, 0, (LPARAM)"Income");
        SendMessageA(hTypeCombo, CB_ADDSTRING, 0, (LPARAM)"Fixed Expense");
        SendMessageA(hTypeCombo, CB_ADDSTRING, 0, (LPARAM)"Variable Expense");
        SendMessageA(hTypeCombo, CB_SETCURSEL, 0, 0);

        repopulateCategoryDropdown();
        displayApplicationState(false); 
        break;
    }

    case WM_TIMER: {
        if (wp == 1) { 
            KillTimer(hwnd, 1); 
            SetWindowTextA(hBtnAnalytics, "Open Smart Analytics Dashboard"); 
            EnableWindow(hBtnAnalytics, TRUE); 
        }
        break;
    }

    case WM_COMMAND: {
        if (HIWORD(wp) == CBN_SELCHANGE && LOWORD(wp) == ID_CBD_TYPE) {
            repopulateCategoryDropdown();
            break;
        }

        switch (LOWORD(wp)) {
        
        case ID_BTN_ANALYTICS: {
            SetWindowTextA(hBtnAnalytics, "Loading AI Engine... Please Wait");
            EnableWindow(hBtnAnalytics, FALSE); 
            system("start pythonw analytics.py");
            SetTimer(hwnd, 1, 3500, NULL); 
            break;
        }

        case ID_BTN_REGISTER:
        {
            GetWindowTextA(hUsername, userBuffer, 100); GetWindowTextA(hPassword, passBuffer, 100);
            if (strlen(userBuffer) < 3 || strlen(passBuffer) < 4) {
                MessageBoxA(hwnd, "Please enter a valid username and password.", "Registration Failed", MB_ICONWARNING);
                break;
            }
            
            // Generate Salt and Hash
            std::string salt = generateSalt();
            std::string hashed = hashPassword(passBuffer, salt);
            
            // Store as "salt:hash" so no database schema change is needed
            std::string dbEntry = salt + ":" + hashed;

            const char* sql = "INSERT INTO users (username, password_hash) VALUES (?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, userBuffer, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, dbEntry.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt) == SQLITE_DONE) MessageBoxA(hwnd, "Account created successfully! You can now log in.", "Success", MB_OK);
                else MessageBoxA(hwnd, "That username is already taken. Please choose another.", "Error", MB_ICONERROR);
            }
            sqlite3_finalize(stmt);
            break;
        }

        case ID_BTN_LOGIN:
        {
            GetWindowTextA(hUsername, userBuffer, 100); GetWindowTextA(hPassword, passBuffer, 100);
            
            // Fetch the ID and the stored salt:hash string
            const char* sql = "SELECT id, password_hash FROM users WHERE username = ?;";
            sqlite3_stmt* stmt;
            bool loginSuccess = false;

            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, userBuffer, -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    int dbId = sqlite3_column_int(stmt, 0); 
                    std::string storedData = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    
                    // Split the stored data into salt and hash
                    size_t colonPos = storedData.find(':');
                    if (colonPos != std::string::npos) {
                        std::string salt = storedData.substr(0, colonPos);
                        std::string storedHash = storedData.substr(colonPos + 1);
                        
                        // Verify inputted password against the retrieved salt and expected hash
                        if (hashPassword(passBuffer, salt) == storedHash) {
                            loginSuccess = true;
                            loggedInUserId = dbId;
                            loggedInUsername = userBuffer;
                        }
                    }
                }
            }
            sqlite3_finalize(stmt);

            if (loginSuccess) {
                std::string welcomeTitle = "Welcome back, " + loggedInUsername + "! Here are your transactions:";
                SetWindowTextA(hDashTitle, welcomeTitle.c_str());
                SetWindowTextA(hStatus, "Status: Logged in successfully.");
                displayApplicationState(true); 
                updateDashboardReport();
            } else {
                MessageBoxA(hwnd, "Incorrect username or password.", "Login Failed", MB_ICONERROR);
            }
            break;
        }

        case ID_BTN_ADD_TRANS:
        {
            GetWindowTextA(hAmount, amtBuffer, 50);
            if (strlen(amtBuffer) == 0) { MessageBoxA(hwnd, "Please enter an amount.", "Missing Info", MB_OK); break; }
            
            int typeIdx = SendMessageA(hTypeCombo, CB_GETCURSEL, 0, 0); SendMessageA(hTypeCombo, CB_GETLBTEXT, typeIdx, (LPARAM)typeBuffer);
            int catIdx  = SendMessageA(hCatCombo, CB_GETCURSEL, 0, 0);  SendMessageA(hCatCombo, CB_GETLBTEXT, catIdx, (LPARAM)catBuffer);
            GetWindowTextA(hDate, dateBuffer, 30); GetWindowTextA(hDesc, descBuffer, 250);

            double amt = std::stod(amtBuffer);
            const char* sql = "INSERT INTO transactions (user_id, amount, class_type, category, date, description) VALUES (?, ?, ?, ?, ?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, loggedInUserId); sqlite3_bind_double(stmt, 2, amt);
                sqlite3_bind_text(stmt, 3, typeBuffer, -1, SQLITE_TRANSIENT); sqlite3_bind_text(stmt, 4, catBuffer, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 5, dateBuffer, -1, SQLITE_TRANSIENT); sqlite3_bind_text(stmt, 6, descBuffer, -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    SetWindowTextA(hStatus, "Status: Transaction added.");
                    SetWindowTextA(hAmount, ""); SetWindowTextA(hDesc, ""); 
                    updateDashboardReport();
                }
            }
            sqlite3_finalize(stmt);
            break;
        }

        case ID_BTN_EDIT_ENTRY:
        {
            int selectedRow = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (selectedRow == -1) { MessageBoxA(hwnd, "Please select a transaction from the list first.", "Select Item", MB_OK); break; }
            
            LVITEM item = {0};
            item.iItem = selectedRow;
            item.mask = LVIF_PARAM;
            ListView_GetItem(hListView, &item);
            int realDbId = item.lParam;
            
            GetWindowTextA(hAmount, amtBuffer, 50);
            if (strlen(amtBuffer) == 0) { MessageBoxA(hwnd, "Please enter the updated information in the fields on the left.", "Missing Info", MB_OK); break; }

            int typeIdx = SendMessageA(hTypeCombo, CB_GETCURSEL, 0, 0); SendMessageA(hTypeCombo, CB_GETLBTEXT, typeIdx, (LPARAM)typeBuffer);
            int catIdx  = SendMessageA(hCatCombo, CB_GETCURSEL, 0, 0);  SendMessageA(hCatCombo, CB_GETLBTEXT, catIdx, (LPARAM)catBuffer);
            GetWindowTextA(hDate, dateBuffer, 30); GetWindowTextA(hDesc, descBuffer, 250);

            const char* sql = "UPDATE transactions SET amount = ?, class_type = ?, category = ?, date = ?, description = ? WHERE id = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_double(stmt, 1, std::stod(amtBuffer));
                sqlite3_bind_text(stmt, 2, typeBuffer, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, catBuffer, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 4, dateBuffer, -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 5, descBuffer, -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 6, realDbId); 

                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    SetWindowTextA(hStatus, "Status: Transaction updated.");
                    updateDashboardReport();
                }
            }
            sqlite3_finalize(stmt);
            break;
        }

        case ID_BTN_DELETE_ENTRY:
        {
            int selectedRow = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (selectedRow == -1) { MessageBoxA(hwnd, "Please select a transaction from the list to delete.", "Select Item", MB_OK); break; }
            
            LVITEM item = {0};
            item.iItem = selectedRow;
            item.mask = LVIF_PARAM;
            ListView_GetItem(hListView, &item);
            int realDbId = item.lParam;

            const char* sql = "DELETE FROM transactions WHERE id = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, realDbId); 
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    SetWindowTextA(hStatus, "Status: Transaction deleted.");
                    updateDashboardReport();
                }
            }
            sqlite3_finalize(stmt);
            break;
        }

        case ID_BTN_LOGOUT:
            loggedInUserId = -1;
            loggedInUsername = "";
            SetWindowTextA(hUsername, ""); SetWindowTextA(hPassword, "");
            SetWindowTextA(hStatus, "Status: Logged out successfully.");
            displayApplicationState(false); 
            break;

        case ID_BTN_DELETE_ACC:
            if (MessageBoxA(hwnd, "Are you sure you want to delete your account and all data?", "Warning", MB_YESNO | MB_ICONERROR) == IDYES) {
                const char* sql = "DELETE FROM users WHERE id = ?;"; sqlite3_stmt* stmt;
                if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int(stmt, 1, loggedInUserId);
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        MessageBoxA(hwnd, "Account deleted successfully.", "Done", MB_OK);
                        loggedInUserId = -1; loggedInUsername = "";
                        SetWindowTextA(hStatus, "Status: Ready.");
                        displayApplicationState(false);
                    }
                }
                sqlite3_finalize(stmt);
            }
            break;
        }
        break;
    }
    case WM_DESTROY:
        if (db) sqlite3_close(db);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    if (!initDatabase()) return 0;
    WNDCLASSEXA wc = {0}; wc.cbSize = sizeof(WNDCLASSEXA); wc.lpfnWndProc = WndProc; wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(245, 247, 250)); 
    wc.lpszClassName = "ModernFinanceWorkspace";
    
    // This tells Windows to pull your custom logo icon!
    wc.hIcon = LoadIcon(hInst, "MAINICON"); 
    wc.hIconSm = LoadIcon(hInst, "MAINICON");

    if (!RegisterClassExA(&wc)) return 0;

    HWND hwnd = CreateWindowExA(WS_EX_LEFT, "ModernFinanceWorkspace", "Predictive Analytics Financial Tracker Engine v1.0", 
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
                                CW_USEDEFAULT, CW_USEDEFAULT, 1100, 560, NULL, NULL, hInst, NULL);
    if (!hwnd) return 0;
    ShowWindow(hwnd, nShow); UpdateWindow(hwnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return msg.wParam;
}

// windres resources.rc -o resources.o
// g++ main.cpp sqlite3.o resources.o -o FinancialTrackerApp.exe -O3 -mwindows -lcomctl32 -ladvapi32 -static-libgcc -static-libstdc++