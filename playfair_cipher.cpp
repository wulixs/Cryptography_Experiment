#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace std;

// 将字母统一转成大写，便于后续统一处理
char toUpperLetter(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return static_cast<char>(ch - 'a' + 'A');
    }
    return ch;
}

// 只保留英文字母，并将 J 统一映射为 I，这是 Playfair 密码的标准约定
string normalizeLetters(const string &text) {
    string result;
    for (char ch : text) {
        if (isalpha(static_cast<unsigned char>(ch))) {
            char upper = toUpperLetter(ch);
            if (upper == 'J') {
                upper = 'I';
            }
            result.push_back(upper);
        }
    }
    return result;
}

// 构造 5x5 的 Playfair 密钥矩阵
// 先写入密钥中出现过的字母，再按 A-Z 补齐，跳过 J
void buildKeySquare(const string &key, array<char, 25> &square, array<pair<int, int>, 26> &position) {
    array<bool, 26> used{};
    string normalizedKey = normalizeLetters(key);

    int index = 0;
    auto appendLetter = [&](char ch) {
        int mapped = ch - 'A';
        if (mapped == ('J' - 'A')) {
            mapped = 'I' - 'A';
        }

        if (!used[mapped]) {
            used[mapped] = true;
            square[index] = static_cast<char>('A' + mapped);
            position[mapped] = {index / 5, index % 5};
            ++index;
        }
    };

    for (char ch : normalizedKey) {
        appendLetter(ch);
    }

    for (char ch = 'A'; ch <= 'Z'; ++ch) {
        if (ch == 'J') {
            continue;
        }
        appendLetter(ch);
    }
}

// 打印 5x5 密钥矩阵，方便用户核对
void printKeySquare(const array<char, 25> &square) {
    cout << "Key square:" << endl;
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            cout << square[row * 5 + col] << ' ';
        }
        cout << endl;
    }
}

// 按照 Playfair 规则把明文拆分成二元组
// 规则：
// 1. 相邻重复字母之间插入 X；如果当前字母就是 X，则插入 Q，避免重复冲突
// 2. 如果最后剩下一个字母，则补一个 X；若末字母本身是 X，则补 Q
vector<pair<char, char>> buildPlaintextPairs(const string &plainText) {
    string letters = normalizeLetters(plainText);
    vector<pair<char, char>> pairs;

    for (size_t i = 0; i < letters.size();) {
        char first = letters[i];
        char second = 0;

        if (i + 1 < letters.size()) {
            second = letters[i + 1];
            if (first == second) {
                second = (first == 'X') ? 'Q' : 'X';
                ++i;
            } else {
                i += 2;
            }
        } else {
            second = (first == 'X') ? 'Q' : 'X';
            ++i;
        }

        pairs.push_back({first, second});
    }

    return pairs;
}

// 解密时输入一般已经是成对字母，这里只负责清洗并在奇数长度时补齐
vector<pair<char, char>> buildCiphertextPairs(const string &cipherText) {
    string letters = normalizeLetters(cipherText);
    if (letters.size() % 2 != 0) {
        letters.push_back('X');
    }

    vector<pair<char, char>> pairs;
    for (size_t i = 0; i + 1 < letters.size(); i += 2) {
        pairs.push_back({letters[i], letters[i + 1]});
    }
    return pairs;
}

// 根据密钥矩阵和位置表，对一对字母执行加密或解密
pair<char, char> transformPair(char a, char b, const array<char, 25> &square,
                               const array<pair<int, int>, 26> &position, bool encrypt) {
    int indexA = a - 'A';
    int indexB = b - 'A';
    if (indexA == ('J' - 'A')) {
        indexA = 'I' - 'A';
    }
    if (indexB == ('J' - 'A')) {
        indexB = 'I' - 'A';
    }

    pair<int, int> posA = position[indexA];
    pair<int, int> posB = position[indexB];

    if (posA.first == posB.first) {
        // 同一行时，向右移动一格；解密时则向左移动一格
        int shift = encrypt ? 1 : 4;
        char outA = square[posA.first * 5 + (posA.second + shift) % 5];
        char outB = square[posB.first * 5 + (posB.second + shift) % 5];
        return {outA, outB};
    }

    if (posA.second == posB.second) {
        // 同一列时，向下移动一格；解密时则向上移动一格
        int shift = encrypt ? 1 : 4;
        char outA = square[((posA.first + shift) % 5) * 5 + posA.second];
        char outB = square[((posB.first + shift) % 5) * 5 + posB.second];
        return {outA, outB};
    }

    // 不同行不同列时，使用矩形替换规则
    char outA = square[posA.first * 5 + posB.second];
    char outB = square[posB.first * 5 + posA.second];
    return {outA, outB};
}

// Playfair 加密
string playfairEncrypt(const string &plainText, const array<char, 25> &square,
                       const array<pair<int, int>, 26> &position) {
    vector<pair<char, char>> pairs = buildPlaintextPairs(plainText);
    string result;
    result.reserve(pairs.size() * 2);

    for (const auto &item : pairs) {
        pair<char, char> encrypted = transformPair(item.first, item.second, square, position, true);
        result.push_back(encrypted.first);
        result.push_back(encrypted.second);
    }

    return result;
}

// 处理解密后的文本，尝试移除加密时人为插入的填充字母
// 这里采用常见的启发式规则：删除 A X A 结构中的 X，并去掉末尾补齐用的 X
string removePaddingLetters(const string &text) {
    string result;
    for (size_t i = 0; i < text.size(); ++i) {
        if (i + 2 < text.size() && text[i] == text[i + 2] && text[i + 1] == 'X') {
            result.push_back(text[i]);
            ++i;
            continue;
        }
        result.push_back(text[i]);
    }

    if (!result.empty() && result.back() == 'X') {
        result.pop_back();
    }

    return result;
}

// Playfair 解密
string playfairDecrypt(const string &cipherText, const array<char, 25> &square,
                       const array<pair<int, int>, 26> &position) {
    vector<pair<char, char>> pairs = buildCiphertextPairs(cipherText);
    string result;
    result.reserve(pairs.size() * 2);

    for (const auto &item : pairs) {
        pair<char, char> decrypted = transformPair(item.first, item.second, square, position, false);
        result.push_back(decrypted.first);
        result.push_back(decrypted.second);
    }

    return removePaddingLetters(result);
}

int main() {
    int choice = 0;
    string key;
    string text;
    array<char, 25> square{};
    array<pair<int, int>, 26> position{};

    cout << "====================================" << endl;
    cout << "         Playfair Cipher Program" << endl;
    cout << "====================================" << endl;
    cout << "1. Encrypt" << endl;
    cout << "2. Decrypt" << endl;
    cout << "Please choose an option: ";
    cin >> choice;

    if (choice != 1 && choice != 2) {
        cout << "Invalid input. Please enter 1 or 2." << endl;
        return 0;
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "Enter the keyword: ";
    getline(cin, key);
    if (normalizeLetters(key).empty()) {
        cout << "Invalid key. The keyword must contain at least one letter." << endl;
        return 0;
    }

    cout << "Enter the text: ";
    getline(cin, text);

    buildKeySquare(key, square, position);
    printKeySquare(square);

    if (choice == 1) {
        string result = playfairEncrypt(text, square, position);
        cout << "Encryption result: " << result << endl;
    } else {
        string result = playfairDecrypt(text, square, position);
        cout << "Decryption result: " << result << endl;
    }

    return 0;
}