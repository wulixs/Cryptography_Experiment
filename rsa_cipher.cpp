#include <cctype>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// RSA 教学版配置：把所有需要用户输入或程序推导的参数集中起来，便于调试和展示
struct RsaConfig {
    unsigned long long p = 0;
    unsigned long long q = 0;
    unsigned long long e = 0;
    unsigned long long n = 0;
    unsigned long long phi = 0;
    unsigned long long d = 0;
    size_t blockByteCount = 0;
};

// 去掉字符串首尾空白，方便做健壮的控制台输入
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

// 把逗号替换为空格，方便用户输入“1, 2, 3”这种列表形式
string normalizeSeparators(string text) {
    for (char &ch : text) {
        if (ch == ',') {
            ch = ' ';
        }
    }
    return text;
}

// 读取一个无符号整数，要求整行输入必须完全合法
unsigned long long readUnsignedLongLong(const string &prompt) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        line = trim(line);

        if (line.empty()) {
            cout << "Invalid input. Please enter a positive integer." << endl;
            continue;
        }

        try {
            size_t consumed = 0;
            unsigned long long value = stoull(line, &consumed, 10);
            if (consumed != line.size()) {
                throw invalid_argument("extra characters");
            }
            return value;
        } catch (...) {
            cout << "Invalid input. Please enter a positive integer." << endl;
        }
    }
}

// 读取一行普通文本，保留空格和标点
string readTextLine(const string &prompt) {
    cout << prompt;
    string line;
    getline(cin, line);
    return line;
}

// 读取一个整数菜单选项
int readChoice(const string &prompt) {
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        line = trim(line);

        if (line.empty()) {
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }

        try {
            size_t consumed = 0;
            int value = stoi(line, &consumed, 10);
            if (consumed != line.size()) {
                throw invalid_argument("extra characters");
            }
            return value;
        } catch (...) {
            cout << "Invalid input. Please enter a number." << endl;
        }
    }
}

// 解析空格或逗号分隔的无符号整数列表，用于输入密文分组
bool parseUnsignedList(const string &line, vector<unsigned long long> &values) {
    values.clear();
    istringstream stream(normalizeSeparators(line));
    unsigned long long number = 0;

    while (stream >> number) {
        values.push_back(number);
    }

    return stream.eof();
}

