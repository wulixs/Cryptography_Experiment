#include <cctype>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

using namespace std;

// 统一把字符转成大写字母，非字母直接返回原字符
char toUpperLetter(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return static_cast<char>(ch - 'a' + 'A');
    }
    return ch;
}

// 取模运算的统一封装，保证结果始终落在 [0, mod-1]
int modNormalize(long long value, int mod) {
    int result = static_cast<int>(value % mod);
    if (result < 0) {
        result += mod;
    }
    return result;
}

// 扩展欧几里得算法，用于求模逆元
int extendedGcd(int a, int b, int &x, int &y) {
    if (b == 0) {
        x = 1;
        y = 0;
        return a;
    }

    int x1 = 0;
    int y1 = 0;
    int gcd = extendedGcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return gcd;
}

// 计算 a 在 mod 下的乘法逆元；如果不存在则返回 -1
int modInverse(int a, int mod) {
    int x = 0;
    int y = 0;
    int gcd = extendedGcd(modNormalize(a, mod), mod, x, y);
    if (gcd != 1) {
        return -1;
    }
    return modNormalize(x, mod);
}

// 通过高斯-约旦消元法求矩阵在模 26 下的逆矩阵
// 如果矩阵不可逆，返回 false
bool invertMatrixMod26(const vector<vector<int>> &matrix, vector<vector<int>> &inverse) {
    const int mod = 26;
    const int n = static_cast<int>(matrix.size());

    vector<vector<int>> augmented(n, vector<int>(2 * n, 0));
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            augmented[row][col] = modNormalize(matrix[row][col], mod);
        }
        augmented[row][n + row] = 1;
    }

    for (int col = 0; col < n; ++col) {
        int pivotRow = -1;

        // 先找一个与 26 互质的主元，否则该列无法做模逆运算
        for (int row = col; row < n; ++row) {
            int value = augmented[row][col];
            if (value != 0 && std::gcd(value, mod) == 1) {
                pivotRow = row;
                break;
            }
        }

        if (pivotRow == -1) {
            return false;
        }

        if (pivotRow != col) {
            swap(augmented[pivotRow], augmented[col]);
        }

        int pivotValue = augmented[col][col];
        int inversePivot = modInverse(pivotValue, mod);
        if (inversePivot == -1) {
            return false;
        }

        // 将主元行归一化成 1
        for (int j = 0; j < 2 * n; ++j) {
            augmented[col][j] = modNormalize(1LL * augmented[col][j] * inversePivot, mod);
        }

        // 把当前列的其他元素消成 0
        for (int row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }

            int factor = augmented[row][col];
            if (factor == 0) {
                continue;
            }

            for (int j = 0; j < 2 * n; ++j) {
                augmented[row][j] = modNormalize(augmented[row][j] - 1LL * factor * augmented[col][j], mod);
            }
        }
    }

    inverse.assign(n, vector<int>(n, 0));
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            inverse[row][col] = modNormalize(augmented[row][n + col], mod);
        }
    }

    return true;
}

// 读取用户输入的密钥矩阵
bool readKeyMatrix(int n, vector<vector<int>> &matrix) {
    matrix.assign(n, vector<int>(n, 0));

    cout << "请输入密钥矩阵，共 " << n * n << " 个整数（每个元素会按 26 取模）：" << endl;
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            cout << "a[" << row + 1 << "][" << col + 1 << "] = ";
            if (!(cin >> matrix[row][col])) {
                return false;
            }
        }
    }

    return true;
}

// 只保留字母并统一转成大写，便于希尔密码计算
string normalizeText(const string &text) {
    string result;
    for (char ch : text) {
        if (isalpha(static_cast<unsigned char>(ch))) {
            result.push_back(toUpperLetter(ch));
        }
    }
    return result;
}

// 将文本补齐到矩阵长度的整数倍，补位字符使用 X
string padText(const string &text, int blockSize) {
    string result = text;
    while (static_cast<int>(result.size()) % blockSize != 0) {
        result.push_back('X');
    }
    return result;
}

