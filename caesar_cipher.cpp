#include <iostream>
#include <string>
#include <limits>

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

int main() {
    string text;
    int offset;
    int choice;

    cout << "==============================" << endl;
    cout << "      Caesar Cipher Program      " << endl;
    cout << "==============================" << endl;
    cout << "1. Encrypt" << endl;
    cout << "2. Decrypt" << endl;
    cout << "Please choose an option: ";
    cin >> choice;

    if (choice != 1 && choice != 2) {
        cout << "Invalid input, please enter 1 or 2." << endl;
        return 0;
    }

    // 清除缓冲区中的换行符，避免影响后面的 getline
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // 输入需要处理的文本，可以包含空格
    if (choice == 1) {
        cout << "Enter plaintext: ";
    } else {
        cout << "Enter ciphertext: ";
    }

    getline(cin, text);

    // 输入偏移量，用户可以自由选择正数、负数或大于 26 的数
    cout << "Enter offset: ";
    cin >> offset;

    bool encrypt = (choice == 1);
    string result = caesarCipher(text, offset, encrypt);

    if (encrypt) {
        cout << "Encryption result: " << result << endl;
    } else {
        cout << "Decryption result: " << result << endl;
    }

    return 0;
}