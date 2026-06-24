#include <array>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// S-DES 的全部参数集中存放，方便用户自定义和打印说明
struct SDesConfig {
    vector<int> p10;
    vector<int> p8;
    vector<int> ip;
    vector<int> ipInverse;
    vector<int> ep;
    vector<int> p4;
    array<array<int, 4>, 4> s0{};
    array<array<int, 4>, 4> s1{};
};

// 去掉字符串首尾空白字符，便于读取用户输入
string trim(const string &text) {
    size_t start = 0;
    while (start < text.size() && isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    size_t end = text.size();
    while (end > start && isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

// 将输入中的逗号替换为空格，方便用户按“1, 2, 3”这种形式输入
string normalizeSeparators(string text) {
    for (char &ch : text) {
        if (ch == ',') {
            ch = ' ';
        }
    }
    return text;
}

// 将空格分隔的整数解析成向量
bool parseIntegerList(const string &line, vector<int> &values) {
    values.clear();
    istringstream stream(normalizeSeparators(line));
    int number = 0;

    while (stream >> number) {
        values.push_back(number);
    }

    return stream.eof();
}

// 解析 0/1 比特串，只保留比特字符
bool parseBinaryString(const string &line, int expectedLength, vector<int> &bits) {
    bits.clear();
    for (char ch : line) {
        if (ch == '0' || ch == '1') {
            bits.push_back(ch - '0');
        } else if (!isspace(static_cast<unsigned char>(ch))) {
            return false;
        }
    }

    return static_cast<int>(bits.size()) == expectedLength;
}

// 将比特串转成字符串，便于打印
string bitsToString(const vector<int> &bits) {
    string result;
    result.reserve(bits.size());
    for (int bit : bits) {
        result.push_back(static_cast<char>('0' + bit));
    }
    return result;
}

// 按照固定格式打印比特串
void printBits(const string &label, const vector<int> &bits) {
    cout << label << bitsToString(bits) << endl;
}

// 打印置换表
void printPermutation(const string &label, const vector<int> &permutation) {
    cout << label;
    for (size_t i = 0; i < permutation.size(); ++i) {
        cout << permutation[i];
        if (i + 1 < permutation.size()) {
            cout << ' ';
        }
    }
    cout << endl;
}

// 打印 4x4 的 S 盒
void printSBox(const string &label, const array<array<int, 4>, 4> &sbox) {
    cout << label << endl;
    for (const auto &row : sbox) {
        cout << row[0] << ' ' << row[1] << ' ' << row[2] << ' ' << row[3] << endl;
    }
}

// 标准 S-DES 默认参数
SDesConfig loadDefaultConfig() {
    SDesConfig config;
    config.p10 = {3, 5, 2, 7, 4, 10, 1, 9, 8, 6};
    config.p8 = {6, 3, 7, 4, 8, 5, 10, 9};
    config.ip = {2, 6, 3, 1, 4, 8, 5, 7};
    config.ep = {4, 1, 2, 3, 2, 3, 4, 1};
    config.p4 = {2, 4, 3, 1};
    config.s0 = {{{1, 0, 3, 2}, {3, 2, 1, 0}, {0, 2, 1, 3}, {3, 1, 3, 2}}};
    config.s1 = {{{0, 1, 2, 3}, {2, 0, 1, 3}, {3, 0, 1, 0}, {2, 1, 0, 3}}};
    return config;
}

// 计算置换的逆置换，IP^-1 通过用户输入的 IP 自动得到
vector<int> invertPermutation(const vector<int> &permutation) {
    vector<int> inverse(permutation.size(), 0);
    for (size_t i = 0; i < permutation.size(); ++i) {
        int position = permutation[i];
        inverse[static_cast<size_t>(position - 1)] = static_cast<int>(i + 1);
    }
    return inverse;
}

// 检查置换表是否合法：长度正确、范围正确、无重复
bool validatePermutation(const vector<int> &permutation, int expectedLength, int maxValue) {
    if (static_cast<int>(permutation.size()) != expectedLength) {
        return false;
    }

    vector<bool> used(static_cast<size_t>(maxValue + 1), false);
    for (int value : permutation) {
        if (value < 1 || value > maxValue || used[static_cast<size_t>(value)]) {
            return false;
        }
        used[static_cast<size_t>(value)] = true;
    }

    return true;
}

// 检查 EP 这类扩展表是否合法：长度正确、数值范围正确，允许重复
bool validateExpansionTable(const vector<int> &table, int expectedLength, int maxValue) {
    if (static_cast<int>(table.size()) != expectedLength) {
        return false;
    }

    for (int value : table) {
        if (value < 1 || value > maxValue) {
            return false;
        }
    }

    return true;
}

// 检查 2x4 S 盒中的数值是否合法
bool validateSBox(const array<array<int, 4>, 4> &sbox) {
    for (const auto &row : sbox) {
        for (int value : row) {
            if (value < 0 || value > 3) {
                return false;
            }
        }
    }
    return true;
}

// 从用户输入中读取一个整数选择项
int readChoice() {
    while (true) {
        string line;
        getline(cin, line);
        line = trim(line);

        if (line.empty()) {
            continue;
        }

        istringstream stream(line);
        int choice = 0;
        char extra = '\0';
        if (stream >> choice && !(stream >> extra)) {
            return choice;
        }

        cout << "输入无效，请输入数字。" << endl;
    }
}

// 判断用户是否接受默认参数
bool askYes(const string &prompt) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        line = trim(line);

        if (line.empty()) {
            return true;
        }

        char ch = static_cast<char>(tolower(static_cast<unsigned char>(line[0])));
        if (ch == 'y') {
            return true;
        }
        if (ch == 'n') {
            return false;
        }

        cout << "请回答 y 或 n。" << endl;
    }
}

// 读取一个指定长度的比特串
vector<int> readBitBlock(const string &prompt, int expectedLength) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);

        vector<int> bits;
        if (parseBinaryString(line, expectedLength, bits)) {
            return bits;
        }

        cout << "输入无效，请只输入 " << expectedLength << " 位 0 和 1。" << endl;
    }
}

