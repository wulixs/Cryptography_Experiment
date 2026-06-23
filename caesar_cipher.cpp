#include <iostream>
#include <string>
#include <limits>
#include <clocale>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

using namespace std;

// 对单个字母执行凯撒位移，mode=true 表示加密，false 表示解密
char caesarShift(char ch, int offset, bool encrypt) {
    int realOffset = encrypt ? offset : -offset;
    realOffset %= 26;

    if (ch >= 'A' && ch <= 'Z') {
        return char('A' + (ch - 'A' + realOffset + 26) % 26);
    }

    if (ch >= 'a' && ch <= 'z') {
        return char('a' + (ch - 'a' + realOffset + 26) % 26);
    }

    // 非字母字符不处理
    return ch;
}

// 对整段文本执行凯撒密码处理
string caesarCipher(const string &text, int offset, bool encrypt) {
    string result = text;

    for (char &ch : result) {
        ch = caesarShift(ch, offset, encrypt);
    }

    return result;
}

// 将 ASCII 文本转换为宽字符串，便于和 wcout 混合输出
wstring asciiToWide(const string &text) {
    return wstring(text.begin(), text.end());
}

int main() {
#ifdef _WIN32
    // 在 Windows 控制台中启用 UTF-8，并使用 UTF-16 文本模式输出中文
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);
#endif

    // 让 C/C++ 本地化设置与系统区域匹配
    setlocale(LC_ALL, "");

    string text;
    int offset;
    int choice;

    wcout << L"==============================" << endl;
    wcout << L"        凯撒密码程序        " << endl;
    wcout << L"==============================" << endl;
    wcout << L"1. 加密" << endl;
    wcout << L"2. 解密" << endl;
    wcout << L"请选择功能：";
    cin >> choice;

    if (choice != 1 && choice != 2) {
        wcout << L"输入有误，请输入 1 或 2。" << endl;
        return 0;
    }

    // 清除缓冲区中的换行符，避免影响后面的 getline
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // 输入需要处理的文本，可以包含空格
    if (choice == 1) {
        wcout << L"请输入明文：";
    } else {
        wcout << L"请输入密文：";
    }

    getline(cin, text);

    // 输入偏移量，用户可以自由选择正数、负数或大于 26 的数
    wcout << L"请输入偏移量：";
    cin >> offset;

    bool encrypt = (choice == 1);
    string result = caesarCipher(text, offset, encrypt);

    if (encrypt) {
        wcout << L"凯撒加密结果：" << asciiToWide(result) << endl;
    } else {
        wcout << L"凯撒解密结果：" << asciiToWide(result) << endl;
    }

    return 0;
}