// 欧几里得算法求最大公约数
unsigned long long gcdUnsigned(unsigned long long a, unsigned long long b) {
    while (b != 0) {
        unsigned long long temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

// 安全乘法取模，避免中间结果溢出
unsigned long long mulMod(unsigned long long a, unsigned long long b, unsigned long long mod) {
    unsigned long long result = 0;
    a %= mod;
    while (b > 0) {
        if (b & 1ULL) {
            if (result >= mod - a) {
                result = result - (mod - a);
            } else {
                result += a;
            }
        }

        if (a >= mod - a) {
            a = a - (mod - a);
        } else {
            a += a;
        }

        b >>= 1;
    }

    return result;
}

// 下面这些输出辅助函数会在后面的流程里反复用到，先声明再定义，避免前向引用问题
void printDivider();
void printSectionTitle(const string &title);
void printBracketList(const string &label, const vector<unsigned long long> &blocks);
void printByteListCompact(const string &label, const vector<unsigned char> &bytes);

// 快速幂取模，同时逐步打印每一轮的中间结果
unsigned long long powModVerbose(unsigned long long base, unsigned long long exponent, unsigned long long mod, const string &label) {
    printDivider();
    cout << label << endl;
    printDivider();
    cout << "Base:      " << base << endl;
    cout << "Exponent:  " << exponent << endl;
    cout << "Modulus:   " << mod << endl;

    if (mod == 1) {
        cout << "  Result = 0" << endl;
        return 0;
    }

    if (exponent == 0) {
        cout << "  Result = 1" << endl;
        return 1 % mod;
    }

    // 左到右二进制快速幂：每处理一位都先平方，再根据当前位决定是否乘基数
    string binary;
    for (unsigned long long temp = exponent; temp > 0; temp >>= 1) {
        binary.insert(binary.begin(), static_cast<char>('0' + (temp & 1ULL)));
    }

    cout << "Binary exponent: " << binary << endl;

    unsigned long long result = 1 % mod;
    unsigned long long normalizedBase = base % mod;

    for (size_t index = 0; index < binary.size(); ++index) {
        char bit = binary[index];

        unsigned long long squared = mulMod(result, result, mod);
        cout << "  Step " << index + 1 << "/" << binary.size() << " - square: (" << result << "^2) mod " << mod << " = " << squared << endl;
        result = squared;

        if (bit == '1') {
            unsigned long long multiplied = mulMod(result, normalizedBase, mod);
            cout << "  Step " << index + 1 << "/" << binary.size() << " - multiply by base: (" << result << " * " << normalizedBase << ") mod " << mod << " = " << multiplied << endl;
            result = multiplied;
        } else {
            cout << "  Step " << index + 1 << "/" << binary.size() << " - bit = 0, skip multiply" << endl;
        }
    }

    cout << "Result:    " << result << endl;
    return result;
}

// 扩展欧几里得算法，用于求模逆元
long long extendedGcd(long long a, long long b, long long &x, long long &y) {
    if (b == 0) {
        x = 1;
        y = 0;
        return a;
    }

    long long x1 = 0;
    long long y1 = 0;
    long long gcd = extendedGcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return gcd;
}

// 计算 a 在 mod 下的乘法逆元；不存在时返回空值
optional<unsigned long long> modInverse(unsigned long long a, unsigned long long mod) {
    long long x = 0;
    long long y = 0;
    long long gcd = extendedGcd(static_cast<long long>(a % mod), static_cast<long long>(mod), x, y);
    if (gcd != 1) {
        return nullopt;
    }

    long long result = x % static_cast<long long>(mod);
    if (result < 0) {
        result += static_cast<long long>(mod);
    }

    return static_cast<unsigned long long>(result);
}

// 判断一个数是否为素数，用于检查 RSA 的 p 和 q
bool isPrime(unsigned long long value) {
    if (value < 2) {
        return false;
    }
    if (value == 2 || value == 3) {
        return true;
    }
    if (value % 2 == 0 || value % 3 == 0) {
        return false;
    }

    for (unsigned long long factor = 5; factor <= value / factor; factor += 6) {
        if (value % factor == 0 || value % (factor + 2) == 0) {
            return false;
        }
    }

    return true;
}

// 计算 RSA 可用于单个分组的最大字节数，要求 256^k < n
size_t computeBlockByteCount(unsigned long long modulus) {
    size_t blockBytes = 0;
    unsigned long long capacity = 1;

    while (capacity <= (numeric_limits<unsigned long long>::max() / 256ULL) && capacity * 256ULL < modulus) {
        capacity *= 256ULL;
        ++blockBytes;
    }

    return blockBytes;
}

// 将字节序列按固定大小打包成一个整数分组
unsigned long long bytesToBlockValue(const vector<unsigned char> &bytes) {
    unsigned long long value = 0;
    for (unsigned char byte : bytes) {
        value = value * 256ULL + static_cast<unsigned long long>(byte);
    }
    return value;
}

// 把一个整数分组还原成固定字节数的字节序列
vector<unsigned char> blockValueToBytes(unsigned long long value, size_t blockByteCount) {
    vector<unsigned char> bytes(blockByteCount, 0);
    for (size_t index = 0; index < blockByteCount; ++index) {
        size_t reverseIndex = blockByteCount - 1 - index;
        bytes[reverseIndex] = static_cast<unsigned char>(value % 256ULL);
        value /= 256ULL;
    }
    return bytes;
}

// 打印一个字节列表，便于观察明文如何被编码
void printByteList(const string &label, const vector<unsigned char> &bytes) {
    cout << label;
    if (bytes.empty()) {
        cout << "<empty>" << endl;
        return;
    }

    for (size_t index = 0; index < bytes.size(); ++index) {
        cout << static_cast<unsigned int>(bytes[index]);
        if (index + 1 < bytes.size()) {
            cout << ' ';
        }
    }
    cout << endl;
}

// 打印一个整数分组列表，便于观察 RSA 的数值块
void printBlockList(const string &label, const vector<unsigned long long> &blocks) {
    cout << label;
    if (blocks.empty()) {
        cout << "<empty>" << endl;
        return;
    }

    for (size_t index = 0; index < blocks.size(); ++index) {
        cout << blocks[index];
        if (index + 1 < blocks.size()) {
            cout << ' ';
        }
    }
    cout << endl;
}

// 下面这些输出辅助函数会在后面的流程里反复用到，先声明再定义，避免前向引用问题
void printDivider();
void printSectionTitle(const string &title);
void printBracketList(const string &label, const vector<unsigned long long> &blocks);
void printByteListCompact(const string &label, const vector<unsigned char> &bytes);

// 打印统一分隔线，增强控制台输出的层次感
void printDivider() {
    cout << "------------------------------------------------------------" << endl;
}

// 打印一个清晰的步骤标题
void printSectionTitle(const string &title) {
    printDivider();
    cout << title << endl;
    printDivider();
}

// 打印一个更直观的列表，方便展示分组结果
void printBracketList(const string &label, const vector<unsigned long long> &blocks) {
    cout << label << "[";
    for (size_t index = 0; index < blocks.size(); ++index) {
        cout << blocks[index];
        if (index + 1 < blocks.size()) {
            cout << ", ";
        }
    }
    cout << "]" << endl;
}

// 打印字节内容，列表形式比单纯空格分隔更容易阅读
void printByteListCompact(const string &label, const vector<unsigned char> &bytes) {
    cout << label << "[";
    for (size_t index = 0; index < bytes.size(); ++index) {
        cout << static_cast<unsigned int>(bytes[index]);
        if (index + 1 < bytes.size()) {
            cout << ", ";
        }
    }
    cout << "]" << endl;
}

// 把一段文本转换为原始字节序列
vector<unsigned char> textToBytes(const string &text) {
    vector<unsigned char> bytes;
    bytes.reserve(text.size());
    for (unsigned char ch : text) {
        bytes.push_back(ch);
    }
    return bytes;
}

// 将字节序列拆成 RSA 分组，并在最后一组补 0
vector<unsigned long long> splitBytesToBlocks(const vector<unsigned char> &bytes, size_t blockByteCount, size_t &paddingBytes) {
    vector<unsigned long long> blocks;
    paddingBytes = 0;

    if (blockByteCount == 0) {
        return blocks;
    }

    size_t totalBlocks = (bytes.size() + blockByteCount - 1) / blockByteCount;
    paddingBytes = totalBlocks * blockByteCount - bytes.size();

    vector<unsigned char> working = bytes;
    working.insert(working.end(), paddingBytes, 0);

    for (size_t offset = 0; offset < working.size(); offset += blockByteCount) {
        vector<unsigned char> blockBytes;
        blockBytes.reserve(blockByteCount);
        for (size_t index = 0; index < blockByteCount; ++index) {
            blockBytes.push_back(working[offset + index]);
        }
        blocks.push_back(bytesToBlockValue(blockBytes));
    }

    return blocks;
}

// 根据补位数量，从解密后的字节序列中去掉最后的 0 字节
vector<unsigned char> removePaddingBytes(const vector<unsigned char> &bytes, size_t paddingBytes) {
    if (paddingBytes == 0 || paddingBytes > bytes.size()) {
        return bytes;
    }

    return vector<unsigned char>(bytes.begin(), bytes.end() - static_cast<vector<unsigned char>::difference_type>(paddingBytes));
}

// 把字节序列转回文本，原样保留字节内容
string bytesToText(const vector<unsigned char> &bytes) {
    string text;
    text.reserve(bytes.size());
    for (unsigned char byte : bytes) {
        text.push_back(static_cast<char>(byte));
    }
    return text;
}

// 读取 RSA 公私钥参数，并完成基本校验和派生参数计算
bool loadRsaParameters(RsaConfig &config) {
    printSectionTitle("Step 1: Enter RSA Parameters");
    cout << "Enter p, q, and e. The program will compute n, phi(n), and d automatically." << endl;
    config.p = readUnsignedLongLong("p = ");
    config.q = readUnsignedLongLong("q = ");
    config.e = readUnsignedLongLong("e = ");

    if (!isPrime(config.p)) {
        cout << "Error: p is not prime." << endl;
        return false;
    }
    if (!isPrime(config.q)) {
        cout << "Error: q is not prime." << endl;
        return false;
    }
    if (config.p == config.q) {
        cout << "Error: p and q must be different primes." << endl;
        return false;
    }

    config.n = config.p * config.q;
    config.phi = (config.p - 1) * (config.q - 1);

    printSectionTitle("Step 2: Derived Key Values");
    cout << "n = " << config.n << endl;
    cout << "phi(n) = " << config.phi << endl;

    if (config.e <= 1 || config.e >= config.phi) {
        cout << "Error: e must satisfy 1 < e < phi(n)." << endl;
        return false;
    }

    unsigned long long gcdValue = gcdUnsigned(config.e, config.phi);
    cout << "gcd(e, phi(n)) = " << gcdValue << endl;
    if (gcdValue != 1) {
        cout << "Error: e and phi(n) are not coprime, so the private key cannot be generated." << endl;
        return false;
    }

    optional<unsigned long long> inverse = modInverse(config.e, config.phi);
    if (!inverse.has_value()) {
        cout << "Error: modular inverse does not exist." << endl;
        return false;
    }

    config.d = inverse.value();
    cout << "d = " << config.d << endl;

    config.blockByteCount = computeBlockByteCount(config.n);
    cout << "Maximum block byte count = " << config.blockByteCount << endl;
    if (config.blockByteCount == 0) {
        cout << "Error: n is too small. It must be greater than 255 so that byte-based plaintext blocks can be encrypted." << endl;
        return false;
    }

    printSectionTitle("Step 3: Key Summary");
    cout << "Public key  : (" << config.e << ", " << config.n << ')' << endl;
    cout << "Private key : (" << config.d << ", " << config.n << ')' << endl;
    cout << "Block size  : " << config.blockByteCount << " byte(s) per plaintext block" << endl;
    return true;
}

// 处理 RSA 加密流程，并显示每一步中间结果
void encryptAndVerify(const RsaConfig &config) {
    printSectionTitle("Mode 1: Encrypt Plaintext and Verify Decryption");
    string plaintext = readTextLine("Enter plaintext: ");
    vector<unsigned char> plainBytes = textToBytes(plaintext);

    printSectionTitle("Plaintext Preview");
    cout << "Original text: " << plaintext << endl;
    cout << "Plaintext length: " << plainBytes.size() << " byte(s)" << endl;
    printByteListCompact("Plaintext bytes: ", plainBytes);

    size_t paddingBytes = 0;
    vector<unsigned long long> plainBlocks = splitBytesToBlocks(plainBytes, config.blockByteCount, paddingBytes);
    printSectionTitle("Plaintext Blocking");
    cout << "Bytes per block: " << config.blockByteCount << endl;
    cout << "Padding bytes added to last block: " << paddingBytes << endl;

    printBracketList("Plaintext blocks: ", plainBlocks);

    vector<unsigned long long> cipherBlocks;
    cipherBlocks.reserve(plainBlocks.size());

    for (size_t index = 0; index < plainBlocks.size(); ++index) {
        printSectionTitle(string("Encrypt Block ") + to_string(index + 1));
        cout << "Plain block value: " << plainBlocks[index] << endl;
        unsigned long long encrypted = powModVerbose(plainBlocks[index], config.e, config.n, "  Modular exponentiation for encryption");
        cout << "Cipher block value: " << encrypted << endl;
        cipherBlocks.push_back(encrypted);
    }

    printSectionTitle("Ciphertext Result");
    printBracketList("Ciphertext blocks: ", cipherBlocks);

    // 为了验证完整流程，直接用私钥把刚生成的密文解回来
    vector<unsigned char> recoveredBytes;
    for (size_t index = 0; index < cipherBlocks.size(); ++index) {
        printSectionTitle(string("Decrypt Block ") + to_string(index + 1));
        cout << "Cipher block value: " << cipherBlocks[index] << endl;
        unsigned long long decrypted = powModVerbose(cipherBlocks[index], config.d, config.n, "  Modular exponentiation for decryption");
        cout << "Decrypted block value: " << decrypted << endl;

        vector<unsigned char> blockBytes = blockValueToBytes(decrypted, config.blockByteCount);
        printByteListCompact("Block bytes: ", blockBytes);

        recoveredBytes.insert(recoveredBytes.end(), blockBytes.begin(), blockBytes.end());
    }

    vector<unsigned char> trimmedBytes = removePaddingBytes(recoveredBytes, paddingBytes);
    string recoveredText = bytesToText(trimmedBytes);

    printSectionTitle("Verification Result");
    printByteListCompact("Recovered bytes: ", trimmedBytes);
    cout << "Recovered plaintext: " << recoveredText << endl;
    cout << "Match original: " << (recoveredText == plaintext ? "YES" : "NO") << endl;
}

// 处理 RSA 解密流程，适合用户手动输入密文分组进行验证
void decryptCiphertext(const RsaConfig &config) {
    printSectionTitle("Mode 2: Decrypt Ciphertext Blocks");
    cout << "Enter ciphertext blocks separated by spaces or commas." << endl;
    string line;
    getline(cin, line);

    vector<unsigned long long> cipherBlocks;
    if (!parseUnsignedList(line, cipherBlocks)) {
        cout << "Error: invalid ciphertext block list." << endl;
        return;
    }

    if (cipherBlocks.empty()) {
        cout << "Error: ciphertext block list is empty." << endl;
        return;
    }

    printSectionTitle("Ciphertext Input");
    printBracketList("Ciphertext blocks: ", cipherBlocks);

    size_t paddingBytes = static_cast<size_t>(readUnsignedLongLong("Padding bytes to remove from the final plaintext block = "));
    if (paddingBytes > config.blockByteCount) {
        cout << "Error: padding bytes cannot exceed the block byte count." << endl;
        return;
    }

    for (unsigned long long block : cipherBlocks) {
        if (block >= config.n) {
            cout << "Error: every ciphertext block must be smaller than n." << endl;
            return;
        }
    }

    vector<unsigned char> recoveredBytes;
    for (size_t index = 0; index < cipherBlocks.size(); ++index) {
        printSectionTitle(string("Decrypt Block ") + to_string(index + 1));
        cout << "Cipher block value: " << cipherBlocks[index] << endl;
        unsigned long long decrypted = powModVerbose(cipherBlocks[index], config.d, config.n, "  Modular exponentiation for decryption");
        cout << "Decrypted block value: " << decrypted << endl;

        vector<unsigned char> blockBytes = blockValueToBytes(decrypted, config.blockByteCount);
        printByteListCompact("Block bytes: ", blockBytes);

        recoveredBytes.insert(recoveredBytes.end(), blockBytes.begin(), blockBytes.end());
    }

    vector<unsigned char> trimmedBytes = removePaddingBytes(recoveredBytes, paddingBytes);
    string recoveredText = bytesToText(trimmedBytes);

    printSectionTitle("Recovered Plaintext");
    printByteListCompact("Recovered bytes: ", trimmedBytes);
    cout << "Recovered plaintext: " << recoveredText << endl;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    printSectionTitle("RSA Cipher Demonstration");
    cout << "This program shows RSA parameter derivation, block splitting, encryption, and decryption step by step." << endl;
    cout << "1. Encrypt plaintext and verify by decrypting it back" << endl;
    cout << "2. Decrypt ciphertext blocks" << endl;
    cout << "Tip: choose primes p and q so that n = p * q is greater than 255." << endl;

    int mode = readChoice("Choose mode (1 or 2): ");

    RsaConfig config;
    if (!loadRsaParameters(config)) {
        return 1;
    }

    cout << endl;
    if (mode == 1) {
        encryptAndVerify(config);
    } else if (mode == 2) {
        decryptCiphertext(config);
    } else {
        cout << "Error: unsupported mode." << endl;
        return 1;
    }

    return 0;
}