// 矩阵与向量相乘，结果按 26 取模
vector<int> multiplyMatrixVector(const vector<vector<int>> &matrix, const vector<int> &vectorValue) {
    const int mod = 26;
    const int n = static_cast<int>(matrix.size());
    vector<int> result(n, 0);

    for (int row = 0; row < n; ++row) {
        long long sum = 0;
        for (int col = 0; col < n; ++col) {
            sum += 1LL * matrix[row][col] * vectorValue[col];
        }
        result[row] = modNormalize(sum, mod);
    }

    return result;
}

// 使用希尔密码加密
string hillEncrypt(const string &plainText, const vector<vector<int>> &keyMatrix) {
    const int n = static_cast<int>(keyMatrix.size());
    string normalized = normalizeText(plainText);

    // 希尔密码要求明文长度是矩阵阶数的整数倍，不足部分用 X 补齐
    normalized = padText(normalized, n);

    string cipherText;
    for (int index = 0; index < static_cast<int>(normalized.size()); index += n) {
        vector<int> block(n, 0);
        for (int i = 0; i < n; ++i) {
            block[i] = normalized[index + i] - 'A';
        }

        vector<int> encryptedBlock = multiplyMatrixVector(keyMatrix, block);
        for (int i = 0; i < n; ++i) {
            cipherText.push_back(static_cast<char>('A' + encryptedBlock[i]));
        }
    }

    return cipherText;
}

// 使用希尔密码解密
string hillDecrypt(const string &cipherText, const vector<vector<int>> &keyMatrix) {
    const int n = static_cast<int>(keyMatrix.size());
    vector<vector<int>> inverseMatrix;

    if (!invertMatrixMod26(keyMatrix, inverseMatrix)) {
        return "[错误] 该密钥矩阵在模 26 下不可逆，无法解密。";
    }

    string normalized = normalizeText(cipherText);
    if (normalized.empty()) {
        return "";
    }

    // 如果密文长度不是矩阵阶数的整数倍，补齐后再解密，避免越界
    normalized = padText(normalized, n);

    string plainText;
    for (int index = 0; index < static_cast<int>(normalized.size()); index += n) {
        vector<int> block(n, 0);
        for (int i = 0; i < n; ++i) {
            block[i] = normalized[index + i] - 'A';
        }

        vector<int> decryptedBlock = multiplyMatrixVector(inverseMatrix, block);
        for (int i = 0; i < n; ++i) {
            plainText.push_back(static_cast<char>('A' + decryptedBlock[i]));
        }
    }

    return plainText;
}

int main() {
    int choice = 0;
    int n = 0;
    string inputText;
    vector<vector<int>> keyMatrix;

    cout << "====================================" << endl;
    cout << "          希尔密码程序" << endl;
    cout << "====================================" << endl;
    cout << "1. 加密" << endl;
    cout << "2. 解密" << endl;
    cout << "请选择功能：";
    cin >> choice;

    if (choice != 1 && choice != 2) {
        cout << "输入有误，请输入 1 或 2。" << endl;
        return 0;
    }

    cout << "请输入密钥矩阵的阶数 n（例如 2 表示 2x2，3 表示 3x3）：";
    cin >> n;
    if (n <= 0) {
        cout << "矩阵阶数必须大于 0。" << endl;
        return 0;
    }

    if (!readKeyMatrix(n, keyMatrix)) {
        cout << "密钥矩阵输入失败，请确保输入的是整数。" << endl;
        return 0;
    }

    // 清除缓冲区中的换行符，避免影响后续 getline
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        cout << "请输入明文（程序会自动只保留英文字母并转成大写）：";
    } else {
        cout << "请输入密文（程序会自动只保留英文字母并转成大写）：";
    }

    getline(cin, inputText);

    if (choice == 1) {
        string cipherText = hillEncrypt(inputText, keyMatrix);
        cout << "希尔加密结果：" << cipherText << endl;
    } else {
        string plainText = hillDecrypt(inputText, keyMatrix);
        cout << "希尔解密结果：" << plainText << endl;
    }

    return 0;
}