// 读取一个置换表，允许用户直接回车使用默认值
vector<int> readPermutationTable(const string &prompt, int expectedLength, int maxValue, const vector<int> &defaultValue) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        line = trim(line);

        if (line.empty()) {
            return defaultValue;
        }

        vector<int> values;
        if (parseIntegerList(line, values) && validatePermutation(values, expectedLength, maxValue)) {
            return values;
        }

        cout << "置换表无效，请输入 " << expectedLength << " 个互不重复的整数，范围为 1 到 " << maxValue << "。" << endl;
    }
}

// 读取一个扩展表，允许重复索引
vector<int> readExpansionTable(const string &prompt, int expectedLength, int maxValue, const vector<int> &defaultValue) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        line = trim(line);

        if (line.empty()) {
            return defaultValue;
        }

        vector<int> values;
        if (parseIntegerList(line, values) && validateExpansionTable(values, expectedLength, maxValue)) {
            return values;
        }

        cout << "扩展表无效，请输入 " << expectedLength << " 个整数，范围为 1 到 " << maxValue << "，允许重复。" << endl;
    }
}

// 读取一个 2x4 的 S 盒，可按默认值逐行回车
array<array<int, 4>, 4> readSBox(const string &label, const array<array<int, 4>, 4> &defaultValue) {
    array<array<int, 4>, 4> result = defaultValue;

    for (int row = 0; row < 4; ++row) {
        while (true) {
              cout << label << " 第 " << (row + 1) << " 行（4 个数字，直接回车使用默认值："
                 << defaultValue[row][0] << ' ' << defaultValue[row][1] << ' ' << defaultValue[row][2] << ' '
                 << defaultValue[row][3] << "): ";

            string line;
            getline(cin, line);
            line = trim(line);

            if (line.empty()) {
                break;
            }

            vector<int> values;
            if (parseIntegerList(line, values) && values.size() == 4) {
                for (int col = 0; col < 4; ++col) {
                    result[row][col] = values[static_cast<size_t>(col)];
                }
                break;
            }

            cout << "该行无效，请输入 4 个 0 到 3 之间的整数。" << endl;
        }
    }

    return result;
}

// 对比特串执行置换
vector<int> permute(const vector<int> &bits, const vector<int> &permutation) {
    vector<int> result;
    result.reserve(permutation.size());
    for (int index : permutation) {
        result.push_back(bits[static_cast<size_t>(index - 1)]);
    }
    return result;
}

// 按位异或
vector<int> xorBits(const vector<int> &left, const vector<int> &right) {
    vector<int> result;
    result.reserve(left.size());
    for (size_t i = 0; i < left.size(); ++i) {
        result.push_back(left[i] ^ right[i]);
    }
    return result;
}

// 左循环移位
vector<int> leftRotate(const vector<int> &bits, int shift) {
    vector<int> result = bits;
    if (!result.empty()) {
        shift %= static_cast<int>(result.size());
        rotate(result.begin(), result.begin() + shift, result.end());
    }
    return result;
}

// 拼接两个比特串
vector<int> combine(const vector<int> &left, const vector<int> &right) {
    vector<int> result = left;
    result.insert(result.end(), right.begin(), right.end());
    return result;
}

// 将四位输入送入 S 盒，输出两位结果
vector<int> applySBox(const array<array<int, 4>, 4> &sbox, const vector<int> &bits4) {
    int row = bits4[0] * 2 + bits4[3];
    int col = bits4[1] * 2 + bits4[2];
    int value = sbox[static_cast<size_t>(row)][static_cast<size_t>(col)];
    return {value / 2, value % 2};
}

// 单轮 fk 运算，打印完整中间过程
vector<int> fk(const vector<int> &state8, const vector<int> &subKey, const SDesConfig &config, const string &roundName) {
    vector<int> left(state8.begin(), state8.begin() + 4);
    vector<int> right(state8.begin() + 4, state8.end());

    cout << roundName << endl;
    printBits("  左半部分 = ", left);
    printBits("  右半部分 = ", right);

    vector<int> expanded = permute(right, config.ep);
    printBits("  扩展置换 EP(R) = ", expanded);

    vector<int> xored = xorBits(expanded, subKey);
    printBits("  EP(R) 与子密钥异或 = ", xored);

    vector<int> leftPart(xored.begin(), xored.begin() + 4);
    vector<int> rightPart(xored.begin() + 4, xored.end());

    vector<int> s0Output = applySBox(config.s0, leftPart);
    vector<int> s1Output = applySBox(config.s1, rightPart);
    printBits("  S0 输出 = ", s0Output);
    printBits("  S1 输出 = ", s1Output);

    vector<int> sboxCombined = combine(s0Output, s1Output);
    printBits("  S0||S1 拼接结果 = ", sboxCombined);

    vector<int> p4 = permute(sboxCombined, config.p4);
    printBits("  P4 置换 = ", p4);

    vector<int> newLeft = xorBits(left, p4);
    printBits("  新左半部分 = ", newLeft);

    vector<int> result = combine(newLeft, right);
    printBits("  fk 输出 = ", result);
    return result;
}

// 交换左右 4 位
vector<int> swapHalves(const vector<int> &state8) {
    vector<int> result(state8.begin() + 4, state8.end());
    result.insert(result.end(), state8.begin(), state8.begin() + 4);
    return result;
}

// 生成 K1 和 K2，并打印密钥扩展全过程
pair<vector<int>, vector<int>> generateSubkeys(const vector<int> &key10, const SDesConfig &config) {
    cout << endl;
    cout << "密钥扩展" << endl;
    printBits("  密钥 = ", key10);

    vector<int> p10Result = permute(key10, config.p10);
    printBits("  P10(密钥) = ", p10Result);

    vector<int> left(p10Result.begin(), p10Result.begin() + 5);
    vector<int> right(p10Result.begin() + 5, p10Result.end());
    printBits("  左 5 位 = ", left);
    printBits("  右 5 位 = ", right);

    vector<int> ls1Left = leftRotate(left, 1);
    vector<int> ls1Right = leftRotate(right, 1);
    printBits("  左移 1 位后左半部分 = ", ls1Left);
    printBits("  左移 1 位后右半部分 = ", ls1Right);

    vector<int> ls1Combined = combine(ls1Left, ls1Right);
    printBits("  左移 1 位后拼接结果 = ", ls1Combined);

    vector<int> k1 = permute(ls1Combined, config.p8);
    printBits("  子密钥 K1 = ", k1);

    vector<int> ls2Left = leftRotate(ls1Left, 2);
    vector<int> ls2Right = leftRotate(ls1Right, 2);
    printBits("  再左移 2 位后左半部分 = ", ls2Left);
    printBits("  再左移 2 位后右半部分 = ", ls2Right);

    vector<int> ls2Combined = combine(ls2Left, ls2Right);
    printBits("  再左移 2 位后拼接结果 = ", ls2Combined);

    vector<int> k2 = permute(ls2Combined, config.p8);
    printBits("  子密钥 K2 = ", k2);

    return {k1, k2};
}

// 单个 8 位明文块加密
vector<int> encryptBlock(const vector<int> &plain8, const SDesConfig &config, const vector<int> &k1, const vector<int> &k2) {
    cout << endl;
    cout << "加密过程" << endl;
    printBits("明文块 = ", plain8);

    vector<int> afterIp = permute(plain8, config.ip);
    printBits("经过初始置换 IP = ", afterIp);

    vector<int> afterRound1 = fk(afterIp, k1, config, "第 1 轮");

    vector<int> afterSwap = swapHalves(afterRound1);
    printBits("交换左右半部分后 = ", afterSwap);

    vector<int> afterRound2 = fk(afterSwap, k2, config, "第 2 轮");

    vector<int> cipher8 = permute(afterRound2, config.ipInverse);
    printBits("经过逆初始置换 IP^-1 = ", cipher8);
    return cipher8;
}

// 单个 8 位密文块解密
vector<int> decryptBlock(const vector<int> &cipher8, const SDesConfig &config, const vector<int> &k1, const vector<int> &k2) {
    cout << endl;
    cout << "解密过程" << endl;
    printBits("密文块 = ", cipher8);

    vector<int> afterIp = permute(cipher8, config.ip);
    printBits("经过初始置换 IP = ", afterIp);

    vector<int> afterRound1 = fk(afterIp, k2, config, "第 1 轮（K2）");

    vector<int> afterSwap = swapHalves(afterRound1);
    printBits("交换左右半部分后 = ", afterSwap);

    vector<int> afterRound2 = fk(afterSwap, k1, config, "第 2 轮（K1）");

    vector<int> plain8 = permute(afterRound2, config.ipInverse);
    printBits("经过逆初始置换 IP^-1 = ", plain8);
    return plain8;
}

// 打印当前配置，方便用户核对
void printConfigSummary(const SDesConfig &config) {
    cout << endl;
    cout << "当前参数" << endl;
    printPermutation("  P10 = ", config.p10);
    printPermutation("  P8 = ", config.p8);
    printPermutation("  IP = ", config.ip);
    printPermutation("  IP^-1 = ", config.ipInverse);
    printPermutation("  EP = ", config.ep);
    printPermutation("  P4 = ", config.p4);
    printSBox("  S0 =", config.s0);
    printSBox("  S1 =", config.s1);
}

int main() {
    cout << "====================================" << endl;
    cout << "          S-DES 密码程序" << endl;
    cout << "====================================" << endl;
    cout << "1. 加密" << endl;
    cout << "2. 解密" << endl;
    cout << "请选择操作：" << flush;

    int choice = readChoice();
    if (choice != 1 && choice != 2) {
        cout << "输入无效，请输入 1 或 2。" << endl;
        return 0;
    }

    SDesConfig config = loadDefaultConfig();

    cout << endl;
    if (!askYes("是否使用默认 S-DES 参数？[Y/n]：")) {
        cout << "请输入自定义参数，直接回车可保持该项默认值。" << endl;
        config.p10 = readPermutationTable("P10（10 个数字，范围 1-10）：", 10, 10, config.p10);
        config.p8 = readPermutationTable("P8（8 个数字，范围 1-10）：", 8, 10, config.p8);
        config.ip = readPermutationTable("IP（8 个数字，范围 1-8）：", 8, 8, config.ip);
        config.ep = readExpansionTable("EP（8 个数字，范围 1-4，允许重复）：", 8, 4, config.ep);
        config.p4 = readPermutationTable("P4（4 个数字，范围 1-4）：", 4, 4, config.p4);
        config.s0 = readSBox("S0", config.s0);
        config.s1 = readSBox("S1", config.s1);
    }

    config.ipInverse = invertPermutation(config.ip);

    bool validP10 = validatePermutation(config.p10, 10, 10);
    bool validP8 = validatePermutation(config.p8, 8, 10);
    bool validIp = validatePermutation(config.ip, 8, 8);
    bool validEp = validateExpansionTable(config.ep, 8, 4);
    bool validP4 = validatePermutation(config.p4, 4, 4);
    bool validS0 = validateSBox(config.s0);
    bool validS1 = validateSBox(config.s1);

    if (!validP10 || !validP8 || !validIp || !validEp || !validP4 || !validS0 || !validS1) {
        cout << "检测到无效参数：" << endl;
        cout << "  P10：" << (validP10 ? "通过" : "失败") << endl;
        cout << "  P8 ：" << (validP8 ? "通过" : "失败") << endl;
        cout << "  IP ：" << (validIp ? "通过" : "失败") << endl;
        cout << "  EP ：" << (validEp ? "通过" : "失败") << endl;
        cout << "  P4 ：" << (validP4 ? "通过" : "失败") << endl;
        cout << "  S0 ：" << (validS0 ? "通过" : "失败") << endl;
        cout << "  S1 ：" << (validS1 ? "通过" : "失败") << endl;
        cout << "参数无效，请检查置换表和 S 盒取值。" << endl;
        return 0;
    }

    printConfigSummary(config);

    vector<int> key10 = readBitBlock("请输入 10 位密钥：", 10);
    vector<int> block8 = readBitBlock(choice == 1 ? "请输入 8 位明文块：" : "请输入 8 位密文块：", 8);

    pair<vector<int>, vector<int>> subkeys = generateSubkeys(key10, config);
    vector<int> k1 = subkeys.first;
    vector<int> k2 = subkeys.second;

    vector<int> result;
    if (choice == 1) {
        result = encryptBlock(block8, config, k1, k2);
        cout << endl;
        cout << "加密结果：" << bitsToString(result) << endl;
    } else {
        result = decryptBlock(block8, config, k1, k2);
        cout << endl;
        cout << "解密结果：" << bitsToString(result) << endl;
    }

    return 0;
}